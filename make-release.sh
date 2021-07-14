#!/bin/bash

copy2folder() {
    mkdir -p "$2"
    cp -r "Dist/." "$2"
    cp "lcs-manager/LICENSE" "$2"

    cp "$1/lcs-manager/lcs-manager.exe" "$2"
    cp "$1/lcs-manager/lolcustomskin/lolcustomskin.exe" "$2"
    cp "$1/lcs-wadextract/lcs-wadextract.exe" "$2"
    cp "$1/lcs-wadmake/lcs-wadmake.exe" "$2"
    cp "$1/lcs-wxyextract/lcs-wxyextract.exe" "$2"
    curl -R -L -o "$2/hashes.game.txt" -z "$2/hashes.game.txt" "https://raw.githubusercontent.com/CommunityDragon/CDTB/master/cdragontoolbox/hashes.game.txt"
}

VERSION=$(git log --format="%as-%h" -1)
echo "Version: $VERSION"

if [ "$#" -gt 1 ] && [ -d "$2" ]; then
    copy2folder "$2" "lolcustomskin-tools-32"
fi;
if [ "$#" -gt 0 ] && [ -d "$1" ]; then
    copy2folder "$1" "lolcustomskin-tools-64"
else
    echo "Error: Provide at least one valid path."
    exit
fi;
