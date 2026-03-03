#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Chess {

// Piece types
enum class PieceType : uint8_t {
    PAWN = 0,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NONE
};

enum class Color : uint8_t {
    WHITE = 0,
    BLACK = 1,
    NONE
};

struct Piece {
    PieceType type;
    Color color;
    
    Piece() : type(PieceType::NONE), color(Color::NONE) {}
    Piece(PieceType t, Color c) : type(t), color(c) {}
    
    bool isEmpty() const { return type == PieceType::NONE; }
};

struct Position {
    int row;
    int col;
    
    Position() : row(-1), col(-1) {}
    Position(int r, int c) : row(r), col(c) {}
    
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
};

struct Move {
    Position from;
    Position to;
    Piece piece;
    Piece captured;
    bool isPromotion;
    PieceType promotionType;
    bool isCastling;
    bool isEnPassant;
    
    Move() : isPromotion(false), promotionType(PieceType::NONE), 
             isCastling(false), isEnPassant(false) {}
    
    Move(Position f, Position t, Piece p) 
        : from(f), to(t), piece(p), isPromotion(false), 
          promotionType(PieceType::NONE), isCastling(false), isEnPassant(false) {}
};

struct CastlingRights {
    bool whiteKingSide;
    bool whiteQueenSide;
    bool blackKingSide;
    bool blackQueenSide;
    
    CastlingRights() : whiteKingSide(true), whiteQueenSide(true),
                       blackKingSide(true), blackQueenSide(true) {}
};

// 8x8 chess board
class Board {
public:
    Piece squares[8][8];
    
    Board();
    void initializeStandardPosition();
    Piece getPiece(Position pos) const;
    void setPiece(Position pos, Piece piece);
    bool isEmpty(Position pos) const;
    Board clone() const;
    void applyMove(const Move& move);
};

class MoveGenerator {
public:
    static std::vector<Move> generateMoves(const Board& board, Color color, 
                                          const CastlingRights& castling);
    static std::vector<Position> getValidMovesForPiece(const Board& board, Position from, 
                                                       const Piece& piece);
    static bool isKingInCheck(const Board& board, Color color);
    static bool wouldMoveResultInCheck(const Board& board, const Move& move);
    
private:
    static std::vector<Position> getPawnMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getKnightMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getBishopMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getRookMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getQueenMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getKingMoves(const Board& board, Position from, Color color);
    static std::vector<Position> getLinearMoves(const Board& board, Position from, Color color,
                                               const std::vector<std::pair<int, int>>& directions);
};

class Evaluator {
public:
    static int evaluate(const Board& board, Color aiColor);
    
private:
    static const int PIECE_VALUES[7];
};

class MinimaxEngine {
public:
    MinimaxEngine(int depth = 3);
    
    Move findBestMove(const Board& board, Color color, const CastlingRights& castling);
    void setDepth(int depth);
    
private:
    int depth_;
    Color rootColor_;
    
    int minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                int alpha, int beta, const CastlingRights& castling);
    CastlingRights updateCastlingRights(const CastlingRights& rights, const Move& move, 
                                       const Board& board);
};

} // namespace Chess

#endif // ENGINE_H
