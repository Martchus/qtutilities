use anyhow::{Result, anyhow};
use quick_xml::events::{BytesEnd, BytesStart, BytesText, Event};
use quick_xml::{Reader, Writer};
use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;

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
        Some(text) if text.is_empty() => {
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

async fn handle_unfinished_translation<W: Write>(
    writer: &mut Writer<W>,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut dyn Write,
    current_source: &str,
    language: &str,
    attributes: &[(String, String)],
    fallback_text: Option<&str>,
    results: &mut Results,
) -> Result<()> {
    let translation_result = if !current_source.is_empty() {
        let _ = write!(
            output,
            "  Translating '{current_source}' into '{language}' …"
        );
        let _ = output.flush();
        Some(translate_text(client, api_url, current_source, language, model).await)
    } else {
        None
    };

    match translation_result {
        Some(Ok(translated)) => {
            let _ = writeln!(output, " Done -> {translated}");
            let mut start = BytesStart::new("translation");
            for (k, v) in attributes {
                start.push_attribute((k.as_str(), v.as_str()));
            }
            writer.write_event(Event::Start(start))?;
            let escaped = quick_xml::escape::escape(&translated);
            writer.write_event(Event::Text(BytesText::from_escaped(escaped)))?;
            writer.write_event(Event::End(BytesEnd::new("translation")))?;
            results.translation_count += 1;
        }
        Some(Err(err)) => {
            let _ = writeln!(output, " Failed: {}", format_error(&err));
            write_unfinished_translation(writer, attributes, fallback_text)?;
            results.error_count += 1;
        }
        None => {
            write_unfinished_translation(writer, attributes, fallback_text)?;
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

/// Processes TS XML from a reader and writes the modified XML to a writer.
pub async fn process_ts<R: BufRead, W: Write>(
    reader: &mut Reader<R>,
    writer: &mut Writer<W>,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut dyn Write,
) -> Result<Results> {
    let mut buf = Vec::new();
    let mut language = String::new();
    let mut results = Results {
        translation_count: 0,
        error_count: 0,
    };

    let mut inside_source = false;
    let mut current_source = String::new();

    let mut inside_unfinished_translation = false;
    let mut current_translation_attributes: Vec<(String, String)> = Vec::new();
    let mut unfinished_translation_text = String::new();

    loop {
        match reader.read_event_into(&mut buf)? {
            Event::Eof => break,

            Event::Start(e) => {
                let name = e.name();
                if name.as_ref() == b"TS" {
                    for attr in e.attributes().filter_map(|a| a.ok()) {
                        if attr.key.as_ref() == b"language" {
                            language = attr.unescape_value()?.to_string();
                        }
                    }
                    writer.write_event(Event::Start(e))?;
                } else if name.as_ref() == b"source" {
                    inside_source = true;
                    current_source.clear();
                    writer.write_event(Event::Start(e))?;
                } else if name.as_ref() == b"translation" {
                    let (is_unfinished, attrs) = parse_translation_attributes(&e)?;
                    current_translation_attributes = attrs;
                    unfinished_translation_text.clear();

                    if is_unfinished && !language.starts_with("en") {
                        inside_unfinished_translation = true;
                    } else {
                        writer.write_event(Event::Start(e))?;
                    }
                } else {
                    writer.write_event(Event::Start(e))?;
                }
            }

            Event::Empty(e) => {
                let name = e.name();
                if name.as_ref() == b"translation" {
                    let (is_unfinished, current_attrs) = parse_translation_attributes(&e)?;

                    if is_unfinished && !language.starts_with("en") {
                        if language.is_empty() {
                            return Err(anyhow!(
                                "Could not auto-determine the 'language' attribute from <TS> node before hitting a translation!"
                            ));
                        }

                        handle_unfinished_translation(
                            writer,
                            client,
                            api_url,
                            model,
                            output,
                            &current_source,
                            &language,
                            &current_attrs,
                            None,
                            &mut results,
                        )
                        .await?;
                    } else {
                        writer.write_event(Event::Empty(e))?;
                    }
                } else {
                    writer.write_event(Event::Empty(e))?;
                }
            }

            Event::Text(e) => {
                if inside_unfinished_translation {
                    unfinished_translation_text.push_str(&e.unescape()?);
                } else {
                    if inside_source {
                        current_source.push_str(&e.unescape()?);
                    }
                    writer.write_event(Event::Text(e))?;
                }
            }

            Event::CData(e) => {
                if inside_unfinished_translation {
                    unfinished_translation_text.push_str(&String::from_utf8_lossy(e.as_ref()));
                } else {
                    if inside_source {
                        current_source.push_str(&String::from_utf8_lossy(e.as_ref()));
                    }
                    writer.write_event(Event::CData(e))?;
                }
            }

            Event::End(e) => {
                let name = e.name();
                if name.as_ref() == b"source" {
                    inside_source = false;
                    writer.write_event(Event::End(e))?;
                } else if name.as_ref() == b"translation" && inside_unfinished_translation {
                    if language.is_empty() {
                        return Err(anyhow!(
                            "Could not auto-determine the 'language' attribute from <TS> node before hitting a translation!"
                        ));
                    }

                    handle_unfinished_translation(
                        writer,
                        client,
                        api_url,
                        model,
                        output,
                        &current_source,
                        &language,
                        &current_translation_attributes,
                        Some(&unfinished_translation_text),
                        &mut results,
                    )
                    .await?;

                    inside_unfinished_translation = false;
                } else {
                    if name.as_ref() == b"message" {
                        current_source.clear();
                    }
                    writer.write_event(Event::End(e))?;
                }
            }

            other => {
                writer.write_event(other)?;
            }
        }
        buf.clear();
    }

    Ok(results)
}

/// Parses and modifies a Qt TS file.
pub async fn process_ts_file(
    file_path: &Path,
    client: &reqwest::Client,
    api_url: &str,
    model: Option<&str>,
    output: &mut dyn Write,
) -> Result<Results> {
    let _ = writeln!(output, "Processing: {}", file_path.display());

    let file = File::open(file_path)?;
    let mut reader = Reader::from_reader(BufReader::new(file));

    let mut out_data = Vec::new();
    let mut writer = Writer::new(&mut out_data);

    let results = process_ts(&mut reader, &mut writer, client, api_url, model, output).await?;

    if results.translation_count > 0 {
        let message = if results.error_count == 0 {
            format!("All unfinished translations have been translated.")
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
}
