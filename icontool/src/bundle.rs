use std::collections::HashSet;
use std::fs;
use std::io::{self, BufRead, Write};
use std::path::{Component, Path, PathBuf};

/// Entry representing a file in the Qt resource (.qrc) file.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct QrcEntry {
    pub alias: Option<PathBuf>,
    pub path: PathBuf,
}

/// Specification for a required icon: how it should be named when bundled,
/// and a prioritized list of icon names to look for in the theme.
#[derive(Debug, Clone, PartialEq, Eq)]
struct IconSpec {
    pub bundled_name: PathBuf,
    pub options: Vec<PathBuf>,
    pub is_explicit_alias: bool,
}

/// Parses an icon specification, which can be `icon_name` or `bundled_name:option1,option2,...`.
fn parse_icon_spec(spec: &Path) -> IconSpec {
    let bytes = spec.as_os_str().as_encoded_bytes();
    if let Some(pos) = bytes.iter().position(|&b| b == b':') {
        #[cfg(windows)]
        let is_drive_letter =
            pos == 1 && (bytes.get(2) == Some(&b'\\') || bytes.get(2) == Some(&b'/'));
        #[cfg(not(windows))]
        let is_drive_letter = false;

        if !is_drive_letter {
            let bundled_bytes = &bytes[..pos];
            let options_bytes = &bytes[pos + 1..];

            #[cfg(unix)]
            use std::os::unix::ffi::OsStrExt;

            #[cfg(unix)]
            let bundled_name = PathBuf::from(std::ffi::OsStr::from_bytes(bundled_bytes));
            #[cfg(not(unix))]
            let bundled_name = PathBuf::from(String::from_utf8_lossy(bundled_bytes).as_ref());

            let mut options = Vec::new();
            for part in options_bytes.split(|&b| b == b',') {
                if !part.is_empty() {
                    #[cfg(unix)]
                    options.push(PathBuf::from(std::ffi::OsStr::from_bytes(part)));
                    #[cfg(not(unix))]
                    options.push(PathBuf::from(String::from_utf8_lossy(part).as_ref()));
                }
            }

            if options.is_empty() {
                options.push(bundled_name.clone());
            }

            return IconSpec {
                bundled_name,
                options,
                is_explicit_alias: true,
            };
        }
    }

    IconSpec {
        bundled_name: spec.to_path_buf(),
        options: vec![spec.to_path_buf()],
        is_explicit_alias: false,
    }
}

/// Parses a theme specification, which can be `theme_name` or `source_theme:new_theme_name`.
fn parse_theme_spec(theme_spec: &Path) -> (PathBuf, PathBuf) {
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
fn icon_matches(file_name: &Path, icon_name: &Path) -> bool {
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
fn get_fallback_icon_names(icon_name: &Path) -> Vec<PathBuf> {
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
fn normalize_path(path: &Path) -> PathBuf {
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
fn resolve_symlink(mut path: PathBuf) -> io::Result<PathBuf> {
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
fn replace_theme_prefix(rel_path: &Path, new_theme_name: &Path) -> PathBuf {
    let mut components = rel_path.components();
    let mut result = PathBuf::from(new_theme_name);
    components.next(); // discard original first component
    for comp in components {
        result.push(comp.as_os_str());
    }
    result
}

/// Converts a path to a Qt resource path (using forward slashes `/`).
fn path_to_qrc_path(path: &Path) -> String {
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
fn escape_xml(s: &str) -> String {
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
fn generate_qrc_content(entries: &[QrcEntry], pretty: bool) -> String {
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
fn find_theme_dir(source_theme: &Path, search_paths: &[PathBuf]) -> Option<(PathBuf, PathBuf)> {
    for search_path in search_paths {
        let theme_path = search_path.join(source_theme);
        if theme_path.is_dir() {
            return Some((theme_path, search_path.clone()));
        }
    }
    None
}

/// Finds index files (index.theme, and icon-theme.cache if not fallback theme) in a theme dir.
fn find_index_files(theme_dir: &Path, is_fallback_theme: bool) -> Vec<PathBuf> {
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
fn collect_files(dir: &Path) -> io::Result<Vec<PathBuf>> {
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

/// Finds all matching icons for required icons in a theme, applying candidate options and fallbacks.
/// Returns a tuple of `(matched_files, missing_icons)`.
/// Each matched file is paired with an optional target bundled icon name (if explicitly aliased).
fn find_required_icons(
    all_theme_files: &[PathBuf],
    icon_specs: &[IconSpec],
) -> (Vec<(PathBuf, Option<PathBuf>)>, Vec<PathBuf>) {
    let mut matched_files = Vec::new();
    let mut missing_icons = Vec::new();
    let mut seen_entries = HashSet::new();

    for spec in icon_specs {
        let mut found_for_icon = false;
        let target_name = if spec.is_explicit_alias {
            Some(spec.bundled_name.clone())
        } else {
            None
        };

        // try exact match for candidate options in priority order
        for option in &spec.options {
            let mut matched_for_option = false;
            for file in all_theme_files {
                if let Some(file_name) = file.file_name()
                    && icon_matches(Path::new(file_name), option)
                {
                    matched_for_option = true;
                    if seen_entries.insert((file.clone(), target_name.clone())) {
                        matched_files.push((file.clone(), target_name.clone()));
                    }
                }
            }
            if matched_for_option {
                found_for_icon = true;
                break;
            }
        }

        // try fallbacks (stripping -suffix) in priority order if no candidate option matched directly
        if !found_for_icon {
            for option in &spec.options {
                let fallbacks = get_fallback_icon_names(option);
                let mut fallback_matched = false;
                for fallback in fallbacks {
                    for file in all_theme_files {
                        if let Some(file_name) = file.file_name()
                            && icon_matches(Path::new(file_name), &fallback)
                        {
                            fallback_matched = true;
                            if seen_entries.insert((file.clone(), target_name.clone())) {
                                matched_files.push((file.clone(), target_name.clone()));
                            }
                        }
                    }
                    if fallback_matched {
                        break;
                    }
                }
                if fallback_matched {
                    found_for_icon = true;
                    break;
                }
            }
        }

        if !found_for_icon && !missing_icons.contains(&spec.bundled_name) {
            missing_icons.push(spec.bundled_name.clone());
        }
    }
    (matched_files, missing_icons)
}

/// Writes `content` to `path` only if `path` does not exist or its content differs.
/// Returns `Ok(true)` if the file was written, or `Ok(false)` if unchanged.
fn write_if_different(path: &Path, content: &[u8]) -> io::Result<bool> {
    if path.exists()
        && let Ok(existing) = fs::read(path)
        && existing == content
    {
        return Ok(false);
    }
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(path, content)?;
    Ok(true)
}

/// Copies `src` to `dst` only if `dst` does not exist or its content differs from `src`.
/// Returns `Ok(true)` if the file was copied, or `Ok(false)` if unchanged.
fn copy_if_different(src: &Path, dst: &Path) -> io::Result<bool> {
    if dst.exists()
        && let (Ok(meta_src), Ok(meta_dst)) = (fs::metadata(src), fs::metadata(dst))
        && meta_src.len() == meta_dst.len()
        && let (Ok(src_bytes), Ok(dst_bytes)) = (fs::read(src), fs::read(dst))
        && src_bytes == dst_bytes
    {
        return Ok(false);
    }
    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::copy(src, dst)?;
    Ok(true)
}

/// Processes a single file (index file or icon file) for bundling into output_path and generates a QrcEntry.
fn process_and_copy_file(
    file_path: &Path,
    bundled_name: Option<&Path>,
    search_path: &Path,
    search_paths: &[PathBuf],
    theme_dir: &Path,
    new_theme_name: &Path,
    output_path: &Path,
) -> io::Result<Option<(QrcEntry, bool)>> {
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

    let alias_path = match bundled_name {
        Some(bundled) => {
            let new_file_name = match file_path.extension() {
                Some(ext) if bundled.extension().is_none() => {
                    let mut name = bundled.as_os_str().to_os_string();
                    name.push(".");
                    name.push(ext);
                    PathBuf::from(name)
                }
                _ => bundled.to_path_buf(),
            };
            rel_from_search
                .parent()
                .unwrap_or(Path::new(""))
                .join(new_file_name)
        }
        None => rel_from_search.clone(),
    };

    let (resolved_path, target_rel) = if is_symlink {
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

        (resolved, target_rel)
    } else {
        (file_path.to_path_buf(), rel_from_search)
    };

    let alias = if alias_path != target_rel {
        Some(alias_path)
    } else {
        None
    };

    let dest_file = output_path.join(&target_rel);
    let changed = copy_if_different(&resolved_path, &dest_file)?;

    Ok(Some((
        QrcEntry {
            alias,
            path: target_rel,
        },
        changed,
    )))
}

#[allow(clippy::too_many_arguments)]
fn process_theme(
    output: &mut dyn Write,
    source_theme: &Path,
    new_theme_name: &Path,
    search_paths: &[PathBuf],
    icon_specs: &[IconSpec],
    fallback_theme: &Path,
    output_path: &Path,
    error_on_missing_icon: bool,
    has_default: &mut bool,
    has_default_dark: &mut bool,
    qrc_entries: &mut Vec<QrcEntry>,
    seen_qrc_entries: &mut HashSet<QrcEntry>,
    any_file_changed: &mut bool,
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
        let content = format!(
            "[Icon Theme]\nInherits={}\n",
            new_theme_name.to_string_lossy()
        );
        if write_if_different(&dir.join("index.theme"), content.as_bytes())
            .map_err(|e| e.to_string())?
        {
            *any_file_changed = true;
        }
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
        let content = format!(
            "[Icon Theme]\nInherits={}\n",
            new_theme_name.to_string_lossy()
        );
        if write_if_different(&dir.join("index.theme"), content.as_bytes())
            .map_err(|e| e.to_string())?
        {
            *any_file_changed = true;
        }
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
        let (matched, missing) = find_required_icons(&all_theme_files, icon_specs);
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

    for file_path in &index_files {
        if let Some((entry, changed)) = process_and_copy_file(
            file_path,
            None,
            &search_path,
            search_paths,
            &theme_dir,
            new_theme_name,
            output_path,
        )
        .map_err(|e| e.to_string())?
        {
            if changed {
                *any_file_changed = true;
            }
            if seen_qrc_entries.insert(entry.clone()) {
                qrc_entries.push(entry);
            }
        }
    }

    for (file_path, bundled_name) in &matched_icon_files {
        if let Some((entry, changed)) = process_and_copy_file(
            file_path,
            bundled_name.as_deref(),
            &search_path,
            search_paths,
            &theme_dir,
            new_theme_name,
            output_path,
        )
        .map_err(|e| e.to_string())?
        {
            if changed {
                *any_file_changed = true;
            }
            if seen_qrc_entries.insert(entry.clone()) {
                qrc_entries.push(entry);
            }
        }
    }

    Ok(())
}

#[allow(clippy::too_many_arguments)]
pub(crate) fn bundle_themes(
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

    let icon_specs: Vec<IconSpec> = icons.iter().map(|i| parse_icon_spec(i)).collect();

    let mut has_default = false;
    let mut has_default_dark = false;
    let mut any_file_changed = false;
    let mut qrc_entries = Vec::new();
    let mut seen_qrc_entries = HashSet::new();

    for theme_entry in icon_themes {
        let (source_theme, new_theme_name) = parse_theme_spec(theme_entry);
        if let Err(err_msg) = process_theme(
            output,
            &source_theme,
            &new_theme_name,
            &normalized_search_paths,
            &icon_specs,
            fallback_theme,
            output_path,
            error_on_missing_icon,
            &mut has_default,
            &mut has_default_dark,
            &mut qrc_entries,
            &mut seen_qrc_entries,
            &mut any_file_changed,
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

    let qrc_content = generate_qrc_content(&qrc_entries, pretty);
    if any_file_changed {
        if let Some(parent) = qrc_file_path.parent()
            && let Err(e) = fs::create_dir_all(parent)
        {
            writeln!(output, "Failed to create qrc directory: {}", e).unwrap();
            return 1;
        }
        if let Err(e) = fs::write(&qrc_file_path, qrc_content) {
            writeln!(output, "Failed to write qrc file: {}", e).unwrap();
            return 1;
        }
    } else if let Err(e) = write_if_different(&qrc_file_path, qrc_content.as_bytes()) {
        writeln!(output, "Failed to write qrc file: {}", e).unwrap();
        return 1;
    }

    0
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn test_parse_icon_spec() {
        assert_eq!(
            parse_icon_spec(Path::new("simple-icon")),
            IconSpec {
                bundled_name: PathBuf::from("simple-icon"),
                options: vec![PathBuf::from("simple-icon")],
                is_explicit_alias: false,
            }
        );
        assert_eq!(
            parse_icon_spec(Path::new(
                "sidebar-show-symbolic:sidebar-show-symbolic,view-sidebar-symbolic"
            )),
            IconSpec {
                bundled_name: PathBuf::from("sidebar-show-symbolic"),
                options: vec![
                    PathBuf::from("sidebar-show-symbolic"),
                    PathBuf::from("view-sidebar-symbolic")
                ],
                is_explicit_alias: true,
            }
        );
        assert_eq!(
            parse_icon_spec(Path::new("target:first,second,third")),
            IconSpec {
                bundled_name: PathBuf::from("target"),
                options: vec![
                    PathBuf::from("first"),
                    PathBuf::from("second"),
                    PathBuf::from("third")
                ],
                is_explicit_alias: true,
            }
        );
        assert_eq!(
            parse_icon_spec(Path::new("target:")),
            IconSpec {
                bundled_name: PathBuf::from("target"),
                options: vec![PathBuf::from("target")],
                is_explicit_alias: true,
            }
        );
    }

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

        let qrc_mtime = fs::metadata(output_dir.join("test_resources.qrc"))
            .unwrap()
            .modified()
            .unwrap();
        let ok_svg_mtime = fs::metadata(output_dir.join("test-theme/actions/16/dialog-ok.svg"))
            .unwrap()
            .modified()
            .unwrap();
        let default_index_mtime = fs::metadata(output_dir.join("default/index.theme"))
            .unwrap()
            .modified()
            .unwrap();

        // Second run with identical inputs must be a complete no-op (no files modified, mtimes preserved)
        std::thread::sleep(std::time::Duration::from_millis(50));
        let res2 = bundle_themes(
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
                PathBuf::from("folder-open-recent"),
            ],
            Path::new("hicolor"),
            false,
        );
        assert_eq!(res2, 0);

        let qrc_mtime_after = fs::metadata(output_dir.join("test_resources.qrc"))
            .unwrap()
            .modified()
            .unwrap();
        let ok_svg_mtime_after =
            fs::metadata(output_dir.join("test-theme/actions/16/dialog-ok.svg"))
                .unwrap()
                .modified()
                .unwrap();
        let default_index_mtime_after = fs::metadata(output_dir.join("default/index.theme"))
            .unwrap()
            .modified()
            .unwrap();

        assert_eq!(qrc_mtime, qrc_mtime_after);
        assert_eq!(ok_svg_mtime, ok_svg_mtime_after);
        assert_eq!(default_index_mtime, default_index_mtime_after);

        // clean up
        let _ = fs::remove_dir_all(&temp_dir);
    }

    #[test]
    fn test_write_and_copy_if_different() {
        let temp_dir =
            std::env::temp_dir().join(format!("icontool_test_diff_{}", std::process::id()));
        let _ = fs::remove_dir_all(&temp_dir);
        fs::create_dir_all(&temp_dir).unwrap();

        let file_a = temp_dir.join("file_a.txt");
        let file_b = temp_dir.join("file_b.txt");

        // Initial write
        assert!(write_if_different(&file_a, b"hello").unwrap());
        // Same content -> no write
        assert!(!write_if_different(&file_a, b"hello").unwrap());
        // Different content -> write
        assert!(write_if_different(&file_a, b"hello world").unwrap());

        // Copy
        assert!(copy_if_different(&file_a, &file_b).unwrap());
        // Same content -> no copy
        assert!(!copy_if_different(&file_a, &file_b).unwrap());

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
    fn test_bundle_icon_with_options() {
        let temp_dir =
            std::env::temp_dir().join(format!("icontool_test_options_{}", std::process::id()));
        let _ = fs::remove_dir_all(&temp_dir);

        let search_dir = temp_dir.join("share/icons");
        let theme_dir = search_dir.join("test-theme");
        let output_dir = temp_dir.join("build/icons");

        // write theme that has only has view-sidebar-symbolic.svg, NOT sidebar-show-symbolic.svg
        fs::create_dir_all(theme_dir.join("actions/16")).unwrap();
        fs::write(theme_dir.join("index.theme"), "Test Theme Index").unwrap();
        fs::write(
            theme_dir.join("actions/16/view-sidebar-symbolic.svg"),
            "<svg>sidebar</svg>",
        )
        .unwrap();

        let mut output = Vec::new();
        let mut input = Cursor::new(Vec::new());

        let res = bundle_themes(
            &mut output,
            &mut input,
            true,
            &output_dir,
            Path::new("test.qrc"),
            &[search_dir.clone()],
            &[PathBuf::from("test-theme")],
            &[PathBuf::from(
                "sidebar-show-symbolic:sidebar-show-symbolic,view-sidebar-symbolic",
            )],
            Path::new("hicolor"),
            true,
        );
        assert_eq!(res, 0);

        // verify copied file on disk is view-sidebar-symbolic.svg
        assert!(
            output_dir
                .join("test-theme/actions/16/view-sidebar-symbolic.svg")
                .is_file()
        );

        // verify qrc has alias from sidebar-show-symbolic.svg to view-sidebar-symbolic.svg
        let qrc_content = fs::read_to_string(output_dir.join("test.qrc")).unwrap();
        assert!(qrc_content.contains(
            "<file alias=\"test-theme/actions/16/sidebar-show-symbolic.svg\">test-theme/actions/16/view-sidebar-symbolic.svg</file>"
        ));

        // write theme that has the preferred icon
        fs::write(
            theme_dir.join("actions/16/sidebar-show-symbolic.svg"),
            "<svg>preferred sidebar</svg>",
        )
        .unwrap();

        let mut output2 = Vec::new();
        let mut input2 = Cursor::new(Vec::new());

        let res2 = bundle_themes(
            &mut output2,
            &mut input2,
            true,
            &output_dir,
            Path::new("test.qrc"),
            &[search_dir.clone()],
            &[PathBuf::from("test-theme")],
            &[PathBuf::from(
                "sidebar-show-symbolic:sidebar-show-symbolic,view-sidebar-symbolic",
            )],
            Path::new("hicolor"),
            true,
        );
        assert_eq!(res2, 0);

        // verify that the icon (preferred sidebar-show-symbolic) is bundled directly without alias (as it is present)
        let qrc_content2 = fs::read_to_string(output_dir.join("test.qrc")).unwrap();
        assert!(
            qrc_content2.contains("<file>test-theme/actions/16/sidebar-show-symbolic.svg</file>")
        );

        let _ = fs::remove_dir_all(&temp_dir);
    }
}
