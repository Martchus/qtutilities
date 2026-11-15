mod translate;

pub use translate::{
    Results, format_error, process_ts, process_ts_file, process_ts_file_with_semaphore,
    process_ts_file_with_semaphore_and_cancel, process_ts_with_semaphore,
    process_ts_with_semaphore_and_cancel, translate_text,
};

use clap::Parser;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::Duration;

#[derive(Parser, Debug, Clone)]
#[command(
    author,
    version,
    about = "Translate Qt .ts files using a local llama.cpp server"
)]
pub struct Args {
    /// One or more Qt .ts files or directories to translate
    #[arg(required = true, num_args = 1..)]
    pub files: Vec<PathBuf>,

    /// llama.cpp API URL (or any OpenAI compatible endpoint)
    #[arg(
        long,
        short = 'u',
        default_value = "http://localhost:8080/v1/chat/completions"
    )]
    pub url: String,

    /// Model name to pass to the API (optional)
    #[arg(long, short = 'm')]
    pub model: Option<String>,

    /// Request timeout in seconds (0 to disable)
    #[arg(long, short = 't', default_value = "60")]
    pub timeout: u64,

    /// Maximum number of concurrent translation requests to the API
    #[arg(long, short = 'c', default_value = "1")]
    pub concurrency: usize,
}

/// Recursively searches a directory for `*.ts` files.
fn find_ts_files_in_dir(dir: &Path, files: &mut Vec<PathBuf>) -> std::io::Result<()> {
    for entry in std::fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_dir() {
            find_ts_files_in_dir(&path, files)?;
        } else if path.is_file()
            && path
                .extension()
                .and_then(|ext| ext.to_str())
                .is_some_and(|ext| ext.eq_ignore_ascii_case("ts"))
        {
            files.push(path);
        }
    }
    Ok(())
}

/// Expands directory paths into recursively discovered `*.ts` files, preserving non-directory paths.
pub fn collect_ts_files(paths: &[PathBuf]) -> std::io::Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    for path in paths {
        if path.is_dir() {
            let mut dir_files = Vec::new();
            find_ts_files_in_dir(path, &mut dir_files)?;
            dir_files.sort();
            files.extend(dir_files);
        } else {
            files.push(path.clone());
        }
    }
    Ok(files)
}

pub async fn run_with_args(output: &mut dyn Write, args: &Args) -> i32 {
    let (cancel_tx, cancel_rx) = tokio::sync::watch::channel(false);
    let cancel_tx_clone = cancel_tx.clone();
    tokio::spawn(async move {
        if tokio::signal::ctrl_c().await.is_ok() {
            let _ = cancel_tx_clone.send(true);
            if tokio::signal::ctrl_c().await.is_ok() {
                std::process::exit(130);
            }
        }
    });
    run_with_args_and_cancel(output, args, cancel_rx).await
}

pub async fn run_with_args_and_cancel(
    output: &mut dyn Write,
    args: &Args,
    mut cancel_rx: tokio::sync::watch::Receiver<bool>,
) -> i32 {
    if args.concurrency == 0 {
        let _ = writeln!(output, "Concurrency limit must be greater than 0");
        return 1;
    }

    let mut builder = reqwest::Client::builder();
    if args.timeout > 0 {
        builder = builder.timeout(Duration::from_secs(args.timeout));
    }
    let client = match builder.build() {
        Ok(c) => c,
        Err(e) => {
            let _ = writeln!(output, "Failed to build HTTP client: {e}");
            return 1;
        }
    };

    let files = match collect_ts_files(&args.files) {
        Ok(f) => f,
        Err(e) => {
            let _ = writeln!(output, "Failed to collect files: {e}");
            return 1;
        }
    };

    let semaphore = Arc::new(tokio::sync::Semaphore::new(args.concurrency));

    if args.concurrency == 1 {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel::<Vec<u8>>();
        struct SimpleWriter(tokio::sync::mpsc::UnboundedSender<Vec<u8>>);
        impl Write for SimpleWriter {
            fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
                let _ = self.0.send(buf.to_vec());
                Ok(buf.len())
            }
            fn flush(&mut self) -> std::io::Result<()> {
                Ok(())
            }
        }

        let mut has_error = false;
        let mut canceled = false;
        for file in &files {
            if *cancel_rx.borrow() {
                canceled = true;
                break;
            }
            let mut writer = SimpleWriter(tx.clone());
            let file_future = process_ts_file_with_semaphore_and_cancel(
                file,
                &client,
                &args.url,
                args.model.as_deref(),
                &mut writer,
                Some(semaphore.clone()),
                Some(cancel_rx.clone()),
            );
            tokio::pin!(file_future);

            let res = loop {
                tokio::select! {
                    _ = cancel_rx.changed(), if !canceled => {
                        if *cancel_rx.borrow() {
                            canceled = true;
                            let _ = writeln!(output, "\nWriting finished translations to disk …");
                            let _ = output.flush();
                        }
                    }
                    msg = rx.recv() => {
                        if let Some(data) = msg {
                            let _ = output.write_all(&data);
                            let _ = output.flush();
                        }
                    }
                    res = &mut file_future => {
                        break res;
                    }
                }
            };

            while let Ok(data) = rx.try_recv() {
                let _ = output.write_all(&data);
                let _ = output.flush();
            }

            if *cancel_rx.borrow() && !canceled {
                canceled = true;
                let _ = writeln!(output, "\nWriting finished translations to disk …");
                let _ = output.flush();
            }

            match res {
                Ok(results) => {
                    if results.error_count > 0 {
                        has_error = true;
                    }
                }
                Err(e) => {
                    let _ = writeln!(output, "Error processing {:?}: {e:#}", file);
                    let _ = output.flush();
                    has_error = true;
                }
            }

            if canceled || *cancel_rx.borrow() {
                canceled = true;
                break;
            }
        }
        if canceled {
            return 130;
        }
        return if has_error { 1 } else { 0 };
    }

    struct ChannelWriter {
        id: usize,
        tx: tokio::sync::mpsc::UnboundedSender<(usize, Vec<u8>)>,
    }

    impl Write for ChannelWriter {
        fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
            let _ = self.tx.send((self.id, buf.to_vec()));
            Ok(buf.len())
        }
        fn flush(&mut self) -> std::io::Result<()> {
            Ok(())
        }
    }

    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel::<(usize, Vec<u8>)>();
    let mut tasks = tokio::task::JoinSet::new();

    for (file_id, file) in files.into_iter().enumerate() {
        let client = client.clone();
        let url = args.url.clone();
        let model = args.model.clone();
        let sem = Some(semaphore.clone());
        let file_tx = tx.clone();
        let file_cancel = cancel_rx.clone();
        tasks.spawn(async move {
            let mut writer = ChannelWriter {
                id: file_id,
                tx: file_tx,
            };
            let res = process_ts_file_with_semaphore_and_cancel(
                &file,
                &client,
                &url,
                model.as_deref(),
                &mut writer,
                sem,
                Some(file_cancel),
            )
            .await;
            (file, res)
        });
    }
    drop(tx);

    let mut has_error = false;
    let mut canceled = false;
    let mut last_writer: Option<usize> = None;
    let mut last_ended_with_newline = true;

    loop {
        tokio::select! {
            _ = cancel_rx.changed(), if !canceled => {
                if *cancel_rx.borrow() {
                    canceled = true;
                    if !last_ended_with_newline {
                        let _ = output.write_all(b"\n");
                        last_ended_with_newline = true;
                    }
                    let _ = writeln!(output, "Writing finished translations to disk …");
                    let _ = output.flush();
                }
            }
            chunk = rx.recv() => {
                match chunk {
                    Some((id, data)) => {
                        if !data.is_empty() {
                            if last_writer.is_some_and(|prev| prev != id) && !last_ended_with_newline {
                                let _ = output.write_all(b"\n");
                            }
                            let _ = output.write_all(&data);
                            let _ = output.flush();
                            last_writer = Some(id);
                            last_ended_with_newline = data.ends_with(b"\n");
                        }
                    }
                    None => break,
                }
            }
            task_res = tasks.join_next(), if !tasks.is_empty() => {
                if let Some(task_res) = task_res {
                    match task_res {
                        Ok((_file, Ok(results))) => {
                            if results.error_count > 0 {
                                has_error = true;
                            }
                        }
                        Ok((file, Err(e))) => {
                            if !last_ended_with_newline {
                                let _ = output.write_all(b"\n");
                                last_ended_with_newline = true;
                            }
                            let _ = writeln!(output, "Error processing {:?}: {e:#}", file);
                            let _ = output.flush();
                            has_error = true;
                        }
                        Err(e) => {
                            if !last_ended_with_newline {
                                let _ = output.write_all(b"\n");
                                last_ended_with_newline = true;
                            }
                            let _ = writeln!(output, "Task join error: {e}");
                            let _ = output.flush();
                            has_error = true;
                        }
                    }
                }
            }
        }
    }

    while let Ok((id, data)) = rx.try_recv() {
        if !data.is_empty() {
            if last_writer.is_some_and(|prev| prev != id) && !last_ended_with_newline {
                let _ = output.write_all(b"\n");
            }
            let _ = output.write_all(&data);
            let _ = output.flush();
            last_writer = Some(id);
            last_ended_with_newline = data.ends_with(b"\n");
        }
    }

    while let Some(task_res) = tasks.join_next().await {
        match task_res {
            Ok((_file, Ok(results))) => {
                if results.error_count > 0 {
                    has_error = true;
                }
            }
            Ok((file, Err(e))) => {
                if !last_ended_with_newline {
                    let _ = output.write_all(b"\n");
                    last_ended_with_newline = true;
                }
                let _ = writeln!(output, "Error processing {:?}: {e:#}", file);
                let _ = output.flush();
                has_error = true;
            }
            Err(e) => {
                if !last_ended_with_newline {
                    let _ = output.write_all(b"\n");
                    last_ended_with_newline = true;
                }
                let _ = writeln!(output, "Task join error: {e}");
                let _ = output.flush();
                has_error = true;
            }
        }
    }

    if *cancel_rx.borrow() && !canceled {
        canceled = true;
        if !last_ended_with_newline {
            let _ = output.write_all(b"\n");
        }
        let _ = writeln!(output, "Writing finished translations to disk …");
        let _ = output.flush();
    }

    if canceled {
        return 130;
    }
    if has_error { 1 } else { 0 }
}

pub async fn run(output: &mut dyn Write) -> i32 {
    run_with_args(output, &Args::parse()).await
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_args_parsing() {
        let args = Args::try_parse_from([
            "translator",
            "file1.ts",
            "file2.ts",
            "--url",
            "http://example.com/v1",
            "--timeout",
            "45",
            "--model",
            "qwen",
            "--concurrency",
            "4",
        ])
        .unwrap();

        assert_eq!(
            args.files,
            vec![PathBuf::from("file1.ts"), PathBuf::from("file2.ts")]
        );
        assert_eq!(args.url, "http://example.com/v1");
        assert_eq!(args.timeout, 45);
        assert_eq!(args.model.as_deref(), Some("qwen"));
        assert_eq!(args.concurrency, 4);

        let args_short = Args::try_parse_from([
            "translator",
            "file1.ts",
            "-t",
            "60",
            "-u",
            "http://other:8080/v1",
            "-m",
            "mistral",
            "-c",
            "8",
        ])
        .unwrap();
        assert_eq!(args_short.timeout, 60);
        assert_eq!(args_short.url, "http://other:8080/v1");
        assert_eq!(args_short.concurrency, 8);
        let args_default = Args::try_parse_from(["translator", "file1.ts"]).unwrap();
        assert_eq!(args_default.timeout, 60);
        assert_eq!(
            args_default.url,
            "http://localhost:8080/v1/chat/completions"
        );
        assert_eq!(args_default.model, None);
        assert_eq!(args_default.concurrency, 1);
    }

    #[tokio::test]
    async fn test_run_with_args_nonexistent_file() {
        let mut output = Vec::new();
        let args = Args {
            files: vec![PathBuf::from("/nonexistent/file.ts")],
            url: "http://localhost:8080".to_string(),
            model: None,
            timeout: 5,
            concurrency: 1,
        };

        assert_eq!(run_with_args(&mut output, &args).await, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Error processing"));
    }

    #[test]
    fn test_collect_ts_files_recursive() {
        let temp_dir =
            std::env::temp_dir().join(format!("translator_collect_test_{}", std::process::id()));
        let sub_dir = temp_dir.join("sub");
        std::fs::create_dir_all(&sub_dir).unwrap();

        let file_root_ts = temp_dir.join("root.ts");
        let file_root_txt = temp_dir.join("root.txt");
        let file_sub_ts = sub_dir.join("nested.ts");
        let file_sub_other = sub_dir.join("nested.cpp");

        std::fs::write(&file_root_ts, "").unwrap();
        std::fs::write(&file_root_txt, "").unwrap();
        std::fs::write(&file_sub_ts, "").unwrap();
        std::fs::write(&file_sub_other, "").unwrap();

        let collected = collect_ts_files(&[temp_dir.clone()]).unwrap();
        assert_eq!(collected.len(), 2);
        assert!(collected.contains(&file_root_ts));
        assert!(collected.contains(&file_sub_ts));
        assert!(!collected.contains(&file_root_txt));
        assert!(!collected.contains(&file_sub_other));

        // Direct file path should be preserved as-is
        let direct_file = temp_dir.join("direct.ts");
        let collected_direct = collect_ts_files(&[direct_file.clone()]).unwrap();
        assert_eq!(collected_direct, vec![direct_file]);

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[tokio::test]
    async fn test_run_with_args_zero_concurrency() {
        let mut output = Vec::new();
        let args = Args {
            files: vec![PathBuf::from("dummy.ts")],
            url: "http://localhost:8080".to_string(),
            model: None,
            timeout: 5,
            concurrency: 0,
        };

        assert_eq!(run_with_args(&mut output, &args).await, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Concurrency limit must be greater than 0"));
    }

    #[tokio::test]
    async fn test_run_with_args_concurrent_multiple_files() {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        use tokio::net::TcpListener;

        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                tokio::spawn(async move {
                    let mut buf = vec![0u8; 8192];
                    let n = match socket.read(&mut buf).await {
                        Ok(n) if n > 0 => n,
                        _ => return,
                    };
                    let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                    let translated = if req_str.contains("FileOne") {
                        "DateiEins"
                    } else {
                        "DateiZwei"
                    };
                    let body = serde_json::json!({
                        "choices": [{
                            "message": {
                                "content": translated
                            }
                        }]
                    })
                    .to_string();
                    let resp = format!(
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = socket.write_all(resp.as_bytes()).await;
                    let _ = socket.flush().await;
                });
            }
        });

        let temp_dir =
            std::env::temp_dir().join(format!("translator_multi_test_{}", std::process::id()));
        std::fs::create_dir_all(&temp_dir).unwrap();
        let file1 = temp_dir.join("file1_de_DE.ts");
        let file2 = temp_dir.join("file2_de_DE.ts");

        let xml1 = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>FileOne</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;
        let xml2 = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>FileTwo</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;
        std::fs::write(&file1, xml1).unwrap();
        std::fs::write(&file2, xml2).unwrap();

        let mut output = Vec::new();
        let args = Args {
            files: vec![temp_dir.clone()],
            url,
            model: None,
            timeout: 5,
            concurrency: 2,
        };

        assert_eq!(run_with_args(&mut output, &args).await, 0);

        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Translating 'FileOne' into 'de_DE' …"));
        assert!(out_str.contains("Translating 'FileTwo' into 'de_DE' …"));
        assert!(out_str.contains("DateiEins"));
        assert!(out_str.contains("DateiZwei"));

        let content1 = std::fs::read_to_string(&file1).unwrap();
        assert!(content1.contains("<translation>DateiEins</translation>"));
        let content2 = std::fs::read_to_string(&file2).unwrap();
        assert!(content2.contains("<translation>DateiZwei</translation>"));

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[tokio::test]
    async fn test_run_with_args_sequential_logging() {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        use tokio::net::TcpListener;

        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                tokio::spawn(async move {
                    let mut buf = vec![0u8; 8192];
                    let n = match socket.read(&mut buf).await {
                        Ok(n) if n > 0 => n,
                        _ => return,
                    };
                    let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                    let translated = if req_str.contains("SingleFile") {
                        "EinzelDatei"
                    } else {
                        "Unbekannt"
                    };
                    let body = serde_json::json!({
                        "choices": [{
                            "message": {
                                "content": translated
                            }
                        }]
                    })
                    .to_string();
                    let resp = format!(
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = socket.write_all(resp.as_bytes()).await;
                    let _ = socket.flush().await;
                });
            }
        });

        let temp_dir =
            std::env::temp_dir().join(format!("translator_seq_test_{}", std::process::id()));
        std::fs::create_dir_all(&temp_dir).unwrap();
        let file = temp_dir.join("single_de_DE.ts");

        let xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>SingleFile</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;
        std::fs::write(&file, xml).unwrap();

        let mut output = Vec::new();
        let args = Args {
            files: vec![file.clone()],
            url,
            model: None,
            timeout: 5,
            concurrency: 1,
        };

        assert_eq!(run_with_args(&mut output, &args).await, 0);

        let out_str = String::from_utf8(output).unwrap();
        assert!(
            out_str.contains("  Translating 'SingleFile' into 'de_DE' … Done -> 'EinzelDatei'\n")
        );

        let content = std::fs::read_to_string(&file).unwrap();
        assert!(content.contains("<translation>EinzelDatei</translation>"));

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[tokio::test]
    async fn test_cancellation_writes_partial_translations() {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        use tokio::net::TcpListener;

        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        let (cancel_tx, cancel_rx) = tokio::sync::watch::channel(false);
        let cancel_tx_clone = cancel_tx.clone();

        tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                let cancel_tx = cancel_tx_clone.clone();
                tokio::spawn(async move {
                    let mut buf = vec![0u8; 8192];
                    let n = match socket.read(&mut buf).await {
                        Ok(n) if n > 0 => n,
                        _ => return,
                    };
                    let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                    let translated = if req_str.contains("FirstMsg") {
                        "ErsteNachricht"
                    } else {
                        // FirstMsg is already completely finished. Now cancel during SecondMsg!
                        let _ = cancel_tx.send(true);
                        tokio::time::sleep(tokio::time::Duration::from_millis(1000)).await;
                        "ZweiteNachricht"
                    };
                    let body = serde_json::json!({
                        "choices": [{
                            "message": {
                                "content": translated
                            }
                        }]
                    })
                    .to_string();
                    let resp = format!(
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = socket.write_all(resp.as_bytes()).await;
                    let _ = socket.flush().await;
                });
            }
        });

        let temp_dir =
            std::env::temp_dir().join(format!("translator_cancel_test_{}", std::process::id()));
        std::fs::create_dir_all(&temp_dir).unwrap();
        let file = temp_dir.join("partial_de_DE.ts");

        let xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>FirstMsg</source><translation type="unfinished"></translation></message>
    <message><source>SecondMsg</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;
        std::fs::write(&file, xml).unwrap();

        let mut output = Vec::new();
        let args = Args {
            files: vec![file.clone()],
            url,
            model: None,
            timeout: 5,
            concurrency: 1,
        };

        let exit_code = run_with_args_and_cancel(&mut output, &args, cancel_rx).await;
        assert_eq!(exit_code, 130);

        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Writing finished translations to disk …"));

        let content = std::fs::read_to_string(&file).unwrap();
        // FirstMsg was finished before cancellation -> saved
        assert!(content.contains("<translation>ErsteNachricht</translation>"));
        // SecondMsg was canceled -> remains unfinished
        assert!(content.contains(
            r#"<source>SecondMsg</source><translation type="unfinished"></translation>"#
        ));

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[tokio::test]
    async fn test_cancellation_concurrent_writes_partial_translations() {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        use tokio::net::TcpListener;

        let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let url = format!("http://127.0.0.1:{port}/v1/chat/completions");

        let (cancel_tx, cancel_rx) = tokio::sync::watch::channel(false);
        let cancel_tx_clone = cancel_tx.clone();

        let first_done = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
        let first_done_clone = first_done.clone();

        tokio::spawn(async move {
            while let Ok((mut socket, _)) = listener.accept().await {
                let cancel_tx = cancel_tx_clone.clone();
                let first_done = first_done_clone.clone();
                tokio::spawn(async move {
                    let mut buf = vec![0u8; 8192];
                    let n = match socket.read(&mut buf).await {
                        Ok(n) if n > 0 => n,
                        _ => return,
                    };
                    let req_str = String::from_utf8_lossy(&buf[..n]).to_string();
                    let translated = if req_str.contains("FirstMsg") {
                        "ErsteNachricht"
                    } else {
                        // Wait until FirstMsg has responded, then cancel
                        while !first_done.load(std::sync::atomic::Ordering::SeqCst) {
                            tokio::time::sleep(tokio::time::Duration::from_millis(5)).await;
                        }
                        tokio::time::sleep(tokio::time::Duration::from_millis(20)).await;
                        let _ = cancel_tx.send(true);
                        tokio::time::sleep(tokio::time::Duration::from_millis(1000)).await;
                        "ZweiteNachricht"
                    };
                    let body = serde_json::json!({
                        "choices": [{
                            "message": {
                                "content": translated
                            }
                        }]
                    })
                    .to_string();
                    let resp = format!(
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                        body.len()
                    );
                    let _ = socket.write_all(resp.as_bytes()).await;
                    let _ = socket.flush().await;
                    if req_str.contains("FirstMsg") {
                        first_done.store(true, std::sync::atomic::Ordering::SeqCst);
                    }
                });
            }
        });

        let temp_dir =
            std::env::temp_dir().join(format!("translator_cancel_multi_{}", std::process::id()));
        std::fs::create_dir_all(&temp_dir).unwrap();
        let file = temp_dir.join("partial_multi_de_DE.ts");

        let xml = r#"<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="de_DE">
<context>
    <message><source>FirstMsg</source><translation type="unfinished"></translation></message>
    <message><source>SecondMsg</source><translation type="unfinished"></translation></message>
</context>
</TS>"#;
        std::fs::write(&file, xml).unwrap();

        let mut output = Vec::new();
        let args = Args {
            files: vec![file.clone()],
            url,
            model: None,
            timeout: 5,
            concurrency: 2,
        };

        let exit_code = run_with_args_and_cancel(&mut output, &args, cancel_rx).await;
        assert_eq!(exit_code, 130);

        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Writing finished translations to disk …"));

        let content = std::fs::read_to_string(&file).unwrap();
        assert!(content.contains("<translation>ErsteNachricht</translation>"));
        assert!(content.contains(
            r#"<source>SecondMsg</source><translation type="unfinished"></translation>"#
        ));

        let _ = std::fs::remove_dir_all(&temp_dir);
    }
}
