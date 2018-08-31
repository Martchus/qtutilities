#!/bin/bash
if [[ ! $@ ]]; then
    echo 'Prints the list of icons required by the projects in the specified directories.'
    echo 'However, no project directories have been specified.'
    exit -1
fi

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

    # find icons in *.qml files
    for icon_name in $(find "$srcdir" -iname '*.qml' -print0 | xargs -0 cat | grep -Pzo '(?<=icon\.name\: |iconName\: |source\: )(\".+\"|(.|\n)* \? \".+\" : \".+\")'); do
        # FIXME: improve pattern to get rid of of this
        [ "${#icon_name}"         -ge 3 ] || continue
        [ "${icon_name: 0 : 1}"  == '"' ] || continue
        [ "${icon_name: -1 : 1}" == '"' ] || continue
        [ "${icon_name: 1 : 1}"  == '#' ] && continue
        icon_names["${icon_name: 1 : -1}"]=1
    done
done

# sort results
sorted_icon_names=("${!icon_names[@]}")
IFS=$'\n' sorted_icon_names=($(sort <<<"${sorted_icon_names[*]}"))
unset IFS

# print results
echo 'set(REQUIRED_ICONS'
for icon_name in "${sorted_icon_names[@]}"; do
  echo "    ${icon_name}"
done
echo ')'
