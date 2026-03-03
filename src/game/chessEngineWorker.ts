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
}

interface ChessEngineInstance {
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
  delete(): void;
}

interface ChessEngineClass {
  new(): ChessEngineInstance;
  new(depth: number): ChessEngineInstance;
}

interface ChessEngineV2Class {
  new(): ChessEngineInstance;
  new(depth: number): ChessEngineInstance;
}

interface ChessEngineModule {
  ChessEngine: ChessEngineClass;
  ChessEngineV2: ChessEngineV2Class;
}

// Worker message types
interface WorkerRequest {
  id: number;
  type: 'findBestMove';
  board: (BoardSquare | null)[];
  color: string;
  depth: number;
  maxTime?: number;
  engineVersion: 'v1' | 'v2' | 'v3';
  castlingRights: {
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
  
  if (request.type !== 'findBestMove') {
    return;
  }

  try {
    // Load the module if not already loaded
    const module = await loadModule();
    
    // Select the engine version (default to v1)
    const engineVersion = request.engineVersion || 'v1';
    
    // Create a new engine instance based on version
    const engine = engineVersion === 'v2' 
      ? new module.ChessEngineV2(request.depth)
      : new module.ChessEngine(request.depth);
    
    try {
      // Set the board state
      engine.setBoardFromArray(request.board);
      
      // Set castling rights
      engine.setCastlingRights(
        request.castlingRights.whiteKingSide,
        request.castlingRights.whiteQueenSide,
        request.castlingRights.blackKingSide,
        request.castlingRights.blackQueenSide
      );
      
      // Set max time limit (0 = no limit)
      if (request.maxTime !== undefined) {
        engine.setMaxTime(request.maxTime);
      }
      
      // Find the best move (this runs in the worker thread)
      const result = engine.findBestMove(request.color);
      
      // Send the result back to the main thread
      const response: WorkerResponse = {
        id: request.id,
        success: true,
        move: result
      };
      
      self.postMessage(response);
    } finally {
      // Clean up the engine instance
      engine.delete();
    }
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

// Signal that the worker is ready
self.postMessage({ type: 'ready' });
