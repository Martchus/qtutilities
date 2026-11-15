use std::io;
use translator::run;

#[tokio::main(flavor = "current_thread")]
async fn main() {
    std::process::exit(run(&mut io::stdout().lock()).await);
}
