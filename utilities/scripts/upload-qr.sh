#!/usr/bin/env bash

#
# upload-qr.sh
# A script for utilizing qrtool and (for macOS) screenshots to find and upload a new TOTP to the specified device.
# Usage: ./upload_qr.sh <device> <path to QR code PNG>
#

cd "$(dirname "$0")"/..

make parse
make load

# If arg 2 is not defined and on macOS, use the most recent screenshot in the default location
LOC=$2
if [[ -z "$LOC" ]]; then
    if [[ "$(uname)" == "Darwin" ]]; then
        # TODO - see if this is possible without parsing `ls` output due to it
        # not being standardized; it's fine here for now due to generally all
        # macOS devices having the same `ls` implementation unless the user has
        # a custom implementation, which is very unlikely
        LOC=`
            find $HOME/Desktop -regex '.*Screenshot .* at .*\.png$' -print0 \
            | xargs -0 ls -t \
            | head -n1
        `
    else
        echo "Specify a screenshot to use as arg 2 or run this on macOS"
        exit 1
    fi
fi

echo $(qrtool decode "$LOC" | ./parse)
# ./load $1 $(qrtool decode "$LOC" | ./parse)
