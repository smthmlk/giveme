#!/bin/bash

if [[ $1 == "decode" ]]; then
	#echo "decode mode."
	cp "$2" "$3"
	exit 0
fi

if [[ $1 == "encode" ]]; then
	#echo "encode mode"
	if [[ ./"$2" != "$3" ]]; then
		cp "$2" "$3"
	fi
fi

exit 0

