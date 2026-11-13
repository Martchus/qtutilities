mod translate;

pub use translate::{Results, format_error, process_ts, process_ts_file, translate_text};

use clap::Parser;
use std::io::Write;
use std::path::PathBuf;
use std::time::Duration;

#[derive(Parser, Debug, Clone)]
#[command(
    author,
    version,
    about = "Translate Qt .ts files using a local llama.cpp server"
)]
pub struct Args {
    /// One or more Qt .ts files to translate
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
}

pub async fn run_with_args(output: &mut dyn Write, args: &Args) -> i32 {
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

    let mut has_error = false;
    for file in &args.files {
        match process_ts_file(file, &client, &args.url, args.model.as_deref(), output).await {
            Ok(results) => {
                if results.error_count > 0 {
                    has_error = true;
                }
            }
            Err(e) => {
                let _ = writeln!(output, "Error processing {:?}: {e:#}", file);
                has_error = true;
            }
        }
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
        ])
        .unwrap();

        assert_eq!(
            args.files,
            vec![PathBuf::from("file1.ts"), PathBuf::from("file2.ts")]
        );
        assert_eq!(args.url, "http://example.com/v1");
        assert_eq!(args.timeout, 45);
        assert_eq!(args.model.as_deref(), Some("qwen"));

        let args_short = Args::try_parse_from([
            "translator",
            "file1.ts",
            "-t",
            "60",
            "-u",
            "http://other:8080/v1",
            "-m",
            "mistral",
        ])
        .unwrap();
        assert_eq!(args_short.timeout, 60);
        assert_eq!(args_short.url, "http://other:8080/v1");
        let args_default = Args::try_parse_from(["translator", "file1.ts"]).unwrap();
        assert_eq!(args_default.timeout, 60);
        assert_eq!(
            args_default.url,
            "http://localhost:8080/v1/chat/completions"
        );
        assert_eq!(args_default.model, None);
    }

    #[tokio::test]
    async fn test_run_with_args_nonexistent_file() {
        let mut output = Vec::new();
        let args = Args {
            files: vec![PathBuf::from("/nonexistent/file.ts")],
            url: "http://localhost:8080".to_string(),
            model: None,
            timeout: 5,
        };

        assert_eq!(run_with_args(&mut output, &args).await, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("Error processing"));
    }
}
