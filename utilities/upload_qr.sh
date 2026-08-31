#!/usr/bin/env bash

#
# Usage: ./upload_qr.sh <device> <path to QR code PNG>
#

make parse
make load

./load $1 $(qrtool decode $2 | ./parse)
