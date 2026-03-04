import type { Board, PieceColor, Move, Position, CastlingRights, PieceType } from './types';

// Declare Stockfish type from stockfish.wasm
interface StockfishInterface {
  postMessage: (message: string) => void;
  addMessageListener: (listener: (message: string) => void) => void;
}

declare global {
  interface Window {
    Stockfish: () => Promise<StockfishInterface>;
  }
}

let stockfishEngine: StockfishInterface | null = null;
let engineReady = false;
let pendingCallback: ((move: Move | null) => void) | null = null;

/**
 * Load Stockfish script dynamically
 */
function loadStockfishScript(): Promise<void> {
  return new Promise((resolve, reject) => {
    // Check if already loaded
    if (typeof window.Stockfish === 'function') {
      resolve();
      return;
    }
    
    const script = document.createElement('script');
    script.src = '/stockfish.js';
    script.onload = () => resolve();
    script.onerror = () => reject(new Error('Failed to load Stockfish'));
    document.head.appendChild(script);
  });
}

/**
 * Initialize Stockfish engine
 */
async function initializeStockfish(): Promise<StockfishInterface> {
  if (!stockfishEngine) {
    // Load the script first
    await loadStockfishScript();
    
    // Initialize the engine
    stockfishEngine = await window.Stockfish();
    
    stockfishEngine.addMessageListener((message: string) => {
      // Engine ready
      if (message.includes('uciok')) {
        engineReady = true;
        console.log('✓ Stockfish engine ready');
      }
      
      // Best move found
      if (message.startsWith('bestmove')) {
        handleBestMove(message);
      }
      
      // Log info messages (optional)
      if (message.startsWith('info')) {
        // You can parse depth, score, etc. here if needed
        // console.log('Stockfish info:', message);
      }
    });
    
    // Initialize the engine
    stockfishEngine.postMessage('uci');
  }
  
  return stockfishEngine;
}

/**
 * Convert board position to FEN (Forsyth-Edwards Notation)
 */
function boardToFEN(board: Board, currentPlayer: PieceColor, castlingRights: CastlingRights, moveHistory: Move[]): string {
  // Convert board to FEN format
  let fen = '';
  
  // 1. Piece placement
  for (let row = 7; row >= 0; row--) {
    let emptyCount = 0;
    for (let col = 0; col < 8; col++) {
      const piece = board[row][col];
      if (piece) {
        if (emptyCount > 0) {
          fen += emptyCount;
          emptyCount = 0;
        }
        // Map piece type to FEN character
        const pieceChar = piece.type.toUpperCase();
        fen += piece.color === 'white' ? pieceChar : pieceChar.toLowerCase();
      } else {
        emptyCount++;
      }
    }
    if (emptyCount > 0) {
      fen += emptyCount;
    }
    if (row > 0) {
      fen += '/';
    }
  }
  
  // 2. Active color
  fen += ' ' + (currentPlayer === 'white' ? 'w' : 'b');
  
  // 3. Castling availability
  let castling = '';
  if (castlingRights.whiteKingSide) castling += 'K';
  if (castlingRights.whiteQueenSide) castling += 'Q';
  if (castlingRights.blackKingSide) castling += 'k';
  if (castlingRights.blackQueenSide) castling += 'q';
  fen += ' ' + (castling || '-');
  
  // 4. En passant target square
  const lastMove = moveHistory[moveHistory.length - 1];
  let enPassant = '-';
  if (lastMove?.piece.type === 'p') {
    const rowDiff = Math.abs(lastMove.to.row - lastMove.from.row);
    if (rowDiff === 2) {
      const targetRow = lastMove.piece.color === 'white' ? lastMove.from.row + 1 : lastMove.from.row - 1;
      enPassant = positionToAlgebraic({ row: targetRow, col: lastMove.to.col });
    }
  }
  fen += ' ' + enPassant;
  
  // 5. Halfmove clock (simplified - always 0 for now)
  fen += ' 0';
  
  // 6. Fullmove number
  const fullmoveNumber = Math.floor(moveHistory.length / 2) + 1;
  fen += ' ' + fullmoveNumber;
  
  return fen;
}

/**
 * Convert position to algebraic notation (e.g., {row: 0, col: 4} -> "e1")
 */
function positionToAlgebraic(pos: Position): string {
  const file = String.fromCharCode(97 + pos.col); // 'a' to 'h'
  const rank = (pos.row + 1).toString(); // '1' to '8'
  return file + rank;
}

/**
 * Convert algebraic notation to position (e.g., "e1" -> {row: 0, col: 4})
 */
function algebraicToPosition(algebraic: string): Position {
  const file = algebraic.charCodeAt(0) - 97; // 'a' = 0, 'h' = 7
  const rank = parseInt(algebraic[1]) - 1; // '1' = 0, '8' = 7
  return { row: rank, col: file };
}

/**
 * Handle best move response from Stockfish
 */
function handleBestMove(message: string) {
  if (!pendingCallback) return;
  
  // Parse "bestmove e2e4" or "bestmove e7e8q" (promotion)
  const parts = message.split(' ');
  if (parts.length < 2) {
    pendingCallback(null);
    pendingCallback = null;
    return;
  }
  
  const moveStr = parts[1];
  if (moveStr === '(none)' || !moveStr) {
    pendingCallback(null);
    pendingCallback = null;
    return;
  }
  
  // Parse the move (e.g., "e2e4" or "e7e8q")
  const fromSquare = moveStr.substring(0, 2);
  const toSquare = moveStr.substring(2, 4);
  const promotion = moveStr.length > 4 ? moveStr[4] : undefined;
  
  const from = algebraicToPosition(fromSquare);
  const to = algebraicToPosition(toSquare);
  
  // Get the piece being moved (we'll need the board for this)
  // For now, return a basic move structure
  const move: Move = {
    from,
    to,
    piece: { type: 'p', color: 'white' }, // This will be filled by the caller
    promotion: promotion as PieceType | undefined,
  };
  
  pendingCallback(move);
  pendingCallback = null;
}

/**
 * Find the best move using Stockfish
 */
export async function findBestMoveStockfish(
  board: Board,
  currentPlayer: PieceColor,
  castlingRights: CastlingRights,
  moveHistory: Move[],
  depth: number = 10,
  maxTime: number = 3000
): Promise<Move | null> {
  // Initialize engine and wait for it to be ready
  await initializeStockfish();
  
  // Wait for engine to be ready
  if (!engineReady) {
    await new Promise<void>((resolve) => {
      const checkReady = setInterval(() => {
        if (engineReady) {
          clearInterval(checkReady);
          resolve();
        }
      }, 100);
    });
  }
  
  return new Promise((resolve) => {
    pendingCallback = (move) => {
      if (move) {
        // Fill in the actual piece information from the board
        const piece = board[move.from.row][move.from.col];
        const captured = board[move.to.row][move.to.col];
        
        if (piece) {
          move.piece = piece;
          if (captured) {
            move.captured = captured;
          }
          
          // Check for castling
          if (piece.type === 'k' && Math.abs(move.to.col - move.from.col) === 2) {
            move.isCastling = true;
          }
          
          // Check for en passant
          if (piece.type === 'p' && move.to.col !== move.from.col && !captured) {
            move.isEnPassant = true;
          }
        }
      }
      resolve(move);
    };
    
    // Convert board to FEN
    const fen = boardToFEN(board, currentPlayer, castlingRights, moveHistory);
    
    // Send position to Stockfish
    if (stockfishEngine) {
      stockfishEngine.postMessage('position fen ' + fen);
      
      // Start search with depth or time limit
      if (maxTime > 0) {
        stockfishEngine.postMessage(`go movetime ${maxTime}`);
      } else {
        stockfishEngine.postMessage(`go depth ${depth}`);
      }
    }
  });
}

/**
 * Terminate the Stockfish engine
 */
export function terminateStockfish() {
  if (stockfishEngine) {
    // Stockfish.wasm doesn't have a terminate method
    // Just set to null to allow garbage collection
    stockfishEngine = null;
    engineReady = false;
  }
}
