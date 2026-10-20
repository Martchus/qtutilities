mod bundle;
mod find;

use clap::{Parser, Subcommand};
use std::io::{BufRead, Write};
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(author, version, about = "Tool that helps with bundling icons", long_about = None)]
pub struct Cli {
    #[arg(long, default_value_t = false, help = "Prints formatted output")]
    pub pretty: bool,
    #[command(subcommand)]
    pub command: Option<Commands>,
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    #[command(
        about = "Prepare Qt resources for bundling the specified icon themes",
        long_about = None
    )]
    Bundle {
        #[arg(long, short = 'o', help = "Output path", default_value = "icons")]
        output_path: PathBuf,
        #[arg(
            long,
            short = 'q',
            help = "Path of output qrc file (relative to --output-path)",
            default_value = "builtin_builtinicons.qrc"
        )]
        output_qrc_path: PathBuf,
        #[arg(
            long = "search-path",
            short = 's',
            help = "Paths to locate icon themes",
            default_value = "/usr/share/icons",
            num_args = 1..
        )]
        search_paths: Vec<PathBuf>,
        #[arg(
            long = "icon-theme",
            short = 't',
            help = "Icon themes to bundle",
            default_value = "breeze",
            num_args = 1..
        )]
        icon_themes: Vec<PathBuf>,
        #[arg(
            long = "icon",
            alias = "icon-name",
            short = 'i',
            help = "Icons to bundle",
            num_args = 1..
        )]
        icons: Vec<PathBuf>,
        #[arg(
            long = "fallback-theme",
            default_value = "hicolor",
            help = "Fallback icon theme"
        )]
        fallback_theme: PathBuf,
        #[arg(
            long = "error-on-missing-icon",
            default_value_t = false,
            help = "Fail if an icon cannot be located"
        )]
        error_on_missing_icon: bool,
    },
    #[command(
        about = "Find icons used in source files (.cpp, .ui, .qml, etc.)",
        long_about = None
    )]
    Find {
        #[arg(
            help = "Directories to search for source files",
            default_value = ".",
            num_args = 0..
        )]
        directories: Vec<PathBuf>,
        #[arg(
            long = "with-locations",
            default_value_t = false,
            help = "Prints file locations where icons were found"
        )]
        with_locations: bool,
    },
}

pub fn run_with_args(output: &mut dyn Write, input: &mut dyn BufRead, args: &Cli) -> i32 {
    match &args.command {
        Some(Commands::Bundle {
            output_path,
            output_qrc_path,
            search_paths,
            icon_themes,
            icons,
            fallback_theme,
            error_on_missing_icon,
        }) => bundle::bundle_themes(
            output,
            input,
            args.pretty,
            output_path,
            output_qrc_path,
            search_paths,
            icon_themes,
            icons,
            fallback_theme,
            *error_on_missing_icon,
        ),
        Some(Commands::Find {
            directories,
            with_locations,
        }) => find::find_icons(output, args.pretty, directories, *with_locations),
        None => {
            writeln!(output, "No operation specified").unwrap();
            1
        }
    }
}

pub fn run(output: &mut dyn Write, input: &mut dyn BufRead) -> i32 {
    run_with_args(output, input, &Cli::parse())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn test_run_with_args() {
        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());

        let cli = Cli {
            pretty: false,
            command: None,
        };

        assert_eq!(run_with_args(&mut output, &mut input, &cli), 1);
    }
}
