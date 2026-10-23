use regex::Regex;
use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::io::{self, Write};
use std::path::{Path, PathBuf};

/// Strips C and C++ style line comments (`//`) and block comments (`/* ... */`), preserving string literals.
fn strip_c_comments(input: &str) -> String {
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
fn strip_xml_comments(input: &str) -> String {
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
fn extract_cpp_icons(content: &str) -> Vec<String> {
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
fn extract_ui_icons(content: &str) -> Vec<String> {
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
fn extract_qml_icons(content: &str) -> Vec<String> {
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
enum SourceFileType {
    Cpp,
    Ui,
    Qml,
}

fn get_source_file_type(path: &Path) -> Option<SourceFileType> {
    let ext = path.extension()?.to_str()?.to_ascii_lowercase();
    match ext.as_str() {
        "cpp" | "cxx" | "cc" | "c++" | "cp" | "c" | "h" | "hpp" | "hxx" | "hh" | "h++" | "tpp"
        | "ipp" => Some(SourceFileType::Cpp),
        "ui" => Some(SourceFileType::Ui),
        "qml" => Some(SourceFileType::Qml),
        _ => None,
    }
}

fn collect_source_files(dir: &Path, files: &mut Vec<PathBuf>) -> io::Result<()> {
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

pub(crate) fn find_icons(
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

#[cfg(test)]
mod tests {
    use super::*;

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
