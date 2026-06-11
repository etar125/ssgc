#!/bin/sh
# Copy command for ssgc
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "usage: copycmd.sh src dst"
    exit 1
fi
echo "copying $1 to $2"
read r
rm -rfv $2
cp -rfv $1 $2
rm -fv $2/ssgc.elist
rm -fv $2/template.html
rm -fv $2/ssgc.lock
