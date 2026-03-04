// Configure the WASM location before loading Stockfish
self.Module = {
  locateFile: function(path) {
    if (path.endsWith('.wasm')) {
      return '/stockfish-18-single.wasm';
    }
    return '/stockfish-18-single.js';
  }
};

// Import stockfish - it will use the Module config above
importScripts('/stockfish-18-single.js');

// Wait for Stockfish to be defined and initialize
let stockfish;

// The stockfish-18-single.js exports differently, let's handle it
if (typeof Stockfish !== 'undefined') {
  stockfish = Stockfish();
} else if (typeof INIT !== 'undefined') {
  stockfish = INIT();
} else {
  // Try to get it from the global scope or Module
  stockfish = (self.Stockfish || self.INIT || Module)();
}

// Forward messages from stockfish to main thread
stockfish.onmessage = function(event) {
  postMessage(event);
};

// Listen for messages from main thread and forward to stockfish
self.onmessage = function(e) {
  stockfish.postMessage(e.data);
};
