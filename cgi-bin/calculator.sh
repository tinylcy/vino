#!/bin/bash

set -f    # avoid globbing (expansion of *).

IN="$QUERY_STRING"

array=(${IN//&/ })

echo "${array[0]}"
echo "${array[1]}"
