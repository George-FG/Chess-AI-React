import type { Board, PieceColor, Move, PieceType } from './types';

// TypeScript declarations for the WebAssembly module

interface BoardSquare {
  type: string;
  color: string;
}

// Worker management
let engineWorker: Worker | null = null;
let nextRequestId = 1;
const pendingRequests = new Map<number, {
  resolve: (move: Move | null) => void;
  reject: (error: Error) => void;
}>();

/**
 * Initialize the chess engine worker
 */
function getWorker(): Worker {
  if (!engineWorker) {
    // Create the worker as a module worker
    engineWorker = new Worker(
      new URL('./chessEngineWorker.ts', import.meta.url),
      { type: 'module' }
    );
    
    // Handle messages from the worker
    engineWorker.addEventListener('message', (event) => {
      const response = event.data;
      
      // Skip ready message
      if (response.type === 'ready') {
        console.log('✓ Chess Engine worker ready');
        return;
      }
      
      // Handle move response
      const pending = pendingRequests.get(response.id);
      if (!pending) return;
      
      pendingRequests.delete(response.id);
      
      if (response.success && response.move) {
        console.log('✓ Move received from worker:', response.move);
        // Convert the result to our Move type
        const move: Move = {
          from: {
            row: response.move.from.row,
            col: response.move.from.col
          },
          to: {
            row: response.move.to.row,
            col: response.move.to.col
          },
          piece: {
            type: response.move.piece.type as PieceType,
            color: response.move.piece.color as PieceColor
          },
          captured: response.move.captured ? {
            type: response.move.captured.type as PieceType,
            color: response.move.captured.color as PieceColor
          } : undefined,
          promotion: response.move.promotion ? response.move.promotion as PieceType : undefined,
          isCastling: response.move.isCastling,
          isEnPassant: response.move.isEnPassant
        };
        pending.resolve(move);
      } else {
        console.error('✗ Move failed from worker:', response.error);
        pending.reject(new Error(response.error || 'Failed to find best move'));
      }
    });
    
    // Handle worker errors
    engineWorker.addEventListener('error', (error) => {
      console.error('Chess Engine worker error:', error);
      // Reject all pending requests
      pendingRequests.forEach(({ reject }) => {
        reject(new Error('Worker error'));
      });
      pendingRequests.clear();
    });
  }
  
  return engineWorker;
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
  maxTime?: number; // Max time in milliseconds (0 = no limit)
  version?: number; // Engine version: 1 (default, alpha-beta) or 2 (simple minimax with castling)
  castlingRights: {
    whiteKingSide: boolean;
    whiteQueenSide: boolean;
    blackKingSide: boolean;
    blackQueenSide: boolean;
  };
}

/**
 * Find the best move using the C++ WebAssembly engine (running in a worker)
 */
export async function findBestMoveWasm(request: BestMoveRequest): Promise<Move | null> {
  try {
    const worker = getWorker();
    
    // Convert board to array format
    const boardArray = boardToArray(request.board);
    
    // Create a unique request ID
    const requestId = nextRequestId++;
    
    // Create a promise that will be resolved when the worker responds
    const promise = new Promise<Move | null>((resolve, reject) => {
      pendingRequests.set(requestId, { resolve, reject });
      
      // Set a timeout to prevent hanging forever
      // Use generous timeout for first few requests (WASM loading)
      const timeoutDuration = Math.max((request.maxTime || 30000) + 10000, 60000);
      const timeout = setTimeout(() => {
        pendingRequests.delete(requestId);
        reject(new Error('Worker request timeout'));
      }, timeoutDuration);
      
      // Clear timeout when promise resolves/rejects
      const originalResolve = resolve;
      const originalReject = reject;
      
      pendingRequests.set(requestId, {
        resolve: (move) => {
          clearTimeout(timeout);
          originalResolve(move);
        },
        reject: (error) => {
          clearTimeout(timeout);
          originalReject(error);
        }
      });
    });
    
    // Send the request to the worker
    worker.postMessage({
      id: requestId,
      type: 'findBestMove',
      board: boardArray,
      color: request.color,
      depth: request.depth,
      maxTime: request.maxTime,
      version: request.version || 1,
      castlingRights: request.castlingRights
    });
    
    return await promise;
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

/**
 * Clear the position history in the engine (call when starting a new game)
 */
export async function clearEngineHistory(): Promise<void> {
  try {
    const worker = getWorker();
    const requestId = nextRequestId++;
    
    const promise = new Promise<void>((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error('Clear history timeout'));
      }, 5000);
      
      const handler = (event: MessageEvent) => {
        const response = event.data;
        if (response.id === requestId) {
          clearTimeout(timeout);
          worker.removeEventListener('message', handler);
          if (response.success) {
            resolve();
          } else {
            reject(new Error(response.error || 'Failed to clear history'));
          }
        }
      };
      
      worker.addEventListener('message', handler);
    });
    
    worker.postMessage({
      id: requestId,
      type: 'clearHistory'
    });
    
    await promise;
  } catch (error) {
    console.error('Error clearing engine history:', error);
  }
}
