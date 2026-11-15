use anyhow::{Result, anyhow};
use quick_xml::events::{BytesEnd, BytesStart, BytesText, Event};
use quick_xml::{Reader, Writer};
use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;
use std::sync::Arc;
use tokio::sync::Semaphore;

#[derive(Serialize)]
struct Message {
    role: String,
    content: String,
}

#[derive(Serialize)]
struct CompletionRequest {
    #[serde(skip_serializing_if = "Option::is_none")]
    model: Option<String>,
    messages: Vec<Message>,
    temperature: f32,
}

#[derive(Deserialize)]
struct Choice {
    message: MessageContent,
}

#[derive(Deserialize)]
struct MessageContent {
    content: String,
}

#[derive(Deserialize)]
struct CompletionResponse {
    choices: Vec<Choice>,
}

pub struct Results {
    pub translation_count: usize,
    pub error_count: usize,
}

/// Formats an error into a specific, actionable message.
pub fn format_error(err: &anyhow::Error) -> String {
    let mut is_timeout = false;
    let mut is_connect = false;
    let mut status = None;

    for cause in err.chain() {
        if let Some(req_err) = cause.downcast_ref::<reqwest::Error>() {
            if req_err.is_timeout() {
                is_timeout = true;
            }
            if req_err.is_connect() {
                is_connect = true;
            }
            if let Some(s) = req_err.status() {
                status = Some(s);
            }
        }
        if let Some(io_err) = cause.downcast_ref::<std::io::Error>() {
            if io_err.kind() == std::io::ErrorKind::TimedOut {
                is_timeout = true;
            }
            if io_err.kind() == std::io::ErrorKind::ConnectionRefused {
                is_connect = true;
            }
        }
    }

    if is_timeout {
        format!("request timed out (try increasing --timeout): {err:#}")
    } else if is_connect {
        format!("connection failed (is the server running?): {err:#}")
    } else if let Some(status) = status {
        format!("HTTP error {status}: {err:#}")
    } else {
        format!("{err:#}")
    }
}

fn parse_translation_attributes(e: &BytesStart) -> Result<(bool, Vec<(String, String)>)> {
    let mut is_unfinished = false;
    let mut attrs = Vec::new();
    for attr in e.attributes().filter_map(|a| a.ok()) {
        let key = String::from_utf8_lossy(attr.key.as_ref()).to_string();
        let val = attr.unescape_value()?.to_string();
        if key == "type" && val == "unfinished" {
            is_unfinished = true;
        } else {
            attrs.push((key, val));
        }
    }
    Ok((is_unfinished, attrs))
}

fn write_unfinished_translation<W: Write>(
    writer: &mut Writer<W>,
    attributes: &[(String, String)],
    fallback_text: Option<&str>,
) -> Result<()> {
    let mut start = BytesStart::new("translation");
    start.push_attribute(("type", "unfinished"));
    for (k, v) in attributes {
        start.push_attribute((k.as_str(), v.as_str()));
    }

    match fallback_text {
        None => {
            writer.write_event(Event::Empty(start))?;
        }
        Some("") => {
            writer.write_event(Event::Start(start))?;
            writer.write_event(Event::End(BytesEnd::new("translation")))?;
        }
        Some(text) => {
            writer.write_event(Event::Start(start))?;
            let escaped = quick_xml::escape::escape(text);
            writer.write_event(Event::Text(BytesText::from_escaped(escaped)))?;
            writer.write_event(Event::End(BytesEnd::new("translation")))?;
        }
    }

    Ok(())
}

/// Submits the source text to the llama-server using an OpenAI-compatible request payload.
pub async fn translate_text(
    client: &reqwest::Client,
    url: &str,
    text: &str,
    target_lang: &str,
    model: Option<&str>,
) -> Result<String> {
    let prompt = format!(
        "Translate the following software string into the target language. Do not include quotes, explanations, or backticks unless they exist in the original. Respond with *only* the raw translated text.\n\nTarget Language: {}\nOriginal text:\n{}",
        target_lang, text
    );

    let payload = CompletionRequest {
        model: model.map(|m| m.to_string()),
        messages: vec![
            Message {
                role: "system".to_string(),
                content: "You are a professional software localization translator. Preserve all variable placeholders like %1, %2, {name}, or HTML tags exactly as they appear.".to_string(),
            },
            Message {
                role: "user".to_string(),
                content: prompt,
            },
        ],
        temperature: 0.1,
    };

    let res = client
        .post(url)
        .json(&payload)
        .send()
        .await?
        .error_for_status()?;

    let json_res: CompletionResponse = res.json().await?;
    let content = json_res
        .choices
        .first()
        .map(|c| c.message.content.trim().to_string())
        .ok_or_else(|| anyhow!("No text returned in llama.cpp response"))?;

    // Quick sanitization of spurious generation output often found in models (like wrapper quotation or code blocks)
    let mut sanitized = content.as_str();
    if (sanitized.starts_with('"') && sanitized.ends_with('"'))
        || (sanitized.starts_with('\'') && sanitized.ends_with('\''))
    {
        sanitized = &sanitized[1..sanitized.len() - 1];
    } else if sanitized.starts_with("```") && sanitized.ends_with("```") {
        sanitized = &sanitized[3..sanitized.len() - 3];
        if let Some(pos) = sanitized.find('\n') {
            sanitized = &sanitized[pos + 1..];
        }
    }

    Ok(sanitized.trim().to_string())
}

struct UnfinishedItem {
    source: String,
    attributes: Vec<(String, String)>,
    fallback_text: Option<String>,
}

enum Chunk {
    Raw(Vec<u8>),
    Unfinished(usize),
}

/// Processes TS XML from a reader and writes the modified XML to a writer.
pub async fn process_ts<R: BufRead, W: Write>(
    reader: &mut Reader<R>,
    writer: &mut Writer<W>,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
) -> Result<Results> {
    process_ts_with_semaphore(reader, writer, client, api_url, model, output, None).await
}

/// Processes TS XML from a reader with an optional concurrency semaphore and writes the modified XML to a writer.
pub async fn process_ts_with_semaphore<R: BufRead, W: Write>(
    reader: &mut Reader<R>,
    writer: &mut Writer<W>,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
    semaphore: Option<Arc<Semaphore>>,
) -> Result<Results> {
    process_ts_with_semaphore_and_cancel(
        reader, writer, client, api_url, model, output, semaphore, None,
    )
    .await
}

/// Processes TS XML from a reader with an optional concurrency semaphore and cancellation receiver.
#[allow(clippy::too_many_arguments)]
pub async fn process_ts_with_semaphore_and_cancel<R: BufRead, W: Write>(
    reader: &mut Reader<R>,
    writer: &mut Writer<W>,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
    semaphore: Option<Arc<Semaphore>>,
    cancel: Option<tokio::sync::watch::Receiver<bool>>,
) -> Result<Results> {
    let mut buf = Vec::new();
    let mut language = String::new();
    let mut results = Results {
        translation_count: 0,
        error_count: 0,
    };

    let mut inside_source = false;
    let mut current_source = String::new();

    let mut inside_numerus_message = false;
    let mut inside_unfinished_translation = false;
    let mut current_translation_attributes: Vec<(String, String)> = Vec::new();
    let mut unfinished_translation_text = String::new();

    let mut unfinished_items = Vec::new();
    let mut chunks = Vec::new();
    let mut chunk_writer = Writer::new(Vec::new());

    loop {
        match reader.read_event_into(&mut buf)? {
            Event::Eof => break,

            Event::Start(e) => {
                let name = e.name();
                if name.as_ref() == b"message" {
                    inside_numerus_message = false;
                    for attr in e.attributes().filter_map(|a| a.ok()) {
                        if attr.key.as_ref() == b"numerus" && attr.value.as_ref() == b"yes" {
                            inside_numerus_message = true;
                        }
                    }
                    chunk_writer.write_event(Event::Start(e))?;
                } else if name.as_ref() == b"TS" {
                    for attr in e.attributes().filter_map(|a| a.ok()) {
                        if attr.key.as_ref() == b"language" {
                            language = attr.unescape_value()?.to_string();
                        }
                    }
                    chunk_writer.write_event(Event::Start(e))?;
                } else if name.as_ref() == b"source" {
                    inside_source = true;
                    current_source.clear();
                    chunk_writer.write_event(Event::Start(e))?;
                } else if name.as_ref() == b"translation" {
                    let (is_unfinished, attrs) = parse_translation_attributes(&e)?;
                    current_translation_attributes = attrs;
                    unfinished_translation_text.clear();

                    if is_unfinished && !inside_numerus_message && !language.starts_with("en") {
                        inside_unfinished_translation = true;
                    } else {
                        chunk_writer.write_event(Event::Start(e))?;
                    }
                } else if inside_unfinished_translation {
                    // Child element (e.g. <numerusform>) inside an unfinished translation.
                    // Cancel treating it as a plain-text translation and preserve it directly in chunk_writer.
                    let mut start = BytesStart::new("translation");
                    start.push_attribute(("type", "unfinished"));
                    for (k, v) in &current_translation_attributes {
                        start.push_attribute((k.as_str(), v.as_str()));
                    }
                    chunk_writer.write_event(Event::Start(start))?;
                    if !unfinished_translation_text.is_empty() {
                        let escaped = quick_xml::escape::escape(&unfinished_translation_text);
                        chunk_writer.write_event(Event::Text(BytesText::from_escaped(escaped)))?;
                        unfinished_translation_text.clear();
                    }
                    chunk_writer.write_event(Event::Start(e))?;
                    inside_unfinished_translation = false;
                } else {
                    chunk_writer.write_event(Event::Start(e))?;
                }
            }

            Event::Empty(e) => {
                let name = e.name();
                if name.as_ref() == b"message" {
                    inside_numerus_message = false;
                    chunk_writer.write_event(Event::Empty(e))?;
                } else if name.as_ref() == b"translation" {
                    let (is_unfinished, current_attrs) = parse_translation_attributes(&e)?;

                    if is_unfinished && !inside_numerus_message && !language.starts_with("en") {
                        if language.is_empty() {
                            return Err(anyhow!(
                                "Could not auto-determine the 'language' attribute from <TS> node before hitting a translation!"
                            ));
                        }

                        let raw_bytes = std::mem::take(chunk_writer.get_mut());
                        if !raw_bytes.is_empty() {
                            chunks.push(Chunk::Raw(raw_bytes));
                        }

                        let idx = unfinished_items.len();
                        unfinished_items.push(UnfinishedItem {
                            source: current_source.clone(),
                            attributes: current_attrs,
                            fallback_text: None,
                        });
                        chunks.push(Chunk::Unfinished(idx));
                    } else {
                        chunk_writer.write_event(Event::Empty(e))?;
                    }
                } else if inside_unfinished_translation {
                    let mut start = BytesStart::new("translation");
                    start.push_attribute(("type", "unfinished"));
                    for (k, v) in &current_translation_attributes {
                        start.push_attribute((k.as_str(), v.as_str()));
                    }
                    chunk_writer.write_event(Event::Start(start))?;
                    if !unfinished_translation_text.is_empty() {
                        let escaped = quick_xml::escape::escape(&unfinished_translation_text);
                        chunk_writer.write_event(Event::Text(BytesText::from_escaped(escaped)))?;
                        unfinished_translation_text.clear();
                    }
                    chunk_writer.write_event(Event::Empty(e))?;
                    inside_unfinished_translation = false;
                } else {
                    chunk_writer.write_event(Event::Empty(e))?;
                }
            }

            Event::Text(e) => {
                if inside_unfinished_translation {
                    unfinished_translation_text.push_str(&e.unescape()?);
                } else {
                    if inside_source {
                        current_source.push_str(&e.unescape()?);
                    }
                    chunk_writer.write_event(Event::Text(e))?;
                }
            }

            Event::CData(e) => {
                if inside_unfinished_translation {
                    unfinished_translation_text.push_str(&String::from_utf8_lossy(e.as_ref()));
                } else {
                    if inside_source {
                        current_source.push_str(&String::from_utf8_lossy(e.as_ref()));
                    }
                    chunk_writer.write_event(Event::CData(e))?;
                }
            }

            Event::End(e) => {
                let name = e.name();
                if name.as_ref() == b"source" {
                    inside_source = false;
                    chunk_writer.write_event(Event::End(e))?;
                } else if name.as_ref() == b"translation" && inside_unfinished_translation {
                    if language.is_empty() {
                        return Err(anyhow!(
                            "Could not auto-determine the 'language' attribute from <TS> node before hitting a translation!"
                        ));
                    }

                    let raw_bytes = std::mem::take(chunk_writer.get_mut());
                    if !raw_bytes.is_empty() {
                        chunks.push(Chunk::Raw(raw_bytes));
                    }

                    let idx = unfinished_items.len();
                    unfinished_items.push(UnfinishedItem {
                        source: current_source.clone(),
                        attributes: current_translation_attributes.clone(),
                        fallback_text: Some(unfinished_translation_text.clone()),
                    });
                    chunks.push(Chunk::Unfinished(idx));

                    inside_unfinished_translation = false;
                } else {
                    if name.as_ref() == b"message" {
                        current_source.clear();
                        inside_numerus_message = false;
                    }
                    chunk_writer.write_event(Event::End(e))?;
                }
            }

            other => {
                chunk_writer.write_event(other)?;
            }
        }
        buf.clear();
    }

    let raw_bytes = std::mem::take(chunk_writer.get_mut());
    if !raw_bytes.is_empty() {
        chunks.push(Chunk::Raw(raw_bytes));
    }

    if !unfinished_items.is_empty() {
        let mut set = tokio::task::JoinSet::new();

        enum ItemEvent {
            Started { idx: usize },
            Finished { idx: usize, res: Result<String> },
            Empty { idx: usize },
        }

        let (event_tx, mut event_rx) = tokio::sync::mpsc::unbounded_channel();

        for (idx, item) in unfinished_items.iter().enumerate() {
            let client = client.clone();
            let sem = semaphore.clone();
            let url = api_url.to_string();
            let model = model.map(str::to_string);
            let source = item.source.clone();
            let lang = language.clone();
            let tx = event_tx.clone();
            let mut cancel_item = cancel.clone();
            set.spawn(async move {
                if source.is_empty() {
                    let _ = tx.send(ItemEvent::Empty { idx });
                    return (idx, None);
                }
                if cancel_item.as_ref().is_some_and(|c| *c.borrow()) {
                    return (idx, None);
                }
                let permit = match &sem {
                    Some(s) => {
                        if let Some(c) = &mut cancel_item {
                            tokio::select! {
                                p = s.acquire() => p.ok(),
                                _ = c.changed() => return (idx, None),
                            }
                        } else {
                            s.acquire().await.ok()
                        }
                    }
                    None => None,
                };
                if cancel_item.as_ref().is_some_and(|c| *c.borrow()) {
                    drop(permit);
                    return (idx, None);
                }
                let _ = tx.send(ItemEvent::Started { idx });
                let res = if let Some(c) = &mut cancel_item {
                    tokio::select! {
                        r = translate_text(&client, &url, &source, &lang, model.as_deref()) => r,
                        _ = c.changed() => {
                            drop(permit);
                            return (idx, None);
                        }
                    }
                } else {
                    translate_text(&client, &url, &source, &lang, model.as_deref()).await
                };
                drop(permit);
                let _ = tx.send(ItemEvent::Finished { idx, res });
                (idx, Some(()))
            });
        }
        drop(event_tx);

        let mut translated_results: Vec<Option<Option<Result<String>>>> =
            (0..unfinished_items.len()).map(|_| None).collect();
        let mut open_item: Option<usize> = None;

        while let Some(event) = event_rx.recv().await {
            match event {
                ItemEvent::Started { idx } => {
                    if open_item.take().is_some() {
                        let _ = writeln!(output);
                    }
                    let item = &unfinished_items[idx];
                    let _ = write!(
                        output,
                        "  Translating '{}' into '{}' …",
                        item.source, language
                    );
                    let _ = output.flush();
                    open_item = Some(idx);
                }
                ItemEvent::Finished { idx, res } => {
                    let item = &unfinished_items[idx];
                    if open_item == Some(idx) {
                        match &res {
                            Ok(translated) => {
                                let _ = writeln!(output, " Done -> {translated}");
                            }
                            Err(err) => {
                                let _ = writeln!(output, " Failed: {}", format_error(err));
                            }
                        }
                        open_item = None;
                    } else {
                        if open_item.take().is_some() {
                            let _ = writeln!(output);
                        }
                        match &res {
                            Ok(translated) => {
                                let _ = writeln!(output, "  Done '{}' -> {translated}", item.source);
                            }
                            Err(err) => {
                                let _ = writeln!(
                                    output,
                                    "  Failed '{}': {}",
                                    item.source,
                                    format_error(err)
                                );
                            }
                        }
                    }
                    let _ = output.flush();
                    translated_results[idx] = Some(Some(res));
                }
                ItemEvent::Empty { idx } => {
                    if open_item.take().is_some() {
                        let _ = writeln!(output);
                    }
                    let _ = writeln!(output, "  Empty source string cannot be translated");
                    let _ = output.flush();
                    translated_results[idx] = Some(None);
                }
            }
        }

        if open_item.take().is_some() {
            let _ = writeln!(output);
            let _ = output.flush();
        }

        while let Some(join_res) = set.join_next().await {
            join_res?;
        }

        for chunk in chunks {
            match chunk {
                Chunk::Raw(bytes) => {
                    writer.get_mut().write_all(&bytes)?;
                }
                Chunk::Unfinished(idx) => {
                    let item = &unfinished_items[idx];
                    let res = translated_results[idx].take();
                    match res {
                        Some(Some(Ok(translated))) => {
                            let mut start = BytesStart::new("translation");
                            for (k, v) in &item.attributes {
                                start.push_attribute((k.as_str(), v.as_str()));
                            }
                            writer.write_event(Event::Start(start))?;
                            let escaped = quick_xml::escape::escape(&translated);
                            writer.write_event(Event::Text(BytesText::from_escaped(escaped)))?;
                            writer.write_event(Event::End(BytesEnd::new("translation")))?;
                            results.translation_count += 1;
                        }
                        Some(Some(Err(_err))) => {
                            write_unfinished_translation(
                                writer,
                                &item.attributes,
                                item.fallback_text.as_deref(),
                            )?;
                            results.error_count += 1;
                        }
                        _ => {
                            write_unfinished_translation(
                                writer,
                                &item.attributes,
                                item.fallback_text.as_deref(),
                            )?;
                            results.error_count += 1;
                        }
                    }
                }
            }
        }
    } else {
        for chunk in chunks {
            if let Chunk::Raw(bytes) = chunk {
                writer.get_mut().write_all(&bytes)?;
            }
        }
    }

    Ok(results)
}

/// Parses and modifies a Qt TS file.
pub async fn process_ts_file(
    file_path: &Path,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
) -> Result<Results> {
    process_ts_file_with_semaphore(file_path, client, api_url, model, output, None).await
}

/// Parses and modifies a Qt TS file with an optional concurrency semaphore.
pub async fn process_ts_file_with_semaphore(
    file_path: &Path,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
    semaphore: Option<Arc<Semaphore>>,
) -> Result<Results> {
    process_ts_file_with_semaphore_and_cancel(
        file_path, client, api_url, model, output, semaphore, None,
    )
    .await
}

/// Parses and modifies a Qt TS file with an optional concurrency semaphore and cancellation receiver.
pub async fn process_ts_file_with_semaphore_and_cancel(
    file_path: &Path,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut (dyn Write + Send),
    semaphore: Option<Arc<Semaphore>>,
    cancel: Option<tokio::sync::watch::Receiver<bool>>,
) -> Result<Results> {
    let _ = writeln!(output, "Processing: {}", file_path.display());
    let _ = output.flush();

    let file = File::open(file_path)?;
    let mut reader = Reader::from_reader(BufReader::new(file));

    let mut out_data = Vec::new();
    let mut writer = Writer::new(&mut out_data);

    let results = process_ts_with_semaphore_and_cancel(
        &mut reader,
        &mut writer,
        client,
        api_url,
        model,
        output,
        semaphore,
        cancel,
    )
    .await?;

    if results.translation_count > 0 {
        let message = if results.error_count == 0 {
            "All unfinished translations have been translated.".to_string()
        } else {
            format!(
                "{} translations failed and are still unfinished.",
                results.error_count
            )
        };
        let mut out_file = File::create(file_path)?;
        out_file.write_all(&out_data)?;
        let _ = writeln!(
            output,
            "Saved {} finished translation(s) back to {}.\n{}\n",
            results.translation_count,
            file_path.display(),
            message
        );
    } else if results.error_count > 0 {
        let _ = writeln!(
            output,
            "All {} translations failed and are still unfinished\n",
            results.error_count
        );
    } else {
        let _ = writeln!(
            output,
            "Skipped: No unfinished translations found in {}.\n",
            file_path.display()
        );
    }

    Ok(results)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;
    use tokio::io::{AsyncReadExt, AsyncWriteExt};
    use tokio::net::TcpListener;

    async fn spawn_mock_server<F>(handler: F) -> (String, tokio::task::JoinHandle<()>)
    where
        F: Fn(String) -> (u16, String) + Send + Sync + 'static,
    {
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        let handle = tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                let mut buf = vec![0u8; 8192];
                let n = match socket.read(&mut buf).await {
                    Ok(n) if n > 0 => n,
                    _ => continue,
                };
                let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                let (status, body) = handler(req_str);
                let status_line = if status == 200 {
                    "HTTP/1.1 200 OK"
                } else {
                    "HTTP/1.1 500 Internal Server Error"
                };
                let resp = format!(
                    "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                    body.len()
                );
                let _ = socket.write_all(resp.as_bytes()).await;
                let _ = socket.flush().await;
            }
        });

        (url, handle)
    }

    #[tokio::test]
    async fn test_translate_text() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "\"Text löschen\""
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let client = reqwest::Client::new();
        let result = translate_text(&client, &url, "Clear", "de_DE", None)
            .await
            .unwrap();
        assert_eq!(result, "Text löschen");
    }

    #[tokio::test]
    async fn test_translate_text_code_block_sanitization() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "```\nTranslated Code\n```"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let client = reqwest::Client::new();
        let result = translate_text(&client, &url, "Original", "de_DE", Some("test-model"))
            .await
            .unwrap();
        assert_eq!(result, "Translated Code");
    }

    #[tokio::test]
    async fn test_process_ts_xml_translation() {
        let (url, _handle) = spawn_mock_server(|req| {
            let translated = if req.contains("Clear") {
                "Text löschen"
            } else if req.contains("User interface") {
                "Benutzeroberfläche"
            } else if req.contains("Special &amp; chars") || req.contains("Special & chars") {
                "Spezial & Zeichen"
            } else {
                "Unbekannt"
            };
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": translated
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de_DE">
<context>
    <name>QObject</name>
    <message>
        <location filename="../widgets/buttonoverlay.cpp" line="143"/>
        <source>Clear</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Existing finished</source>
        <translation>Bereits fertig</translation>
    </message>
    <message>
        <source>User interface</source>
        <translation type="unfinished"/>
    </message>
    <message>
        <source>Special &amp; chars</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        let results = process_ts(&mut reader, &mut writer, &client, &url, None, &mut out_log)
            .await
            .unwrap();

        assert_eq!(results.translation_count, 3);

        let output_str = String::from_utf8(out_buf).unwrap();
        assert!(output_str.contains("<translation>Text löschen</translation>"));
        assert!(output_str.contains("<translation>Bereits fertig</translation>"));
        assert!(output_str.contains("<translation>Benutzeroberfläche</translation>"));
        assert!(output_str.contains("<translation>Spezial &amp; Zeichen</translation>"));
        assert!(!output_str.contains("unfinished"));
    }

    #[tokio::test]
    async fn test_process_ts_xml_missing_language() {
        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1">
<context>
    <message>
        <source>Clear</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        let res = process_ts(
            &mut reader,
            &mut writer,
            &client,
            "http://localhost:1234",
            None,
            &mut out_log,
        )
        .await;

        assert!(res.is_err());
    }

    #[tokio::test]
    async fn test_process_ts_file_end_to_end() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "Aktualisieren"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let temp_dir = std::env::temp_dir().join(format!("translator_test_{}", std::process::id()));
        std::fs::create_dir_all(&temp_dir).unwrap();
        let file_path = temp_dir.join("test_de_DE.ts");

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <name>Updater</name>
    <message>
        <source>Updating</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>"#;
        std::fs::write(&file_path, input_xml).unwrap();

        let client = reqwest::Client::new();
        let mut out_log = Vec::new();

        let results = process_ts_file(&file_path, &client, &url, None, &mut out_log)
            .await
            .unwrap();

        assert_eq!(results.translation_count, 1);
        let result_content = std::fs::read_to_string(&file_path).unwrap();
        assert!(result_content.contains("<translation>Aktualisieren</translation>"));

        // Second run: should find no unfinished translations and skip writing
        let results2 = process_ts_file(&file_path, &client, &url, None, &mut out_log)
            .await
            .unwrap();
        assert_eq!(results2.translation_count, 0);

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[tokio::test]
    async fn test_format_error_timeout() {
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        tokio::spawn(async move {
            if let Ok((mut socket, _)) = listener.accept().await {
                // Sleep longer than the client timeout to trigger a timeout
                tokio::time::sleep(tokio::time::Duration::from_millis(200)).await;
                let mut buf = [0u8; 1024];
                let _ = socket.read(&mut buf).await;
            }
        });

        let client = reqwest::Client::builder()
            .timeout(std::time::Duration::from_millis(20))
            .build()
            .unwrap();

        let err = translate_text(&client, &url, "Test", "de_DE", None)
            .await
            .unwrap_err();

        let formatted = format_error(&err);
        assert!(
            formatted.contains("request timed out (try increasing --timeout)"),
            "Expected timeout error message, got: {formatted}"
        );
    }

    #[tokio::test]
    async fn test_format_error_connection_refused() {
        let client = reqwest::Client::new();
        // Use a port that won't accept connections
        let err = translate_text(
            &client,
            "http://127.0.0.1:1/v1/chat/completions",
            "Test",
            "de_DE",
            None,
        )
        .await
        .unwrap_err();

        let formatted = format_error(&err);
        assert!(
            formatted.contains("connection failed (is the server running?)"),
            "Expected connection failed error message, got: {formatted}"
        );
    }

    #[tokio::test]
    async fn test_format_error_http_status() {
        let (url, _handle) =
            spawn_mock_server(|_req| (500, "Internal Server Error".to_string())).await;

        let client = reqwest::Client::new();
        let err = translate_text(&client, &url, "Test", "de_DE", None)
            .await
            .unwrap_err();

        let formatted = format_error(&err);
        assert!(
            formatted.contains("HTTP error 500"),
            "Expected HTTP 500 error message, got: {formatted}"
        );
    }

    async fn spawn_mock_server_concurrent<F, Fut>(
        handler: F,
    ) -> (String, tokio::task::JoinHandle<()>)
    where
        F: Fn(String) -> Fut + Send + Sync + 'static,
        Fut: std::future::Future<Output = (u16, String)> + Send + 'static,
    {
        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");
        let handler = Arc::new(handler);

        let handle = tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                let handler = handler.clone();
                tokio::spawn(async move {
                    let mut buf = vec![0u8; 8192];
                    let n = match socket.read(&mut buf).await {
                        Ok(n) if n > 0 => n,
                        _ => return,
                    };
                    let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                    let (status, body) = handler(req_str).await;
                    let status_line = if status == 200 {
                        "HTTP/1.1 200 OK"
                    } else {
                        "HTTP/1.1 500 Internal Server Error"
                    };
                    let resp = format!(
                        "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = socket.write_all(resp.as_bytes()).await;
                    let _ = socket.flush().await;
                });
            }
        });

        (url, handle)
    }

    #[tokio::test]
    async fn test_concurrency_limit() {
        use std::sync::atomic::{AtomicUsize, Ordering};

        let active = Arc::new(AtomicUsize::new(0));
        let max_active = Arc::new(AtomicUsize::new(0));

        let active_clone = active.clone();
        let max_clone = max_active.clone();

        let (url, _handle) = spawn_mock_server_concurrent(move |_req| {
            let active = active_clone.clone();
            let max_active = max_clone.clone();
            async move {
                let cur = active.fetch_add(1, Ordering::SeqCst) + 1;
                max_active.fetch_max(cur, Ordering::SeqCst);
                tokio::time::sleep(tokio::time::Duration::from_millis(50)).await;
                active.fetch_sub(1, Ordering::SeqCst);
                (
                    200,
                    serde_json::json!({
                        "choices": [{
                            "message": {
                                "content": "Übersetzt"
                            }
                        }]
                    })
                    .to_string(),
                )
            }
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <name>QObject</name>
    <message><source>S1</source><translation type="unfinished"></translation></message>
    <message><source>S2</source><translation type="unfinished"></translation></message>
    <message><source>S3</source><translation type="unfinished"></translation></message>
    <message><source>S4</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let sem = Arc::new(Semaphore::new(2));

        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        let results = process_ts_with_semaphore(
            &mut reader,
            &mut writer,
            &client,
            &url,
            None,
            &mut out_log,
            Some(sem),
        )
        .await
        .unwrap();

        assert_eq!(results.translation_count, 4);
        assert_eq!(results.error_count, 0);

        let observed_max = max_active.load(Ordering::SeqCst);
        assert!(
            observed_max <= 2,
            "Expected max concurrent requests <= 2, got {observed_max}"
        );
        assert!(
            observed_max >= 2,
            "Expected concurrent execution with 2 requests, got {observed_max}"
        );
    }

    #[tokio::test]
    async fn test_logging_sequential_no_repetition() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "Fertig"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>Msg1</source><translation type="unfinished"></translation></message>
    <message><source>Msg2</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let sem = Arc::new(Semaphore::new(1));
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        process_ts_with_semaphore(
            &mut reader,
            &mut writer,
            &client,
            &url,
            None,
            &mut out_log,
            Some(sem),
        )
        .await
        .unwrap();

        let log_str = String::from_utf8(out_log).unwrap();
        assert!(log_str.contains("  Translating 'Msg1' into 'de_DE' … Done -> Fertig\n"));
        assert!(log_str.contains("  Translating 'Msg2' into 'de_DE' … Done -> Fertig\n"));
    }

    #[tokio::test]
    async fn test_logging_concurrent_no_repetition() {
        let (url, _handle) = spawn_mock_server_concurrent(|_req| async {
            tokio::time::sleep(tokio::time::Duration::from_millis(50)).await;
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "Übersetzt"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>Con1</source><translation type="unfinished"></translation></message>
    <message><source>Con2</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let sem = Arc::new(Semaphore::new(2));
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        process_ts_with_semaphore(
            &mut reader,
            &mut writer,
            &client,
            &url,
            None,
            &mut out_log,
            Some(sem),
        )
        .await
        .unwrap();

        let log_str = String::from_utf8(out_log).unwrap();
        assert!(log_str.contains("  Translating 'Con1' into 'de_DE' …\n"));
        assert!(log_str.contains("  Translating 'Con2' into 'de_DE' …\n"));
        assert!(log_str.contains("  Done 'Con1' -> Übersetzt\n"));
        assert!(log_str.contains("  Done 'Con2' -> Übersetzt\n"));
    }

    #[tokio::test]
    async fn test_process_ts_numerusform_preserved_inside_translation() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "Einzeltext"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message>
        <source>NormalMsg</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location filename="../misc/htmlinfo.cpp" line="844"/>
        <source>%1 notification(s) available</source>
        <translation type="unfinished">
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        let results = process_ts(
            &mut reader,
            &mut writer,
            &client,
            &url,
            None,
            &mut out_log,
        )
        .await
        .unwrap();

        assert_eq!(results.translation_count, 1);
        assert_eq!(results.error_count, 0);

        let output_xml = String::from_utf8(out_buf).unwrap();

        // Normal message was translated
        assert!(output_xml.contains("<source>NormalMsg</source>\n        <translation>Einzeltext</translation>"));

        // Numerusform elements must remain inside their <translation> element, never moved outside
        assert!(!output_xml.contains("<numerusform></numerusform><numerusform></numerusform><translation"));
        assert!(output_xml.contains("<translation type=\"unfinished\">\n            <numerusform></numerusform>\n            <numerusform></numerusform>\n        </translation>"));
    }

    #[tokio::test]
    async fn test_process_ts_numerusform_without_numerus_attribute_preserved() {
        let (url, _handle) = spawn_mock_server(|_req| {
            (
                200,
                serde_json::json!({
                    "choices": [{
                        "message": {
                            "content": "Ignored"
                        }
                    }]
                })
                .to_string(),
            )
        })
        .await;

        let input_xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message>
        <source>%1 notification(s) available</source>
        <translation type="unfinished">
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
</context>
</TS>"#;

        let client = reqwest::Client::new();
        let mut reader = Reader::from_reader(Cursor::new(input_xml));
        let mut out_buf = Vec::new();
        let mut writer = Writer::new(&mut out_buf);
        let mut out_log = Vec::new();

        let results = process_ts(
            &mut reader,
            &mut writer,
            &client,
            &url,
            None,
            &mut out_log,
        )
        .await
        .unwrap();

        assert_eq!(results.translation_count, 0);

        let output_xml = String::from_utf8(out_buf).unwrap();
        assert!(!output_xml.contains("<numerusform></numerusform><numerusform></numerusform><translation"));
        assert!(output_xml.contains("<translation type=\"unfinished\">\n            <numerusform></numerusform>\n            <numerusform></numerusform>\n        </translation>"));
    }
}
