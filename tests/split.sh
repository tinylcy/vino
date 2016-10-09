#!/bin/sh

IN="a=2&b=24"

params=$(echo $IN | tr "&" "\n")

for pair in $params
do
	echo "$pair"
done
