# Building the C++ Chess Engine

This guide explains how to build the C++ chess engine for WebAssembly.

## Current Status

The application is currently using a **JavaScript fallback** for the AI. To get better performance, you can build the C++ WebAssembly engine following the instructions below.

## Quick Start

If you have Homebrew on macOS:

```bash
# Install Emscripten
brew install emscripten

# Build the engine
cd Engine
chmod +x build.sh
./build.sh
```

## Detailed Instructions

### Prerequisites

You need the Emscripten SDK to compile C++ to WebAssembly. Choose one of these methods:

#### Option 1: Homebrew (macOS - Easiest)

```bash
brew install emscripten
```

#### Option 2: Direct Emscripten SDK Installation

```bash
# Clone the emsdk repository
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate the latest SDK
./emsdk install latest
./emsdk activate latest

# Set up the environment (do this in every new terminal)
source ./emsdk_env.sh
```

### Building the Engine

Once Emscripten is installed:

```bash
# Navigate to the Engine directory
cd Engine

# Make the build script executable
chmod +x build.sh

# Run the build
./build.sh
```

This will create two files in the `public/` directory:
- `chess-engine.js` - JavaScript glue code
- `chess-engine.wasm` - WebAssembly binary

### Verifying the Build

After building, restart your development server:

```bash
npm run dev
```

Open the browser console and look for this message:
```
✓ Using C++ WebAssembly engine
```

If you see this, the C++ engine is working! If you see the fallback message, the build may have failed or the files are not in the right location.

## Troubleshooting

### "emcc: command not found"

If using the direct SDK installation, you need to source the environment in every terminal:
```bash
source ./emsdk/emsdk_env.sh
```

Consider adding this to your shell profile (`~/.zshrc` or `~/.bashrc`):
```bash
source "$HOME/path/to/emsdk/emsdk_env.sh"
```

### Build Fails

1. Ensure you have CMake installed:
   ```bash
   brew install cmake
   ```

2. Clean and rebuild:
   ```bash
   cd Engine
   rm -rf build
   ./build.sh
   ```

### Files Not Found

The WebAssembly files should be in `public/chess-engine.{js,wasm}`. Check:
```bash
ls -lh public/chess-engine.*
```

## Performance Comparison

**JavaScript Engine:**
- Depth 2-3 (default)
- ~500-2000 positions/second
- Suitable for casual play

**C++ WebAssembly Engine:**
- Depth 3-5 (recommended)
- ~10,000-50,000 positions/second
- 5-10x faster than JavaScript
- Better move quality at higher depths

## Technical Details

The C++ engine includes:
- **Minimax with alpha-beta pruning** - Efficient game tree search
- **Move ordering** - Captures prioritized for better pruning
- **Position evaluation** - Material + positional bonuses
- **Full chess rules** - All legal moves, castling, en passant, promotion

The implementation is in:
- `Engine.h` - Type definitions and class declarations
- `Engine.cpp` - Core engine logic
- `bridge.cpp` - WebAssembly bindings using Emscripten's embind
- `CMakeLists.txt` - Build configuration

## Next Steps

Once the engine is built, you can:
- Increase AI depth in game settings (3-5 for C++)
- Compare performance between JavaScript and C++ engines
- Modify the evaluation function in `Engine.cpp` for different play styles
