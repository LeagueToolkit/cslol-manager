#!/bin/bash
REMOTE=${1:-origin}
VERSION=$(git log --date=short --format="%ad-%h" -1)
echo "Version: $VERSION"
git tag $VERSION
git push --tags $REMOTE $VERSION
