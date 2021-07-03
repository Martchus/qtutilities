#!/bin/bash

# abort on first error
set -e

if [[ ! $@ ]]; then
    echo 'Updates the the PNG icons of the projects in the specified directories.'
    echo 'However, no project directories have been specified.'
    exit -1
fi

# define array for commands to be executed
cmds=()

# iterate over specified source directories
for srcdir in "$@"; do
    # find SVG icons
    for svg_icon_full_path in $(find "$srcdir" -iname '*.svg'); do
        prefix="${svg_icon_full_path%/scalable/*}"
        svg_icon="${svg_icon_full_path##*/scalable/}"
        # add inkscape command for each icon and size and ensure output directory exists
        for size in 16 32 48; do
            mkdir -p "${prefix}/${size}x${size}/${svg_icon%/*.svg}"
            cmds+=("inkscape --without-gui \"${svg_icon_full_path}\" --export-png=\"${prefix}/${size}x${size}/${svg_icon%.svg}.png\" --export-width=${size} --export-height=${size}")
        done
    done
done

# run commands
function print_cmds {
    for cmd in "${cmds[@]}"; do
        echo "$cmd"
    done
}
echo "Executing the following commands:"
print_cmds
print_cmds | parallel
