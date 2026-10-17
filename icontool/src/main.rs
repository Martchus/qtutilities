use icontool::run;
use std::io;

fn main() {
    std::process::exit(run(&mut io::stdout().lock(), &mut io::stdin().lock()));
}
