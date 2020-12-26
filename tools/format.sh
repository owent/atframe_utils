#!/bin/bash


# Require python3 -m pip install --user cmake-format 
find .  -name 'build_jobs_*' -prune -o      \
        -name '.vscode' -prune -o           \
        -name '.vs' -prune -o               \
        -name '.clion' -prune -o            \
        -name '3rd_party' -prune -o         \
        -name 'third_party' -prune -o       \
        -type f                             \
        -name "*.cmake" -print              \
        -o -name "*.cmake.in" -print        \
        -o -name 'CMakeLists.txt' -print    \
        | xargs cmake-format -i
