#!/bin/bash

# Build script for Chess Engine WebAssembly module

echo "Building Chess Engine WebAssembly module..."

# Check if emscripten is installed
if ! command -v emcc &> /dev/null
then
    echo "Error: Emscripten not found. Please install Emscripten SDK."
    echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Run CMake with Emscripten toolchain
emcmake cmake ..

# Build the project
emmake make

echo "Build complete! Output files are in ../public/"
