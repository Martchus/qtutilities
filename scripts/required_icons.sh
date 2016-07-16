#!/bin/bash
declare -A icon_names

# iterate over specified source directories
for srcdir in "$@"; do
    # find icons in *.ui files
    for iconset in $(find "$srcdir" -iname '*.ui' -print0 | xargs -0 cat | grep -Po '<iconset\stheme=\".*?\"'); do
        if [ "${iconset:0:7}" == 'theme="' ]; then
            icon_names["${iconset: 7 : -1}"]=1
        fi
    done
    # find icons in *.cpp files
    for from_theme_call in $(find "$srcdir" -iname '*.cpp' -print0 | xargs -0 cat | grep -Po 'QIcon::fromTheme\(QStringLiteral\(\".*?\"\)'); do
        icon_names["${from_theme_call: 33 : -2}"]=1
    done
done

# print results
echo 'set(REQUIRED_ICONS'
for icon_name in "${!icon_names[@]}"
do
  echo "    ${icon_name}"
done
echo ')'
