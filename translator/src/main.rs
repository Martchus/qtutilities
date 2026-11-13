use std::io;
use translator::run;

#[tokio::main]
async fn main() {
    std::process::exit(run(&mut io::stdout().lock()).await);
}
