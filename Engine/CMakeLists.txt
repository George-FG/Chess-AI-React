cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    Engine.cpp
    MinimaxEngine.cpp
    MinimaxEngineV2.cpp
    bridge.cpp
)

# Add executable
add_executable(chess_engine ${SOURCES})

# Emscripten-specific settings
if(EMSCRIPTEN)
    # Set output to .js and .wasm files
    set_target_properties(chess_engine PROPERTIES 
        OUTPUT_NAME "chess-engine"
        SUFFIX ".js"
    )
    
    # Emscripten compile and link flags
    target_compile_options(chess_engine PRIVATE
        -O3                          # Optimize for speed
        -fno-exceptions              # Disable exceptions for smaller binary
    )
    
    target_link_options(chess_engine PRIVATE
        -lembind                     # Link embind library
        -O3                          # Optimize for speed
        -sWASM=1                     # Generate WebAssembly
        -sMODULARIZE=1               # Create a module
        -sEXPORT_NAME=ChessEngineModule  # Module name
        -sALLOW_MEMORY_GROWTH=1      # Allow memory to grow
        -sENVIRONMENT=web            # Target web environment
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        --bind                       # Enable embind
    )
    
    # Set output directory
    set_target_properties(chess_engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../public"
    )
endif()
