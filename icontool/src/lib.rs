use clap::{Parser, Subcommand};
use std::collections::HashSet;
use std::fs;
use std::io::{self, BufRead, Write};
use std::path::{Component, Path, PathBuf};

/// Entry representing a file in the Qt resource (.qrc) file.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct QrcEntry {
    pub alias: Option<PathBuf>,
    pub path: PathBuf,
}

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
    #[command(about = "Prepare Qt resources for bundling the specified icon themes", long_about = None)]
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
    },
}

/// Parses a theme specification, which can be `theme_name` or `source_theme:new_theme_name`.
pub fn parse_theme_spec(theme_spec: &Path) -> (PathBuf, PathBuf) {
    let bytes = theme_spec.as_os_str().as_encoded_bytes();
    if let Some(pos) = bytes.iter().position(|&b| b == b':') {
        #[cfg(windows)]
        let is_drive_letter =
            pos == 1 && (bytes.get(2) == Some(&b'\\') || bytes.get(2) == Some(&b'/'));
        #[cfg(not(windows))]
        let is_drive_letter = false;

        if !is_drive_letter {
            #[cfg(unix)]
            {
                use std::ffi::OsStr;
                use std::os::unix::ffi::OsStrExt;
                let src = OsStr::from_bytes(&bytes[..pos]);
                let dst = OsStr::from_bytes(&bytes[pos + 1..]);
                return (PathBuf::from(src), PathBuf::from(dst));
            }
            #[cfg(not(unix))]
            {
                if let Some(s) = theme_spec.to_str() {
                    if let Some((src, dst)) = s.split_once(':') {
                        return (PathBuf::from(src), PathBuf::from(dst));
                    }
                }
            }
        }
    }
    (theme_spec.to_path_buf(), theme_spec.to_path_buf())
}

/// Checks whether a file name matches a required icon name according to CMake's regex: `^${REQUIRED_ICON}(\\..+)?$`.
pub fn icon_matches(file_name: &Path, icon_name: &Path) -> bool {
    let file_bytes = file_name.as_os_str().as_encoded_bytes();
    let icon_bytes = icon_name.as_os_str().as_encoded_bytes();
    if icon_bytes.is_empty() {
        return false;
    }
    if file_bytes == icon_bytes {
        return true;
    }
    if file_bytes.starts_with(icon_bytes) {
        let rest = &file_bytes[icon_bytes.len()..];
        if rest.starts_with(b".") && rest.len() > 1 {
            return true;
        }
    }
    false
}

/// Computes fallback icon names by iteratively stripping the last `-suffix`.
pub fn get_fallback_icon_names(icon_name: &Path) -> Vec<PathBuf> {
    let mut fallbacks = Vec::new();
    let mut current_bytes = icon_name.as_os_str().as_encoded_bytes().to_vec();
    while let Some(pos) = current_bytes.iter().rposition(|&b| b == b'-') {
        current_bytes.truncate(pos);
        if current_bytes.is_empty() {
            break;
        }
        #[cfg(unix)]
        {
            use std::ffi::OsStr;
            use std::os::unix::ffi::OsStrExt;
            fallbacks.push(PathBuf::from(OsStr::from_bytes(&current_bytes)));
        }
        #[cfg(not(unix))]
        {
            if let Ok(s) = std::str::from_utf8(&current_bytes) {
                fallbacks.push(PathBuf::from(s));
            }
        }
    }
    fallbacks
}

/// Normalizes path components without touching filesystem (resolves `.` and `..`).
pub fn normalize_path(path: &Path) -> PathBuf {
    let mut components = Vec::new();
    for comp in path.components() {
        match comp {
            Component::CurDir => {}
            Component::ParentDir => {
                if let Some(Component::Normal(_)) = components.last() {
                    components.pop();
                } else {
                    components.push(comp);
                }
            }
            _ => components.push(comp),
        }
    }
    components.iter().collect()
}

/// Resolves symlinks iteratively, following relative and absolute links while detecting cycles.
pub fn resolve_symlink(mut path: PathBuf) -> io::Result<PathBuf> {
    let mut visited = HashSet::new();
    while path.is_symlink() {
        if !visited.insert(path.clone()) {
            return Err(io::Error::other("Symlink cycle detected"));
        }
        let target = fs::read_link(&path)?;
        if target.is_absolute() {
            path = target;
        } else {
            path = path.parent().unwrap_or_else(|| Path::new("")).join(target);
        }
        path = normalize_path(&path);
    }
    Ok(normalize_path(&path))
}

/// Replaces the first path component of `rel_path` with `new_theme_name`.
pub fn replace_theme_prefix(rel_path: &Path, new_theme_name: &Path) -> PathBuf {
    let mut components = rel_path.components();
    let mut result = PathBuf::from(new_theme_name);
    components.next(); // discard original first component
    for comp in components {
        result.push(comp.as_os_str());
    }
    result
}

/// Converts a path to a Qt resource path (using forward slashes `/`).
pub fn path_to_qrc_path(path: &Path) -> String {
    let mut result = String::new();
    let mut first = true;
    for component in path.components() {
        if !first {
            result.push('/');
        }
        first = false;
        result.push_str(&component.as_os_str().to_string_lossy());
    }
    result
}

/// Escapes standard XML characters.
pub fn escape_xml(s: &str) -> String {
    let mut result = String::with_capacity(s.len());
    for c in s.chars() {
        match c {
            '&' => result.push_str("&amp;"),
            '<' => result.push_str("&lt;"),
            '>' => result.push_str("&gt;"),
            '"' => result.push_str("&quot;"),
            '\'' => result.push_str("&apos;"),
            _ => result.push(c),
        }
    }
    result
}

/// Generates the XML content for a Qt resource file.
pub fn generate_qrc_content(entries: &[QrcEntry], pretty: bool) -> String {
    let (prefix, suffix, indent, newline) = if pretty {
        (
            "<RCC>\n    <qresource prefix=\"/icons\">\n",
            "    </qresource>\n</RCC>\n",
            "        ",
            "\n",
        )
    } else {
        (
            "<RCC><qresource prefix=\"/icons\">",
            "</qresource></RCC>",
            "",
            "",
        )
    };

    let mut xml = String::from(prefix);
    for entry in entries {
        let path_str = escape_xml(&path_to_qrc_path(&entry.path));
        let alias_attr = match &entry.alias {
            Some(alias) => format!(" alias=\"{}\"", escape_xml(&path_to_qrc_path(alias))),
            None => String::new(),
        };
        xml.push_str(&format!(
            "{indent}<file{alias_attr}>{path_str}</file>{newline}"
        ));
    }
    xml.push_str(suffix);
    xml
}

/// Locates a theme directory within the given search paths.
pub fn find_theme_dir(source_theme: &Path, search_paths: &[PathBuf]) -> Option<(PathBuf, PathBuf)> {
    for search_path in search_paths {
        let theme_path = search_path.join(source_theme);
        if theme_path.is_dir() {
            return Some((theme_path, search_path.clone()));
        }
    }
    None
}

/// Finds index files (index.theme, and icon-theme.cache if not fallback theme) in a theme dir.
pub fn find_index_files(theme_dir: &Path, is_fallback_theme: bool) -> Vec<PathBuf> {
    let mut index_files = Vec::new();
    let index_theme = theme_dir.join("index.theme");
    if index_theme.is_file() {
        index_files.push(index_theme);
    }
    if !is_fallback_theme {
        let icon_cache = theme_dir.join("icon-theme.cache");
        if icon_cache.is_file() {
            index_files.push(icon_cache);
        }
    }
    index_files
}

/// Recursively collects all files and symlinks to files in a directory (ignoring directory symlinks).
pub fn collect_files(dir: &Path) -> io::Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    collect_files_recursive(dir, &mut files)?;
    Ok(files)
}

fn collect_files_recursive(dir: &Path, files: &mut Vec<PathBuf>) -> io::Result<()> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let file_type = entry.file_type()?;
        if file_type.is_dir() {
            collect_files_recursive(&entry.path(), files)?;
        } else if file_type.is_file() {
            files.push(entry.path());
        } else if file_type.is_symlink()
            && let Ok(metadata) = fs::metadata(entry.path())
            && metadata.is_file()
        {
            files.push(entry.path());
        }
    }
    Ok(())
}

/// Finds all matching icons for required icons in a theme, applying fallbacks if not found.
pub fn find_required_icons(
    all_theme_files: &[PathBuf],
    required_icons: &[PathBuf],
) -> Vec<PathBuf> {
    let mut matched_files = Vec::new();
    let mut seen_paths = HashSet::new();

    for req_icon in required_icons {
        let mut found_for_icon = false;
        for file in all_theme_files {
            if let Some(file_name) = file.file_name()
                && icon_matches(Path::new(file_name), req_icon)
            {
                found_for_icon = true;
                if seen_paths.insert(file.clone()) {
                    matched_files.push(file.clone());
                }
            }
        }

        if !found_for_icon {
            let fallbacks = get_fallback_icon_names(req_icon);
            for fallback in fallbacks {
                let mut fallback_matched = false;
                for file in all_theme_files {
                    if let Some(file_name) = file.file_name()
                        && icon_matches(Path::new(file_name), &fallback)
                    {
                        fallback_matched = true;
                        if seen_paths.insert(file.clone()) {
                            matched_files.push(file.clone());
                        }
                    }
                }
                if fallback_matched {
                    break;
                }
            }
        }
    }
    matched_files
}

/// Processes a single file (index file or icon file) for bundling into output_path and generates a QrcEntry.
pub fn process_and_copy_file(
    file_path: &Path,
    search_path: &Path,
    search_paths: &[PathBuf],
    theme_dir: &Path,
    new_theme_name: &Path,
    output_path: &Path,
) -> io::Result<Option<QrcEntry>> {
    if !file_path.exists() {
        return Ok(None);
    }

    let is_symlink = file_path.is_symlink();

    // Determine the relative path of file_path from search_path
    let rel_from_search = match file_path.strip_prefix(search_path) {
        Ok(rel) => replace_theme_prefix(rel, new_theme_name),
        Err(_) => match file_path.strip_prefix(theme_dir) {
            Ok(rel) => new_theme_name.join(rel),
            Err(_) => new_theme_name.join(file_path.file_name().unwrap_or_default()),
        },
    };

    let (alias, resolved_path, target_rel) = if is_symlink {
        let alias = Some(rel_from_search);
        let resolved = resolve_symlink(file_path.to_path_buf())?;
        if !resolved.exists() {
            return Ok(None);
        }

        let target_rel = match resolved.strip_prefix(search_path) {
            Ok(rel) => replace_theme_prefix(rel, new_theme_name),
            Err(_) => {
                let mut found_rel = None;
                for sp in search_paths {
                    if let Ok(rel) = resolved.strip_prefix(sp) {
                        found_rel = Some(replace_theme_prefix(rel, new_theme_name));
                        break;
                    }
                }
                match found_rel {
                    Some(r) => r,
                    None => match resolved.strip_prefix(theme_dir) {
                        Ok(rel) => new_theme_name.join(rel),
                        Err(_) => new_theme_name.join(resolved.file_name().unwrap_or_default()),
                    },
                }
            }
        };

        (alias, resolved, target_rel)
    } else {
        (None, file_path.to_path_buf(), rel_from_search)
    };

    let dest_file = output_path.join(&target_rel);
    if !dest_file.exists() {
        if let Some(parent) = dest_file.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::copy(&resolved_path, &dest_file)?;
    }

    Ok(Some(QrcEntry {
        alias,
        path: target_rel,
    }))
}

#[allow(clippy::too_many_arguments)]
fn process_theme(
    output: &mut dyn Write,
    source_theme: &Path,
    new_theme_name: &Path,
    search_paths: &[PathBuf],
    icons: &[PathBuf],
    fallback_theme: &Path,
    output_path: &Path,
    has_default: &mut bool,
    has_default_dark: &mut bool,
    qrc_entries: &mut Vec<QrcEntry>,
    seen_qrc_entries: &mut HashSet<QrcEntry>,
) -> Result<(), String> {
    let (theme_dir, search_path) = match find_theme_dir(source_theme, search_paths) {
        Some(pair) => pair,
        None => {
            return Err(format!(
                "The specified icon theme \"{}\" could not be found.",
                source_theme.display()
            ));
        }
    };
    writeln!(
        output,
        "The specified icon theme \"{}\" has been located under \"{}\" and will be built-in.",
        source_theme.display(),
        theme_dir.display()
    )
    .map_err(|e| e.to_string())?;

    let is_fallback = source_theme == fallback_theme;
    let index_files = find_index_files(&theme_dir, is_fallback);

    let is_dark = new_theme_name.to_string_lossy().ends_with("-dark");
    if is_dark && !*has_default_dark {
        let dir = output_path.join("default-dark");
        fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
        fs::write(
            dir.join("index.theme"),
            format!(
                "[Icon Theme]\nInherits={}\n",
                new_theme_name.to_string_lossy()
            ),
        )
        .map_err(|e| e.to_string())?;
        *has_default_dark = true;
        let entry = QrcEntry {
            alias: None,
            path: PathBuf::from("default-dark/index.theme"),
        };
        if seen_qrc_entries.insert(entry.clone()) {
            qrc_entries.push(entry);
        }
    } else if !*has_default {
        let dir = output_path.join("default");
        fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
        fs::write(
            dir.join("index.theme"),
            format!(
                "[Icon Theme]\nInherits={}\n",
                new_theme_name.to_string_lossy()
            ),
        )
        .map_err(|e| e.to_string())?;
        *has_default = true;
        let entry = QrcEntry {
            alias: None,
            path: PathBuf::from("default/index.theme"),
        };
        if seen_qrc_entries.insert(entry.clone()) {
            qrc_entries.push(entry);
        }
    }

    let matched_icon_files = if !is_fallback {
        let all_theme_files = collect_files(&theme_dir).map_err(|e| e.to_string())?;
        find_required_icons(&all_theme_files, icons)
    } else {
        writeln!(
            output,
            "Icon files for specified theme \"{}\" are skipped because these are provided as fallback anyways.",
            source_theme.display()
        )
        .map_err(|e| e.to_string())?;
        Vec::new()
    };

    for file_path in index_files.iter().chain(matched_icon_files.iter()) {
        if let Some(entry) = process_and_copy_file(
            file_path,
            &search_path,
            search_paths,
            &theme_dir,
            new_theme_name,
            output_path,
        )
        .map_err(|e| e.to_string())?
            && seen_qrc_entries.insert(entry.clone())
        {
            qrc_entries.push(entry);
        }
    }

    Ok(())
}

#[allow(clippy::too_many_arguments)]
pub fn bundle_themes(
    output: &mut dyn Write,
    _: &mut dyn BufRead,
    pretty: bool,
    output_path: &Path,
    output_qrc_path: &Path,
    search_paths: &[PathBuf],
    icon_themes: &[PathBuf],
    icons: &[PathBuf],
    fallback_theme: &Path,
) -> i32 {
    if let Err(e) = fs::create_dir_all(output_path) {
        writeln!(output, "Failed to create output directory: {}", e).unwrap();
        return 1;
    }

    let normalized_search_paths: Vec<PathBuf> =
        search_paths.iter().map(|p| normalize_path(p)).collect();

    let mut has_default = false;
    let mut has_default_dark = false;
    let mut qrc_entries = Vec::new();
    let mut seen_qrc_entries = HashSet::new();

    for theme_entry in icon_themes {
        let (source_theme, new_theme_name) = parse_theme_spec(theme_entry);
        if let Err(err_msg) = process_theme(
            output,
            &source_theme,
            &new_theme_name,
            &normalized_search_paths,
            icons,
            fallback_theme,
            output_path,
            &mut has_default,
            &mut has_default_dark,
            &mut qrc_entries,
            &mut seen_qrc_entries,
        ) {
            writeln!(output, "{}", err_msg).unwrap();
            return 1;
        }
    }

    let qrc_file_path = if output_qrc_path.is_absolute() {
        output_qrc_path.to_path_buf()
    } else {
        output_path.join(output_qrc_path)
    };

    if let Some(parent) = qrc_file_path.parent()
        && let Err(e) = fs::create_dir_all(parent)
    {
        writeln!(output, "Failed to create qrc directory: {}", e).unwrap();
        return 1;
    }

    let qrc_content = generate_qrc_content(&qrc_entries, pretty);
    if let Err(e) = fs::write(&qrc_file_path, qrc_content) {
        writeln!(output, "Failed to write qrc file: {}", e).unwrap();
        return 1;
    }

    0
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
        }) => bundle_themes(
            output,
            input,
            args.pretty,
            output_path,
            output_qrc_path,
            search_paths,
            icon_themes,
            icons,
            fallback_theme,
        ),
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
    fn test_parse_theme_spec() {
        assert_eq!(
            parse_theme_spec(Path::new("breeze")),
            (PathBuf::from("breeze"), PathBuf::from("breeze"))
        );
        assert_eq!(
            parse_theme_spec(Path::new("breeze:breeze-custom")),
            (PathBuf::from("breeze"), PathBuf::from("breeze-custom"))
        );
        assert_eq!(
            parse_theme_spec(Path::new("foo-bar:baz-qux")),
            (PathBuf::from("foo-bar"), PathBuf::from("baz-qux"))
        );
    }

    #[test]
    fn test_icon_matches() {
        assert!(icon_matches(Path::new("foo.svg"), Path::new("foo")));
        assert!(icon_matches(Path::new("foo.png"), Path::new("foo")));
        assert!(icon_matches(Path::new("foo"), Path::new("foo")));
        assert!(icon_matches(Path::new("foo.custom.ext"), Path::new("foo")));
        assert!(icon_matches(
            Path::new("document-open.svg"),
            Path::new("document-open")
        ));

        // negative matches
        assert!(!icon_matches(Path::new("foo-bar.svg"), Path::new("foo")));
        assert!(!icon_matches(Path::new("foo."), Path::new("foo")));
        assert!(!icon_matches(Path::new("other.svg"), Path::new("foo")));
        assert!(!icon_matches(Path::new(""), Path::new("foo")));
        assert!(!icon_matches(Path::new("foo.svg"), Path::new("")));
    }

    #[test]
    fn test_get_fallback_icon_names() {
        assert_eq!(
            get_fallback_icon_names(Path::new("folder-open-recent")),
            vec![PathBuf::from("folder-open"), PathBuf::from("folder")]
        );
        assert_eq!(
            get_fallback_icon_names(Path::new("dialog-error")),
            vec![PathBuf::from("dialog")]
        );
        assert!(get_fallback_icon_names(Path::new("dialog")).is_empty());
    }

    #[test]
    fn test_normalize_path() {
        assert_eq!(
            normalize_path(Path::new("foo/bar/../baz")),
            PathBuf::from("foo/baz")
        );
        assert_eq!(
            normalize_path(Path::new("foo/./bar/")),
            PathBuf::from("foo/bar")
        );
        assert_eq!(
            normalize_path(Path::new("/a/b/../../c")),
            PathBuf::from("/c")
        );
    }

    #[test]
    fn test_replace_theme_prefix() {
        assert_eq!(
            replace_theme_prefix(
                Path::new("breeze/actions/22/icon.svg"),
                Path::new("breeze-dark")
            ),
            PathBuf::from("breeze-dark/actions/22/icon.svg")
        );
        assert_eq!(
            replace_theme_prefix(Path::new("breeze/index.theme"), Path::new("custom")),
            PathBuf::from("custom/index.theme")
        );
    }

    #[test]
    fn test_escape_xml() {
        assert_eq!(
            escape_xml(r#"foo & bar < "baz" > 'qux'"#),
            "foo &amp; bar &lt; &quot;baz&quot; &gt; &apos;qux&apos;"
        );
        assert_eq!(escape_xml("normal_text/path.svg"), "normal_text/path.svg");
    }

    #[test]
    fn test_generate_qrc_content() {
        let entries = vec![
            QrcEntry {
                alias: None,
                path: PathBuf::from("default/index.theme"),
            },
            QrcEntry {
                alias: Some(PathBuf::from("theme/actions/16/symlink.svg")),
                path: PathBuf::from("theme/actions/16/real.svg"),
            },
        ];

        let compact = generate_qrc_content(&entries, false);
        assert_eq!(
            compact,
            "<RCC><qresource prefix=\"/icons\"><file>default/index.theme</file><file alias=\"theme/actions/16/symlink.svg\">theme/actions/16/real.svg</file></qresource></RCC>"
        );

        let pretty = generate_qrc_content(&entries, true);
        assert!(pretty.contains("    <qresource prefix=\"/icons\">\n"));
        assert!(pretty.contains("        <file>default/index.theme</file>\n"));
        assert!(pretty.contains(
            "        <file alias=\"theme/actions/16/symlink.svg\">theme/actions/16/real.svg</file>\n"
        ));
    }

    #[test]
    fn test_bundle_themes_workflow() {
        let temp_dir = std::env::temp_dir().join(format!("icontool_test_{}", std::process::id()));
        let _ = fs::remove_dir_all(&temp_dir);

        let search_dir = temp_dir.join("share/icons");
        let theme_dir = search_dir.join("test-theme");
        let dark_theme_dir = search_dir.join("test-theme-dark");
        let output_dir = temp_dir.join("build/icons");

        fs::create_dir_all(theme_dir.join("actions/16")).unwrap();
        fs::create_dir_all(dark_theme_dir.join("actions/16")).unwrap();

        fs::write(theme_dir.join("index.theme"), "Test Theme Index").unwrap();
        fs::write(theme_dir.join("icon-theme.cache"), "Cache").unwrap();
        fs::write(theme_dir.join("actions/16/dialog-ok.svg"), "<svg>ok</svg>").unwrap();
        fs::write(theme_dir.join("actions/16/folder.svg"), "<svg>folder</svg>").unwrap();

        // create a symlink: dialog-accept.svg -> dialog-ok.svg
        #[cfg(unix)]
        std::os::unix::fs::symlink(
            "dialog-ok.svg",
            theme_dir.join("actions/16/dialog-accept.svg"),
        )
        .unwrap();
        #[cfg(windows)]
        std::os::windows::fs::symlink_file(
            "dialog-ok.svg",
            theme_dir.join("actions/16/dialog-accept.svg"),
        )
        .unwrap();

        fs::write(dark_theme_dir.join("index.theme"), "Test Dark Theme Index").unwrap();
        fs::write(
            dark_theme_dir.join("actions/16/dialog-ok.svg"),
            "<svg>dark ok</svg>",
        )
        .unwrap();

        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());

        let res = bundle_themes(
            &mut output,
            &mut input,
            true,
            &output_dir,
            Path::new("test_resources.qrc"),
            &[search_dir.clone()],
            &[
                PathBuf::from("test-theme"),
                PathBuf::from("test-theme-dark"),
            ],
            &[
                PathBuf::from("dialog-accept"),
                PathBuf::from("folder-open-recent"), // should fall back to folder.svg
            ],
            Path::new("hicolor"),
        );

        assert_eq!(res, 0);

        // verify default and default-dark themes created
        let default_index = fs::read_to_string(output_dir.join("default/index.theme")).unwrap();
        assert!(default_index.contains("Inherits=test-theme"));

        let default_dark_index =
            fs::read_to_string(output_dir.join("default-dark/index.theme")).unwrap();
        assert!(default_dark_index.contains("Inherits=test-theme-dark"));

        // verify copied files
        assert!(output_dir.join("test-theme/index.theme").is_file());
        assert!(output_dir.join("test-theme/icon-theme.cache").is_file());
        assert!(
            output_dir
                .join("test-theme/actions/16/dialog-ok.svg")
                .is_file()
        );
        assert!(
            output_dir
                .join("test-theme/actions/16/folder.svg")
                .is_file()
        );

        // verify qrc content
        let qrc_content = fs::read_to_string(output_dir.join("test_resources.qrc")).unwrap();
        assert!(qrc_content.contains("<file>default/index.theme</file>"));
        assert!(qrc_content.contains("<file>default-dark/index.theme</file>"));
        assert!(qrc_content.contains("<file>test-theme/index.theme</file>"));
        assert!(qrc_content.contains("<file>test-theme/icon-theme.cache</file>"));
        assert!(qrc_content.contains(
            "<file alias=\"test-theme/actions/16/dialog-accept.svg\">test-theme/actions/16/dialog-ok.svg</file>"
        ));
        assert!(qrc_content.contains("<file>test-theme/actions/16/folder.svg</file>"));

        // clean up
        let _ = fs::remove_dir_all(&temp_dir);
    }

    #[test]
    fn test_bundle_theme_not_found() {
        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());

        let res = bundle_themes(
            &mut output,
            &mut input,
            false,
            Path::new("non_existent_output"),
            Path::new("resources.qrc"),
            &[PathBuf::from("/non/existent/search/path")],
            &[PathBuf::from("non-existent-theme")],
            &[],
            Path::new("hicolor"),
        );

        assert_eq!(res, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("could not be found"));
    }

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
