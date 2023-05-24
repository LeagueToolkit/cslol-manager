#!/bin/bash

SCRIPT_DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

osascript <<EOD
    do shell script "sudo $SCRIPT_DIR/cslol-manager" \
    with prompt "CSLoL-Manager needs admin privileges to restart" \
    with administrator privileges \
    with altering line endings
EOD