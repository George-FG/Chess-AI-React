# Chess Engine (C++)

This directory contains the C++ implementation of the chess AI engine, compiled to WebAssembly for use in the React frontend.

## Structure

- `Engine.h` - Header file with type definitions and class declarations
- `Engine.cpp` - Implementation of the chess engine (minimax, move generation, evaluation)
- `bridge.cpp` - WebAssembly bridge using Emscripten's embind
- `CMakeLists.txt` - CMake build configuration
- `build.sh` - Build script for compiling to WebAssembly

## Features

- **Minimax Algorithm**: Alpha-beta pruning for efficient move search
- **Move Generation**: Complete chess move validation including special moves
- **Position Evaluation**: Material and positional evaluation
- **WebAssembly Bridge**: JavaScript-friendly interface using Emscripten

## Prerequisites

You need to install the Emscripten SDK to compile the C++ code to WebAssembly:

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Building

### Quick Build

```bash
chmod +x build.sh
./build.sh
```

### Manual Build

```bash
mkdir -p build
cd build
emcmake cmake ..
emmake make
```

The output files (`chess-engine.js` and `chess-engine.wasm`) will be placed in the `public` directory.

## Usage in Frontend

The compiled WebAssembly module can be imported and used in the React frontend:

```javascript
import ChessEngineModule from './chess-engine.js';

const engine = await ChessEngineModule();
const chessEngine = new engine.ChessEngine(3); // depth = 3

// Set the board state
chessEngine.setBoardFromArray(boardArray);
chessEngine.setCastlingRights(true, true, true, true);

// Find the best move
const bestMove = chessEngine.findBestMove("white");
console.log(bestMove); // { from: {row, col}, to: {row, col}, piece: {type, color} }
```

## Performance

The C++ implementation provides significant performance improvements over the JavaScript version:
- Faster move generation
- More efficient minimax search
- Lower memory overhead
- Can search deeper in the same amount of time
