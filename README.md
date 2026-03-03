# Chess in Vite + React

A full-featured chess game with AI opponents, built with React and Vite, featuring a high-performance C++ WebAssembly engine.

## Features

- ♟️ **Complete Chess Implementation** - All rules including castling, en passant, and promotion
- 🤖 **AI Opponents** - Play against computer with multiple difficulty levels
- ⚡ **C++ WebAssembly Engine** - High-performance chess AI (5-10x faster than JavaScript)
- 🎮 **Multiple Game Modes** - Human vs Human, Human vs AI, AI vs AI
- ⏱️ **Optional Chess Clock** - Timed games with configurable time controls
- 📜 **Move History** - Navigate through game moves and export PGN
- 🎨 **Clean UI** - Intuitive interface with move highlighting

[Try the live demo!](https://chess.george.richmnd.uk)

## Quick Start

```bash
# Install dependencies
npm install

# Run development server
npm run dev
```

The application will run with a JavaScript AI by default. For better performance, build the C++ engine (see below).

## Building the C++ Engine (Optional but Recommended)

The C++ WebAssembly engine provides 5-10x better performance than the JavaScript fallback.

### Quick Build (macOS with Homebrew)

```bash
brew install emscripten
cd Engine && chmod +x build.sh && ./build.sh
```

### Detailed Instructions

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for complete build instructions, including:
- Alternative installation methods
- Troubleshooting
- Performance comparison
- Technical details

## Project Structure

```
.
├── src/
│   ├── game/              # Chess logic
│   │   ├── types.ts       # Type definitions
│   │   ├── moveValidation.ts  # Move generation
│   │   ├── evaluate.ts    # Position evaluation (JS)
│   │   ├── myMinimax.ts   # Minimax AI (JS)
│   │   ├── wasmEngine.ts  # WebAssembly interface
│   │   └── useChessGame.ts # React hook for game state
│   ├── ChessBoard.tsx     # Board component
│   └── GameSetup.tsx      # Game configuration
├── Engine/                # C++ WebAssembly engine
│   ├── Engine.h/cpp       # Core chess engine
│   ├── bridge.cpp         # WebAssembly bindings
│   ├── CMakeLists.txt     # Build configuration
│   └── README.md          # Engine documentation
└── public/                # Static assets (WASM files go here)
```

## Technology Stack

- **Frontend**: React + TypeScript + Vite
- **Styling**: CSS Modules
- **Chess AI**: 
  - JavaScript (fallback) - Web Workers for non-blocking computation
  - C++ → WebAssembly (recommended) - Emscripten + embind
- **Algorithm**: Minimax with alpha-beta pruning

## Development

```bash
# Install dependencies
npm install

# Run dev server
npm run dev

# Build for production
npm run build

# Build C++ engine
cd Engine && ./build.sh
```

## AI Configuration

The AI supports multiple configurations:

- **Depth**: 2-5 (higher = stronger but slower)
- **Time Limit**: Max thinking time per move
- **Evaluation**: Different play styles (balanced, offensive, defensive, suicidal)

With the C++ engine, depth 4-5 is recommended for challenging gameplay.

## License

MIT

## Acknowledgments

- Chess piece logic and rules implementation
- React + TypeScript + Vite for the frontend
- Emscripten for C++ to WebAssembly compilation

