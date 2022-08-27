#!/bin/bash
REMOTE=${1:-origin}
SUFFIX="${2}"
VERSION=$(git log --date=short --format="%ad-%h" -1)${SUFFIX}
echo "Version: $VERSION"
git tag $VERSION
git push --tags $REMOTE $VERSION
