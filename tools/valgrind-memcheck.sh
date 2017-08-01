#!/bin/bash

for FILE in "$@"; do
    valgrind --tool=memcheck --log-file="$FILE.valgrind-memcheck.log" --leak-check=full --show-reachable=yes --max-threads=32768 --valgrind-stacksize=262144 "$FILE";
done