#!/bin/bash

for FILE in "$@"; do
    valgrind --tool=memcheck --log-file="$FILE.valgrind-memcheck.log" --leak-check=full --show-reachable=yes "$FILE";
done