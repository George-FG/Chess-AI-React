// Web Worker for running chess engine computations off the main thread

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
  isCastling?: boolean;
  isEnPassant?: boolean;
}

interface ChessEngineInstance {
  setEngineVersion(version: number): void;
  getEngineVersion(): number;
  setDepth(depth: number): void;
  setMaxTime(milliseconds: number): void;
  setBoardFromArray(boardArray: (BoardSquare | null)[]): void;
  setCastlingRights(
    whiteKingSide: boolean,
    whiteQueenSide: boolean,
    blackKingSide: boolean,
    blackQueenSide: boolean
  ): void;
  findBestMove(color: string): WasmMove;
  initializeStandardPosition(): void;
  clearHistory(): void;
  delete(): void;
}

interface ChessEngineClass {
  new(): ChessEngineInstance;
  new(depth: number): ChessEngineInstance;
  new(depth: number, version: number): ChessEngineInstance;
}

interface ChessEngineModule {
  ChessEngine: ChessEngineClass;
}

// Worker message types
interface WorkerRequest {
  id: number;
  type: 'findBestMove' | 'clearHistory';
  board?: (BoardSquare | null)[];
  color?: string;
  depth?: number;
  maxTime?: number;
  version?: number;
  castlingRights?: {
    whiteKingSide: boolean;
    whiteQueenSide: boolean;
    blackKingSide: boolean;
    blackQueenSide: boolean;
  };
}

interface WorkerResponse {
  id: number;
  success: boolean;
  move?: WasmMove;
  error?: string;
}

// Module state
let wasmModule: ChessEngineModule | null = null;
let engineInstance: ChessEngineInstance | null = null;

// Load the WASM module
async function loadModule(): Promise<ChessEngineModule> {
  if (wasmModule) {
    return wasmModule;
  }

  try {
    // Pre-fetch the WASM binary
    const wasmUrl = self.location.origin + '/chess-engine.wasm';
    console.log('[Worker] Fetching WASM from:', wasmUrl);
    const wasmResponse = await fetch(wasmUrl);
    
    console.log('[Worker] WASM response status:', wasmResponse.status);
    
    if (!wasmResponse.ok) {
      const errorText = await wasmResponse.text();
      console.error('[Worker] WASM fetch failed:', errorText.substring(0, 200));
      throw new Error(`Failed to fetch WASM: ${wasmResponse.status} ${wasmResponse.statusText}`);
    }
    
    const wasmBinary = await wasmResponse.arrayBuffer();
    console.log('[Worker] WASM binary loaded, size:', wasmBinary.byteLength, 'bytes');
    
    // Fetch the WASM loader script
    const scriptResponse = await fetch('/chess-engine.js');
    const scriptText = await scriptResponse.text();
    
    // Set up Module config with the pre-loaded WASM binary
    // Disable Emscripten's automatic loading
    (self as any).Module = {
      wasmBinary: wasmBinary,
      locateFile: (path: string) => {
        console.log('[Worker] Emscripten trying to locate:', path);
        // Should not be called since we provide wasmBinary
        return self.location.origin + '/' + path;
      },
      instantiateWasm: undefined, // Let Emscripten use wasmBinary
      print: (text: string) => console.log('[WASM]', text),
      printErr: (text: string) => console.error('[WASM Error]', text),
    };
    
    // Execute the script in the global scope using indirect eval
    console.log('[Worker] Executing Emscripten script...');
    (0, eval)(scriptText);
    
    // Wait for the module factory to be available
    const globalScope = self as typeof globalThis & { ChessEngineModule?: (moduleConfig?: any) => Promise<ChessEngineModule> };
    const factory = globalScope.ChessEngineModule;
    
    if (!factory) {
      throw new Error('ChessEngineModule not found in worker context');
    }

    // Initialize the module, passing the config directly to the factory
    console.log('[Worker] Initializing WASM module...');
    wasmModule = await factory((self as any).Module);
    
    console.log('✓ Chess Engine WebAssembly module loaded in worker');
    return wasmModule;
  } catch (error) {
    console.error('Failed to load WASM module in worker:', error);
    throw error;
  }
}

// Handle messages from the main thread
self.addEventListener('message', async (event: MessageEvent<WorkerRequest>) => {
  const request = event.data;
  
  if (request.type === 'clearHistory') {
    // Clear position history for new game
    try {
      const module = await loadModule();
      if (!engineInstance) {
        engineInstance = new module.ChessEngine(3); // Default depth
      }
      engineInstance.clearHistory();
      
      const response: WorkerResponse = {
        id: request.id,
        success: true
      };
      self.postMessage(response);
    } catch (error) {
      const response: WorkerResponse = {
        id: request.id,
        success: false,
        error: error instanceof Error ? error.message : 'Unknown error'
      };
      self.postMessage(response);
    }
    return;
  }
  
  if (request.type !== 'findBestMove') {
    return;
  }

  try {
    // Load the module if not already loaded
    const module = await loadModule();
    
    // Create or reuse the engine instance
    if (!engineInstance) {
      engineInstance = new module.ChessEngine(request.depth || 3, request.version || 1);
    } else {
      // Update depth if changed
      if (request.depth !== undefined) {
        engineInstance.setDepth(request.depth);
      }
      // Update version if changed
      if (request.version !== undefined) {
        engineInstance.setEngineVersion(request.version);
      }
    }
    
    const engine = engineInstance;
    
    // Set the board state
    if (request.board) {
      engine.setBoardFromArray(request.board);
    }
    
    // Set castling rights
    if (request.castlingRights) {
      engine.setCastlingRights(
        request.castlingRights.whiteKingSide,
        request.castlingRights.whiteQueenSide,
        request.castlingRights.blackKingSide,
        request.castlingRights.blackQueenSide
      );
    }
    
    // Set max time limit (0 = no limit)
    if (request.maxTime !== undefined) {
      engine.setMaxTime(request.maxTime);
    }
    
    // Find the best move (this runs in the worker thread)
    const result = engine.findBestMove(request.color || 'white');
    
    console.log('[Worker] findBestMove result:', result);
    
    // Check if we got an empty result (no legal moves)
    if (!result || typeof result !== 'object') {
      console.error('[Worker] Engine returned no result');
      throw new Error('No legal moves available (checkmate or stalemate)');
    }
    
    // Validate that we got a valid move with valid positions
    if (!result.from || !result.to || 
        result.from.row === undefined || result.from.col === undefined ||
        result.to.row === undefined || result.to.col === undefined ||
        result.from.row < 0 || result.from.col < 0 ||
        result.to.row < 0 || result.to.col < 0) {
      console.error('[Worker] Invalid move positions:', result);
      throw new Error('No legal moves available (engine returned invalid positions)');
    }
    
    // Send the result back to the main thread
    const response: WorkerResponse = {
      id: request.id,
      success: true,
      move: result
    };
    
    self.postMessage(response);
  } catch (error) {
    // Send error back to the main thread
    const response: WorkerResponse = {
      id: request.id,
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error'
    };
    
    self.postMessage(response);
  }
});

// Pre-load the WASM module when worker starts
loadModule()
  .then(() => {
    // Signal that the worker is ready after WASM loads
    self.postMessage({ type: 'ready' });
    console.log('[Worker] Ready and WASM loaded');
  })
  .catch((error) => {
    console.error('[Worker] Failed to initialize:', error);
    self.postMessage({ type: 'error', error: String(error) });
  });
