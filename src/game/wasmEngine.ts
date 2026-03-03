import type { Board, PieceColor, Move, PieceType } from './types';

// Extend Window interface for the global ChessEngineModule
declare global {
  interface Window {
    ChessEngineModule?: () => Promise<ChessEngineModule>;
  }
}

// TypeScript declarations for the WebAssembly module

interface BoardSquare {
  type: string;
  color: string;
}

interface WasmPosition {
  row: number;
  col: number;
}

interface WasmPiece {
  type: string;
  color: string;
}

interface WasmMove {
  from: WasmPosition;
  to: WasmPosition;
  piece: WasmPiece;
  captured?: WasmPiece;
  promotion?: string;
}

interface ChessEngineInstance {
  setDepth(depth: number): void;
  setBoardFromArray(boardArray: (BoardSquare | null)[]): void;
  setCastlingRights(
    whiteKingSide: boolean,
    whiteQueenSide: boolean,
    blackKingSide: boolean,
    blackQueenSide: boolean
  ): void;
  findBestMove(color: string): WasmMove;
  initializeStandardPosition(): void;
  delete(): void;
}

interface ChessEngineClass {
  new(): ChessEngineInstance;
  new(depth: number): ChessEngineInstance;
}

interface ChessEngineModule {
  ChessEngine: ChessEngineClass;
}

// Singleton to hold the loaded WebAssembly module
let wasmModule: ChessEngineModule | null = null;
let moduleLoadPromise: Promise<ChessEngineModule> | null = null;

/**
 * Load the WebAssembly chess engine module
 */
export async function loadChessEngine(): Promise<ChessEngineModule> {
  if (wasmModule) {
    return wasmModule;
  }

  if (moduleLoadPromise) {
    return moduleLoadPromise;
  }

  moduleLoadPromise = (async () => {
    try {
      // Load the WebAssembly module from public directory
      // Check if already loaded
      if (!window.ChessEngineModule) {
        // Check if script is already in the DOM
        const existingScript = document.querySelector('script[src="/chess-engine.js"]');
        
        if (!existingScript) {
          const script = document.createElement('script');
          script.src = '/chess-engine.js';
          
          // Wait for the script to load and the module factory to be available
          await new Promise<void>((resolve, reject) => {
            script.onload = () => resolve();
            script.onerror = () => reject(new Error('Failed to load chess-engine.js'));
            document.head.appendChild(script);
          });
        } else {
          // Script exists but might still be loading
          await new Promise<void>((resolve) => {
            const checkInterval = setInterval(() => {
              if (window.ChessEngineModule) {
                clearInterval(checkInterval);
                resolve();
              }
            }, 50);
            // Timeout after 5 seconds
            setTimeout(() => {
              clearInterval(checkInterval);
              resolve();
            }, 5000);
          });
        }
      }
      
      // The Emscripten module should now be available
      const factory = window.ChessEngineModule;
      
      if (!factory) {
        throw new Error('ChessEngineModule not found');
      }
      
      // Initialize the module
      wasmModule = await factory();
      
      console.log('✓ Chess Engine WebAssembly module loaded successfully');
      return wasmModule as ChessEngineModule;
    } catch (error) {
      console.warn('Chess Engine WebAssembly module not available:', error);
      console.warn('The C++ engine needs to be built. See Engine/README.md for instructions.');
      throw new Error('WASM_NOT_AVAILABLE');
    }
  })();

  return moduleLoadPromise;
}

/**
 * Convert the TypeScript Board to a flat array for WebAssembly
 */
function boardToArray(board: Board): (BoardSquare | null)[] {
  const array: (BoardSquare | null)[] = [];
  
  for (let row = 0; row < 8; row++) {
    for (let col = 0; col < 8; col++) {
      const piece = board[row][col];
      if (piece) {
        array.push({
          type: piece.type,
          color: piece.color
        });
      } else {
        array.push(null);
      }
    }
  }
  
  return array;
}

/**
 * Interface for the best move request
 */
export interface BestMoveRequest {
  board: Board;
  color: PieceColor;
  depth: number;
  castlingRights: {
    whiteKingSide: boolean;
    whiteQueenSide: boolean;
    blackKingSide: boolean;
    blackQueenSide: boolean;
  };
}

/**
 * Find the best move using the C++ WebAssembly engine
 */
export async function findBestMoveWasm(request: BestMoveRequest): Promise<Move | null> {
  try {
    // Load the WebAssembly module if not already loaded
    const module = await loadChessEngine();
    
    // Create a new engine instance
    const engine = new module.ChessEngine(request.depth);
    
    try {
      // Convert board to array format
      const boardArray = boardToArray(request.board);
      
      // Set the board state
      engine.setBoardFromArray(boardArray);
      
      // Set castling rights
      engine.setCastlingRights(
        request.castlingRights.whiteKingSide,
        request.castlingRights.whiteQueenSide,
        request.castlingRights.blackKingSide,
        request.castlingRights.blackQueenSide
      );
      
      // Find the best move
      const result = engine.findBestMove(request.color);
      
      // Check if we got a valid move
      if (!result || !result.from || !result.to || !result.piece) {
        return null;
      }
      
      // Convert the result to our Move type
      const move: Move = {
        from: {
          row: result.from.row,
          col: result.from.col
        },
        to: {
          row: result.to.row,
          col: result.to.col
        },
        piece: {
          type: result.piece.type as PieceType,
          color: result.piece.color as PieceColor
        },
        captured: result.captured ? {
          type: result.captured.type as PieceType,
          color: result.captured.color as PieceColor
        } : undefined,
        promotion: result.promotion ? result.promotion as PieceType : undefined
      };
      
      return move;
    } finally {
      // Clean up the engine instance
      engine.delete();
    }
  } catch (error) {
    console.error('Error in findBestMoveWasm:', error);
    return null;
  }
}

/**
 * Check if WebAssembly is supported in the browser
 */
export function isWasmSupported(): boolean {
  try {
    if (typeof WebAssembly === 'object' &&
        typeof WebAssembly.instantiate === 'function') {
      const module = new WebAssembly.Module(
        Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00)
      );
      if (module instanceof WebAssembly.Module) {
        return new WebAssembly.Instance(module) instanceof WebAssembly.Instance;
      }
    }
  } catch {
    // WebAssembly not supported
  }
  return false;
}
