#!/bin/sh

BUILD_DIR="build_$(uname -o)";

cd "$(dirname $0)";
mkdir -p "$BUILD_DIR";
cd "$BUILD_DIR";

cmake .. -DPROJECT_ENABLE_UNITTEST=ON -DPROJECT_ENABLE_SAMPLE=ON $@;
