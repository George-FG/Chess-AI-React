#!/bin/bash

# Setup and build script for Chess Engine
# This script will install Emscripten if needed and build the WebAssembly module

set -e  # Exit on error

echo "========================================="
echo "Chess Engine WebAssembly Setup & Build"
echo "========================================="
echo ""

# Check if Emscripten is installed
if ! command -v emcc &> /dev/null
then
    echo "Emscripten not found. Installing Emscripten SDK..."
    echo ""
    
    # Clone Emscripten SDK if not exists
    if [ ! -d "emsdk" ]; then
        echo "Cloning Emscripten SDK..."
        git clone https://github.com/emscripten-core/emsdk.git
    fi
    
    cd emsdk
    
    # Install and activate latest Emscripten
    echo "Installing latest Emscripten..."
    ./emsdk install latest
    
    echo "Activating Emscripten..."
    ./emsdk activate latest
    
    # Source the environment
    source ./emsdk_env.sh
    
    cd ..
    
    echo ""
    echo "Emscripten installed successfully!"
    echo ""
else
    echo "✓ Emscripten is already installed"
    echo ""
fi

# Verify emcc is available
if ! command -v emcc &> /dev/null
then
    echo "Error: emcc not found after installation."
    echo "Please run: source ./emsdk/emsdk_env.sh"
    exit 1
fi

echo "Building Chess Engine..."
echo ""

# Navigate to Engine directory
cd Engine

# Make build script executable
chmod +x build.sh

# Run the build
./build.sh

cd ..

echo ""
echo "========================================="
echo "✓ Build complete!"
echo "========================================="
echo ""
echo "WebAssembly files generated in public/:"
echo "  - chess-engine.js"
echo "  - chess-engine.wasm"
echo ""
echo "You can now run: npm run dev"
echo ""
