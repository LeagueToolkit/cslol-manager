#!/bin/bash

copy2folder() {
    mkdir -p "$2/cslol-tools"

    cp "./dist/"*.bat "$2/cslol-tools"
    cp "./dist/FIX-ADMIN.reg" "$2"
    cp "./dist/FIX-NON-ENGLISH.reg" "$2"
    cp "./dist/SOURCE.URL" "$2"
    cp "./LICENSE" "$2"

    cp "$1/cslol-tools/"*.exe "$2/cslol-tools"
    cp "$1/cslol-tools/"*.dll "$2/cslol-tools"
    cp "$1/cslol-manager.exe" "$2"
    echo "windeployqt is only necessary for non-static builds"
    windeployqt --qmldir "src/qml" "$2/cslol-manager.exe"
    curl -N -R -L -o "$2/cslol-tools/hashes.game.txt" "https://raw.communitydragon.org/data/hashes/lol/hashes.game.txt"
}

VERSION=$(git log --date=short --format="%ad-%h" -1)
echo "Version: $VERSION"

if [ "$#" -gt 0 ] && [ -d "$1" ]; then
    copy2folder "$1" "cslol-manager"
    echo "Version: $VERSION" > "cslol-manager/version.txt"
else
    echo "Error: Provide at least one valid path."
    exit
fi;
