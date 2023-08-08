#!/bin/bash

copy2folder() {
    mkdir -p "$2"

    cp "./dist/SOURCE.URL" "$2"
    cp "./LICENSE" "$2"

    cp -R "$1/cslol-manager.app" "$2"
    mkdir -p  "$2/cslol-manager.app/Contents/MacOS/cslol-tools"
    cp "$1/cslol-tools/mod-tools" "$2/cslol-manager.app/Contents/MacOS/cslol-tools"
    cp "$1/cslol-tools/wad-"* "$2"
}

VERSION=$(git log --date=short --format="%ad-%h" -1)
echo "Version: $VERSION"

if [ "$#" -gt 0 ] && [ -d "$1" ]; then
    copy2folder "$1" "cslol-manager-macos"
    echo "Version: $VERSION" > "cslol-manager-macos/version.txt"
else
    echo "Error: Provide at least one valid path."
    exit
fi;
