use clap::{Parser, Subcommand};
use regex::Regex;
use std::collections::{BTreeMap, BTreeSet, HashSet};
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
/// Returns a tuple of `(matched_files, missing_icons)`.
pub fn find_required_icons(
    all_theme_files: &[PathBuf],
    required_icons: &[PathBuf],
) -> (Vec<PathBuf>, Vec<PathBuf>) {
    let mut matched_files = Vec::new();
    let mut missing_icons = Vec::new();
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
            let mut fallback_matched = false;
            for fallback in fallbacks {
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
            if !fallback_matched && !missing_icons.contains(req_icon) {
                missing_icons.push(req_icon.clone());
            }
        }
    }
    (matched_files, missing_icons)
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
    error_on_missing_icon: bool,
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
        let (matched, missing) = find_required_icons(&all_theme_files, icons);
        if error_on_missing_icon && !missing.is_empty() {
            for m in &missing {
                writeln!(
                    output,
                    "The specified icon \"{}\" could not be located in theme \"{}\".",
                    m.display(),
                    source_theme.display()
                )
                .map_err(|e| e.to_string())?;
            }
            return Err(format!(
                "Failed to locate {} required icon(s) in theme \"{}\".",
                missing.len(),
                source_theme.display()
            ));
        }
        matched
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
    error_on_missing_icon: bool,
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
            error_on_missing_icon,
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

/// Strips C and C++ style line comments (`//`) and block comments (`/* ... */`), preserving string literals.
pub fn strip_c_comments(input: &str) -> String {
    let mut out = String::with_capacity(input.len());
    let mut chars = input.chars().peekable();
    while let Some(c) = chars.next() {
        if c == '"' || c == '\'' {
            out.push(c);
            let quote = c;
            let mut escaped = false;
            for nc in chars.by_ref() {
                out.push(nc);
                if escaped {
                    escaped = false;
                } else if nc == '\\' {
                    escaped = true;
                } else if nc == quote {
                    break;
                }
            }
        } else if c == '/' {
            if let Some(&'/') = chars.peek() {
                chars.next();
                for nc in chars.by_ref() {
                    if nc == '\n' {
                        out.push('\n');
                        break;
                    }
                }
            } else if let Some(&'*') = chars.peek() {
                chars.next();
                while let Some(nc) = chars.next() {
                    if nc == '*' && chars.peek() == Some(&'/') {
                        chars.next();
                        break;
                    }
                }
            } else {
                out.push(c);
            }
        } else {
            out.push(c);
        }
    }
    out
}

/// Strips XML comments (`<!-- ... -->`).
pub fn strip_xml_comments(input: &str) -> String {
    let mut out = String::with_capacity(input.len());
    let mut rest = input;
    while let Some(start) = rest.find("<!--") {
        out.push_str(&rest[..start]);
        if let Some(end) = rest[start + 4..].find("-->") {
            rest = &rest[start + 4 + end + 3..];
        } else {
            rest = "";
            break;
        }
    }
    out.push_str(rest);
    out
}

fn extract_string_literals(input: &str) -> Vec<String> {
    let re = Regex::new(r#"["']([^"']+)["']"#).unwrap();
    let mut result = Vec::new();
    for cap in re.captures_iter(input) {
        let s = cap[1].trim();
        if !s.is_empty() {
            result.push(s.to_string());
        }
    }
    result
}

/// Extracts icons referenced via `QIcon::fromTheme(...)` and `QIcon::hasThemeIcon(...)` in C++ source code.
pub fn extract_cpp_icons(content: &str) -> Vec<String> {
    let stripped = strip_c_comments(content);
    let mut icons = Vec::new();

    let re_call = Regex::new(r#"QIcon::(?:fromTheme|hasThemeIcon)\s*\("#).unwrap();
    for mat in re_call.find_iter(&stripped) {
        let after_paren = &stripped[mat.end()..];
        let mut depth = 0;
        let mut arg_end = after_paren.len();
        let mut in_quote: Option<char> = None;
        let mut escaped = false;

        for (idx, c) in after_paren.char_indices() {
            if let Some(q) = in_quote {
                if escaped {
                    escaped = false;
                } else if c == '\\' {
                    escaped = true;
                } else if c == q {
                    in_quote = None;
                }
            } else {
                match c {
                    '"' | '\'' => in_quote = Some(c),
                    '(' | '[' | '{' => depth += 1,
                    ')' => {
                        if depth == 0 {
                            arg_end = idx;
                            break;
                        } else {
                            depth -= 1;
                        }
                    }
                    ',' if depth == 0 => {
                        arg_end = idx;
                        break;
                    }
                    _ => {}
                }
            }
        }

        let first_arg = &after_paren[..arg_end];
        for s in extract_string_literals(first_arg) {
            icons.push(s);
        }
    }

    icons
}

/// Extracts icons from Qt Designer UI files (`<iconset ... theme="..." ...>`).
pub fn extract_ui_icons(content: &str) -> Vec<String> {
    let stripped = strip_xml_comments(content);
    let mut icons = Vec::new();
    let re = Regex::new(r#"<iconset\b[^>]*\btheme\s*=\s*["']([^"']+)["']"#).unwrap();
    for cap in re.captures_iter(&stripped) {
        let s = cap[1].trim();
        if !s.is_empty() {
            icons.push(s.to_string());
        }
    }
    icons
}

fn extract_qml_expression(input: &str) -> String {
    let mut depth_paren: usize = 0;
    let mut depth_brace: usize = 0;
    let mut depth_bracket: usize = 0;
    let mut ternary_depth: usize = 0;
    let mut in_quote: Option<char> = None;
    let mut escaped = false;
    let mut end_idx = input.len();

    for (idx, c) in input.char_indices() {
        if let Some(q) = in_quote {
            if escaped {
                escaped = false;
            } else if c == '\\' {
                escaped = true;
            } else if c == q {
                in_quote = None;
            }
        } else {
            match c {
                '"' | '\'' => in_quote = Some(c),
                '(' => depth_paren += 1,
                ')' => depth_paren = depth_paren.saturating_sub(1),
                '{' => depth_brace += 1,
                '}' => {
                    if depth_brace == 0 && depth_paren == 0 && depth_bracket == 0 {
                        end_idx = idx;
                        break;
                    }
                    depth_brace = depth_brace.saturating_sub(1);
                }
                '[' => depth_bracket += 1,
                ']' => depth_bracket = depth_bracket.saturating_sub(1),
                '?' => ternary_depth += 1,
                ':' => {
                    if ternary_depth > 0 {
                        ternary_depth -= 1;
                    } else if depth_paren == 0 && depth_brace == 0 && depth_bracket == 0 {
                        end_idx = idx;
                        break;
                    }
                }
                ';' => {
                    if depth_paren == 0 && depth_brace == 0 && depth_bracket == 0 {
                        end_idx = idx;
                        break;
                    }
                }
                '\n' if depth_paren == 0
                    && depth_brace == 0
                    && depth_bracket == 0
                    && ternary_depth == 0 =>
                {
                    end_idx = idx;
                    break;
                }
                _ => {}
            }
        }
    }
    input[..end_idx].to_string()
}

/// Extracts icons referenced in QML files (e.g. `icon.name`, `Plasmoid.icon`, `icon: "..."`, `icon { name: "..." }`).
pub fn extract_qml_icons(content: &str) -> Vec<String> {
    let stripped = strip_c_comments(content);
    let mut icons = Vec::new();

    // handle icon.name and Plasmoid.icon assignments
    let re_icon_name = Regex::new(r#"(?:\bicon\.name|\bPlasmoid\.icon)\s*[:=]"#).unwrap();
    for mat in re_icon_name.find_iter(&stripped) {
        let expr = extract_qml_expression(&stripped[mat.end()..]);
        for s in extract_string_literals(&expr) {
            icons.push(s);
        }
    }

    // handle direct icon property assignment: e.g. icon: "foo"
    let re_direct_icon = Regex::new(r#"(?:^|[^\w.])icon\s*:"#).unwrap();
    for mat in re_direct_icon.find_iter(&stripped) {
        let expr = extract_qml_expression(&stripped[mat.end()..]);
        let trimmed = expr.trim();
        if !trimmed.starts_with('{') {
            for s in extract_string_literals(&expr) {
                icons.push(s);
            }
        }
    }

    // handle grouped property syntax: `icon { ... name: "..." ... }`
    let re_grouped = Regex::new(r#"(?:^|[^\w.])icon\s*\{([^}]+)\}"#).unwrap();
    let re_name_prop = Regex::new(r#"\bname\s*[:=]"#).unwrap();
    for cap in re_grouped.captures_iter(&stripped) {
        let block = &cap[1];
        for name_mat in re_name_prop.find_iter(block) {
            let expr = extract_qml_expression(&block[name_mat.end()..]);
            for s in extract_string_literals(&expr) {
                icons.push(s);
            }
        }
    }

    icons
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SourceFileType {
    Cpp,
    Ui,
    Qml,
}

pub fn get_source_file_type(path: &Path) -> Option<SourceFileType> {
    let ext = path.extension()?.to_str()?.to_ascii_lowercase();
    match ext.as_str() {
        "cpp" | "cxx" | "cc" | "c++" | "cp" | "c" | "h" | "hpp" | "hxx" | "hh" | "h++" | "tpp"
        | "ipp" => Some(SourceFileType::Cpp),
        "ui" => Some(SourceFileType::Ui),
        "qml" => Some(SourceFileType::Qml),
        _ => None,
    }
}

pub fn collect_source_files(dir: &Path, files: &mut Vec<PathBuf>) -> io::Result<()> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let file_type = entry.file_type()?;
        let path = entry.path();
        if file_type.is_dir() {
            if let Some(name) = path.file_name().and_then(|n| n.to_str())
                && (name == ".git" || name == "target")
            {
                continue;
            }
            collect_source_files(&path, files)?;
        } else if (file_type.is_file()
            || (file_type.is_symlink()
                && fs::metadata(&path).map(|m| m.is_file()).unwrap_or(false)))
            && get_source_file_type(&path).is_some()
        {
            files.push(path);
        }
    }
    Ok(())
}

pub fn find_icons(
    output: &mut dyn Write,
    pretty: bool,
    directories: &[PathBuf],
    with_locations: bool,
) -> i32 {
    let mut files = Vec::new();
    for path in directories {
        if !path.exists() {
            writeln!(
                output,
                "The specified path \"{}\" does not exist.",
                path.display()
            )
            .unwrap();
            return 1;
        }
        if path.is_dir() {
            if let Err(e) = collect_source_files(path, &mut files) {
                writeln!(
                    output,
                    "Failed to read directory \"{}\": {}",
                    path.display(),
                    e
                )
                .unwrap();
                return 1;
            }
        } else if path.is_file() && get_source_file_type(path).is_some() {
            files.push(path.clone());
        }
    }

    let mut icon_locations: BTreeMap<String, BTreeSet<PathBuf>> = BTreeMap::new();

    for file_path in &files {
        let content = match fs::read_to_string(file_path) {
            Ok(c) => c,
            Err(_) => continue,
        };

        let file_type = match get_source_file_type(file_path) {
            Some(t) => t,
            None => continue,
        };

        let found_icons = match file_type {
            SourceFileType::Cpp => extract_cpp_icons(&content),
            SourceFileType::Ui => extract_ui_icons(&content),
            SourceFileType::Qml => extract_qml_icons(&content),
        };

        for icon in found_icons {
            icon_locations
                .entry(icon)
                .or_default()
                .insert(file_path.clone());
        }
    }

    if with_locations {
        for (icon, locs) in &icon_locations {
            if pretty {
                writeln!(output, "{}:", icon).unwrap();
                for loc in locs {
                    writeln!(output, "  {}", loc.display()).unwrap();
                }
            } else {
                let locs_str = locs
                    .iter()
                    .map(|p| p.display().to_string())
                    .collect::<Vec<_>>()
                    .join(", ");
                writeln!(output, "{}: {}", icon, locs_str).unwrap();
            }
        }
    } else {
        for icon in icon_locations.keys() {
            writeln!(output, "{}", icon).unwrap();
        }
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
            error_on_missing_icon,
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
            *error_on_missing_icon,
        ),
        Some(Commands::Find {
            directories,
            with_locations,
        }) => find_icons(output, args.pretty, directories, *with_locations),
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
            false,
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
        let output_path = Path::new("non_existent_output");

        let res = bundle_themes(
            &mut output,
            &mut input,
            false,
            output_path,
            Path::new("resources.qrc"),
            &[PathBuf::from("/non/existent/search/path")],
            &[PathBuf::from("non-existent-theme")],
            &[],
            Path::new("hicolor"),
            false,
        );

        assert_eq!(res, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains("could not be found"));
        fs::remove_dir_all(&output_path).unwrap();
    }

    #[test]
    fn test_error_on_missing_icon() {
        let temp_dir =
            std::env::temp_dir().join(format!("icontool_test_missing_{}", std::process::id()));
        let _ = fs::remove_dir_all(&temp_dir);

        let search_dir = temp_dir.join("share/icons");
        let theme_dir = search_dir.join("test-theme");
        let output_dir = temp_dir.join("build/icons");

        fs::create_dir_all(theme_dir.join("actions/16")).unwrap();
        fs::write(theme_dir.join("index.theme"), "Test Theme Index").unwrap();
        fs::write(
            theme_dir.join("actions/16/existing.svg"),
            "<svg>existing</svg>",
        )
        .unwrap();

        // missing icon does not cause failure when error_on_missing_icon is false
        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());
        let res = bundle_themes(
            &mut output,
            &mut input,
            false,
            &output_dir,
            Path::new("test.qrc"),
            &[search_dir.clone()],
            &[PathBuf::from("test-theme")],
            &[
                PathBuf::from("existing"),
                PathBuf::from("non-existent-icon"),
            ],
            Path::new("hicolor"),
            false,
        );
        assert_eq!(res, 0);

        // missing icon causes failure when error_on_missing_icon is true
        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());
        let res = bundle_themes(
            &mut output,
            &mut input,
            false,
            &output_dir,
            Path::new("test.qrc"),
            &[search_dir.clone()],
            &[PathBuf::from("test-theme")],
            &[
                PathBuf::from("existing"),
                PathBuf::from("non-existent-icon"),
            ],
            Path::new("hicolor"),
            true,
        );
        assert_eq!(res, 1);
        let out_str = String::from_utf8(output).unwrap();
        assert!(out_str.contains(
            "The specified icon \"non-existent-icon\" could not be located in theme \"test-theme\"."
        ));

        let _ = fs::remove_dir_all(&temp_dir);
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

    #[test]
    fn test_strip_c_comments() {
        let code = r#"
            // Line comment with "quote"
            auto a = "keep // this";
            /* Block comment with "quote" */
            auto b = "keep /* this */";
            /* Multi-line
               comment */
            auto c = 123;
        "#;
        let stripped = strip_c_comments(code);
        assert!(!stripped.contains("Line comment"));
        assert!(!stripped.contains("Block comment"));
        assert!(!stripped.contains("Multi-line"));
        assert!(stripped.contains(r#""keep // this""#));
        assert!(stripped.contains(r#""keep /* this */""#));
        assert!(stripped.contains("auto c = 123;"));
    }

    #[test]
    fn test_strip_xml_comments() {
        let xml = r#"
            <root>
                <!-- This is a comment with <iconset theme="foo"/> -->
                <iconset theme="real-icon"/>
            </root>
        "#;
        let stripped = strip_xml_comments(xml);
        assert!(!stripped.contains("This is a comment"));
        assert!(!stripped.contains("foo"));
        assert!(stripped.contains(r#"theme="real-icon""#));
    }

    #[test]
    fn test_extract_cpp_icons() {
        let code = r#"
            auto i1 = QIcon::fromTheme("simple-icon");
            auto i2 = QIcon::fromTheme(QStringLiteral("literal-icon"));
            auto i3 = QIcon::fromTheme(QLatin1String("latin-icon"), fallback);
            auto i4 = QIcon::fromTheme(u"qt6-string"_s);
            auto i5 = QIcon::fromTheme(
                QStringLiteral("multiline-icon"),
                QIcon(QStringLiteral(":/fallback/not-an-icon.svg"))
            );
            auto i6 = QIcon::fromTheme(cond ? QStringLiteral("ternary-a") : QStringLiteral("ternary-b"));
            auto i7 = QIcon::hasThemeIcon(QStringLiteral("has-theme-icon"));
            // auto commented = QIcon::fromTheme("commented-icon");
            /* auto commented2 = QIcon::fromTheme("commented-icon2"); */
        "#;
        let icons = extract_cpp_icons(code);
        assert_eq!(
            icons,
            vec![
                "simple-icon",
                "literal-icon",
                "latin-icon",
                "qt6-string",
                "multiline-icon",
                "ternary-a",
                "ternary-b",
                "has-theme-icon"
            ]
        );
    }

    #[test]
    fn test_extract_ui_icons() {
        let ui = r#"
            <widget class="QPushButton">
                <!-- <iconset theme="commented-ui"/> -->
                <property name="icon">
                    <iconset theme="dialog-ok-symbolic"/>
                </property>
            </widget>
            <widget class="QToolButton">
                <property name="icon">
                    <iconset resource="../icons.qrc" theme='edit-copy-symbolic'>
                    </iconset>
                </property>
            </widget>
            <widget class="QAction">
                <property name="icon">
                    <iconset resource="../icons.qrc">
                        <normaloff>:/icon.png</normaloff>
                    </iconset>
                </property>
            </widget>
        "#;
        let icons = extract_ui_icons(ui);
        assert_eq!(icons, vec!["dialog-ok-symbolic", "edit-copy-symbolic"]);
    }

    #[test]
    fn test_extract_qml_icons() {
        let qml = r#"
            Item {
                icon.name: "folder-open"
                Plasmoid.icon: "plasmoid-app"
                icon.name: cond ? "ternary-1" : "ternary-2"
                icon.name: Utils.fallbackIconName("fallback-icon")
                icon.name: kde ? "kde-icon" : ""
                // icon.name: "commented-qml"
                /* icon.name: "commented-qml2" */
                icon.source: "file.png"
                icon: "menu-icon"
                icon {
                    name: "grouped-icon"
                }
            }
        "#;
        let icons = extract_qml_icons(qml);
        assert_eq!(
            icons,
            vec![
                "folder-open",
                "plasmoid-app",
                "ternary-1",
                "ternary-2",
                "fallback-icon",
                "kde-icon",
                "menu-icon",
                "grouped-icon",
            ]
        );
    }

    #[test]
    fn test_find_icons_workflow() {
        let temp_dir =
            std::env::temp_dir().join(format!("icontool_test_find_{}", std::process::id()));
        let _ = fs::remove_dir_all(&temp_dir);

        let src_dir = temp_dir.join("src");
        let git_dir = temp_dir.join(".git");
        let target_dir = temp_dir.join("target");

        fs::create_dir_all(&src_dir).unwrap();
        fs::create_dir_all(&git_dir).unwrap();
        fs::create_dir_all(&target_dir).unwrap();

        fs::write(
            src_dir.join("main.cpp"),
            r#"auto i = QIcon::fromTheme("icon-from-cpp");"#,
        )
        .unwrap();
        fs::write(
            src_dir.join("dialog.ui"),
            r#"<iconset theme="icon-from-ui"/>"#,
        )
        .unwrap();
        fs::write(
            src_dir.join("View.qml"),
            r#"Item { icon.name: "icon-from-qml" }"#,
        )
        .unwrap();
        fs::write(
            src_dir.join("ignored.txt"),
            r#"QIcon::fromTheme("ignored-text")"#,
        )
        .unwrap();

        // files inside .git and target should be ignored
        fs::write(
            git_dir.join("git_test.cpp"),
            r#"QIcon::fromTheme("git-icon");"#,
        )
        .unwrap();
        fs::write(
            target_dir.join("target_test.cpp"),
            r#"QIcon::fromTheme("target-icon");"#,
        )
        .unwrap();

        // plain output
        let mut output = Vec::new();
        let res = find_icons(&mut output, false, &[temp_dir.clone()], false);
        assert_eq!(res, 0);
        let out_str = String::from_utf8(output).unwrap();
        let lines: Vec<&str> = out_str.lines().collect();
        assert_eq!(
            lines,
            vec!["icon-from-cpp", "icon-from-qml", "icon-from-ui"]
        );

        // output with locations
        let mut output_loc = Vec::new();
        let res_loc = find_icons(&mut output_loc, false, &[temp_dir.clone()], true);
        assert_eq!(res_loc, 0);
        let out_loc_str = String::from_utf8(output_loc).unwrap();
        assert!(out_loc_str.contains("icon-from-cpp: "));
        assert!(out_loc_str.contains("main.cpp"));
        assert!(out_loc_str.contains("icon-from-ui: "));
        assert!(out_loc_str.contains("dialog.ui"));
        assert!(out_loc_str.contains("icon-from-qml: "));
        assert!(out_loc_str.contains("View.qml"));

        // pretty output with locations
        let mut output_pretty = Vec::new();
        let res_pretty = find_icons(&mut output_pretty, true, &[temp_dir.clone()], true);
        assert_eq!(res_pretty, 0);
        let out_pretty_str = String::from_utf8(output_pretty).unwrap();
        assert!(out_pretty_str.contains("icon-from-cpp:\n  "));

        let _ = fs::remove_dir_all(&temp_dir);
    }
}
