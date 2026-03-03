#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "Engine.h"
#include "MinimaxEngine.h"
#include <string>
#include <sstream>

using namespace emscripten;
using namespace Chess;

// Helper functions to convert between C++ and JavaScript types

// Convert a piece type to string
std::string pieceTypeToString(PieceType type) {
    switch (type) {
        case PieceType::PAWN: return "p";
        case PieceType::KNIGHT: return "n";
        case PieceType::BISHOP: return "b";
        case PieceType::ROOK: return "r";
        case PieceType::QUEEN: return "q";
        case PieceType::KING: return "k";
        default: return "";
    }
}

// Convert a string to piece type
PieceType stringToPieceType(const std::string& str) {
    if (str == "p") return PieceType::PAWN;
    if (str == "n") return PieceType::KNIGHT;
    if (str == "b") return PieceType::BISHOP;
    if (str == "r") return PieceType::ROOK;
    if (str == "q") return PieceType::QUEEN;
    if (str == "k") return PieceType::KING;
    return PieceType::NONE;
}

// Convert a color to string
std::string colorToString(Color color) {
    return (color == Color::WHITE) ? "white" : "black";
}

// Convert a string to color
Color stringToColor(const std::string& str) {
    return (str == "white") ? Color::WHITE : Color::BLACK;
}

// Wrapper class for the engine that provides JavaScript-friendly interface
class ChessEngineWrapper {
private:
    Board board_;
    MinimaxEngine engine_;
    CastlingRights castling_;
    
public:
    ChessEngineWrapper(int depth = 3) : engine_(depth) {
        board_.initializeStandardPosition();
    }
    
    void setDepth(int depth) {
        engine_.setDepth(depth);
    }
    
    void setMaxTime(int milliseconds) {
        engine_.setMaxTime(milliseconds);
    }
    
    // Set board from JavaScript array
    // Expected format: array of 64 objects with {type: string, color: string} or null
    void setBoardFromArray(val boardArray) {
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int index = row * 8 + col;
                val square = boardArray[index];
                
                if (square.isNull() || square.isUndefined()) {
                    board_.setPiece(Position(row, col), Piece());
                } else {
                    std::string typeStr = square["type"].as<std::string>();
                    std::string colorStr = square["color"].as<std::string>();
                    
                    PieceType type = stringToPieceType(typeStr);
                    Color color = stringToColor(colorStr);
                    
                    board_.setPiece(Position(row, col), Piece(type, color));
                }
            }
        }
    }
    
    // Set castling rights
    void setCastlingRights(bool whiteKingSide, bool whiteQueenSide, 
                          bool blackKingSide, bool blackQueenSide) {
        castling_.whiteKingSide = whiteKingSide;
        castling_.whiteQueenSide = whiteQueenSide;
        castling_.blackKingSide = blackKingSide;
        castling_.blackQueenSide = blackQueenSide;
    }
    
    // Find best move and return as JavaScript object
    val findBestMove(const std::string& colorStr) {
        Color color = stringToColor(colorStr);
        Move bestMove = engine_.findBestMove(board_, color, castling_);
        
        // Convert move to JavaScript object
        val result = val::object();
        
        if (bestMove.from.isValid() && bestMove.to.isValid()) {
            val from = val::object();
            from.set("row", bestMove.from.row);
            from.set("col", bestMove.from.col);
            
            val to = val::object();
            to.set("row", bestMove.to.row);
            to.set("col", bestMove.to.col);
            
            val piece = val::object();
            piece.set("type", pieceTypeToString(bestMove.piece.type));
            piece.set("color", colorToString(bestMove.piece.color));
            
            result.set("from", from);
            result.set("to", to);
            result.set("piece", piece);
            
            if (!bestMove.captured.isEmpty()) {
                val captured = val::object();
                captured.set("type", pieceTypeToString(bestMove.captured.type));
                captured.set("color", colorToString(bestMove.captured.color));
                result.set("captured", captured);
            }
            
            if (bestMove.isPromotion) {
                result.set("promotion", pieceTypeToString(bestMove.promotionType));
            }
        }
        
        return result;
    }
    
    // Initialize to standard chess position
    void initializeStandardPosition() {
        board_.initializeStandardPosition();
        castling_ = CastlingRights();
    }
};

// Bind the wrapper class to JavaScript
EMSCRIPTEN_BINDINGS(chess_engine) {
    class_<ChessEngineWrapper>("ChessEngine")
        .constructor<>()
        .constructor<int>()
        .function("setDepth", &ChessEngineWrapper::setDepth)
        .function("setMaxTime", &ChessEngineWrapper::setMaxTime)
        .function("setBoardFromArray", &ChessEngineWrapper::setBoardFromArray)
        .function("setCastlingRights", &ChessEngineWrapper::setCastlingRights)
        .function("findBestMove", &ChessEngineWrapper::findBestMove)
        .function("initializeStandardPosition", &ChessEngineWrapper::initializeStandardPosition);
}
