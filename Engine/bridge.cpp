#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "Engine.h"
#include "MinimaxEngine.h"
#include "MinimaxEngineV2.h"
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
    MinimaxEngine engineV1_;
    MinimaxEngineV2 engineV2_;
    CastlingRights castling_;
    std::vector<std::string> positionHistory_;
    int engineVersion_; // 1 for V1, 2 for V2
    bool whiteHasCastled_;
    bool blackHasCastled_;
    
public:
    ChessEngineWrapper(int depth = 3, int version = 1) 
        : engineV1_(depth), engineV2_(depth), engineVersion_(version),
          whiteHasCastled_(false), blackHasCastled_(false) {
        board_.initializeStandardPosition();
    }
    
    void setEngineVersion(int version) {
        engineVersion_ = (version == 2) ? 2 : 1;
    }
    
    int getEngineVersion() const {
        return engineVersion_;
    }
    
    void setDepth(int depth) {
        engineV1_.setDepth(depth);
        engineV2_.setDepth(depth);
    }
    
    void setMaxTime(int milliseconds) {
        engineV1_.setMaxTime(milliseconds);
        engineV2_.setMaxTime(milliseconds);
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
        Move bestMove;
        
        // Use the appropriate engine version
        if (engineVersion_ == 2) {
            bestMove = engineV2_.findBestMove(board_, color, castling_, positionHistory_,
                                              whiteHasCastled_, blackHasCastled_);
        } else {
            bestMove = engineV1_.findBestMove(board_, color, castling_, positionHistory_);
        }
        
        // Track if this move is a castling move
        if (bestMove.isCastling) {
            if (color == Color::WHITE) {
                whiteHasCastled_ = true;
            } else {
                blackHasCastled_ = true;
            }
        }
        
        // Update position history after move
        Board newBoard = board_.clone();
        newBoard.applyMove(bestMove);
        std::string newPosHash = (engineVersion_ == 2) 
            ? MinimaxEngineV2::getPositionHash(newBoard)
            : MinimaxEngine::getPositionHash(newBoard);
        positionHistory_.push_back(newPosHash);
        
        // Keep only last 16 positions to avoid unbounded growth
        if (positionHistory_.size() > 16) {
            positionHistory_.erase(positionHistory_.begin());
        }
        
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
            result.set("searchDepth", bestMove.searchDepth); // Include search depth
            
            if (!bestMove.captured.isEmpty()) {
                val captured = val::object();
                captured.set("type", pieceTypeToString(bestMove.captured.type));
                captured.set("color", colorToString(bestMove.captured.color));
                result.set("captured", captured);
            }
            
            if (bestMove.isPromotion) {
                result.set("promotion", pieceTypeToString(bestMove.promotionType));
            }
            
            if (bestMove.isCastling) {
                result.set("isCastling", true);
            }
            
            if (bestMove.isEnPassant) {
                result.set("isEnPassant", true);
            }
        }
        
        return result;
    }
    
    // Initialize to standard chess position
    void initializeStandardPosition() {
        board_.initializeStandardPosition();
        castling_ = CastlingRights();
        positionHistory_.clear();
        whiteHasCastled_ = false;
        blackHasCastled_ = false;
    }
    
    // Clear position history (for new games)
    void clearHistory() {
        positionHistory_.clear();
        whiteHasCastled_ = false;
        blackHasCastled_ = false;
    }
};

// Bind the wrapper class to JavaScript
EMSCRIPTEN_BINDINGS(chess_engine) {
    class_<ChessEngineWrapper>("ChessEngine")
        .constructor<>()
        .constructor<int>()
        .constructor<int, int>()
        .function("setEngineVersion", &ChessEngineWrapper::setEngineVersion)
        .function("getEngineVersion", &ChessEngineWrapper::getEngineVersion)
        .function("setDepth", &ChessEngineWrapper::setDepth)
        .function("setMaxTime", &ChessEngineWrapper::setMaxTime)
        .function("setBoardFromArray", &ChessEngineWrapper::setBoardFromArray)
        .function("setCastlingRights", &ChessEngineWrapper::setCastlingRights)
        .function("findBestMove", &ChessEngineWrapper::findBestMove)
        .function("initializeStandardPosition", &ChessEngineWrapper::initializeStandardPosition)
        .function("clearHistory", &ChessEngineWrapper::clearHistory);
}
