#!/bin/sh

IN="a=2&b=24"

set -f

array=(${IN//&/ })

echo "${array[0]}"
echo "${array[1]}"


