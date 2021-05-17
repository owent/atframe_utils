#!/bin/bash

find . -type f 
  -regex ".*third_party/.*" -prune                \
  -o -regex ".*build_jobs_.*" -prune              \
  -o -regex ".*project/cmake/toolset/.*" -prune   \
  -o -name "*.cmake" -print                       \
  -o -name "*.cmake.in" -print                    \
  -o -name 'CMakeLists.txt' -print                \
  | xargs cmake-format -i

find . -type f 
  -regex ".*third_party/.*" -prune                \
  -o -regex ".*build_jobs_.*" -prune              \
  -o -regex ".*project/cmake/toolset/.*" -prune   \
  -o -name "*.h" -print                           \
  -o -name "*.hpp" -print                         \
  -o -name "*.cxx" -print                         \
  -o -name '*.cpp' -print                         \
  -o -name '*.cc' -print                          \
  -o -name '*.c' -print                           \
  | xargs clang-format -i --style=file --fallback-style= none
