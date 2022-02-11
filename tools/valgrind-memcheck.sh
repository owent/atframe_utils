#!/bin/bash

valgrind --tool=memcheck --log-file="$1.valgrind-memcheck.log" --leak-check=full --show-reachable=yes --max-threads=32768 --valgrind-stacksize=262144 \
  "$@"
