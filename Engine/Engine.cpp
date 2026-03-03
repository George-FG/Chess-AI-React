#include "Engine.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace Chess {

// Board implementation
Board::Board() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            squares[i][j] = Piece();
        }
    }
}

void Board::initializeStandardPosition() {
    // Clear board
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            squares[i][j] = Piece();
        }
    }
    
    // Set up pawns
    for (int i = 0; i < 8; i++) {
        squares[1][i] = Piece(PieceType::PAWN, Color::WHITE);
        squares[6][i] = Piece(PieceType::PAWN, Color::BLACK);
    }
    
    // Set up other pieces (King and Queen swapped to match TypeScript with display reversal)
    PieceType backRow[8] = {
        PieceType::ROOK, PieceType::KNIGHT, PieceType::BISHOP, PieceType::KING,
        PieceType::QUEEN, PieceType::BISHOP, PieceType::KNIGHT, PieceType::ROOK
    };
    
    for (int i = 0; i < 8; i++) {
        squares[0][i] = Piece(backRow[i], Color::WHITE);
        squares[7][i] = Piece(backRow[i], Color::BLACK);
    }
}

Piece Board::getPiece(Position pos) const {
    if (!pos.isValid()) return Piece();
    return squares[pos.row][pos.col];
}

void Board::setPiece(Position pos, Piece piece) {
    if (pos.isValid()) {
        squares[pos.row][pos.col] = piece;
    }
}

bool Board::isEmpty(Position pos) const {
    return getPiece(pos).isEmpty();
}

Board Board::clone() const {
    Board copy;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            copy.squares[i][j] = squares[i][j];
        }
    }
    return copy;
}

void Board::applyMove(const Move& move) {
    // Handle en passant
    if (move.isEnPassant) {
        setPiece(move.to, move.piece);
        setPiece(move.from, Piece());
        // Remove captured pawn
        Position capturedPawnPos(move.from.row, move.to.col);
        setPiece(capturedPawnPos, Piece());
        return;
    }
    
    // Handle castling
    if (move.isCastling) {
        setPiece(move.to, move.piece);
        setPiece(move.from, Piece());
        
        // Move the rook
        if (move.to.col < move.from.col) { // King side (King moves left in data: 3→1)
            Piece rook = getPiece(Position(move.from.row, 0));
            setPiece(Position(move.from.row, 0), Piece());
            setPiece(Position(move.from.row, 2), rook);
        } else { // Queen side (King moves right in data: 3→5)
            Piece rook = getPiece(Position(move.from.row, 7));
            setPiece(Position(move.from.row, 7), Piece());
            setPiece(Position(move.from.row, 4), rook);
        }
        return;
    }
    
    // Handle promotion
    if (move.isPromotion) {
        Piece promoted(move.promotionType, move.piece.color);
        setPiece(move.to, promoted);
        setPiece(move.from, Piece());
        return;
    }
    
    // Normal move
    setPiece(move.to, move.piece);
    setPiece(move.from, Piece());
}

// MoveGenerator implementation
std::vector<Position> MoveGenerator::getLinearMoves(
    const Board& board, Position from, Color color,
    const std::vector<std::pair<int, int>>& directions) {
    
    std::vector<Position> moves;
    
    for (const auto& dir : directions) {
        int row = from.row + dir.first;
        int col = from.col + dir.second;
        
        while (Position(row, col).isValid()) {
            Piece target = board.getPiece(Position(row, col));
            
            if (target.isEmpty()) {
                moves.push_back(Position(row, col));
            } else {
                if (target.color != color) {
                    moves.push_back(Position(row, col));
                }
                break;
            }
            row += dir.first;
            col += dir.second;
        }
    }
    
    return moves;
}

std::vector<Position> MoveGenerator::getPawnMoves(const Board& board, Position from, Color color) {
    std::vector<Position> moves;
    int direction = (color == Color::WHITE) ? 1 : -1;
    int startRow = (color == Color::WHITE) ? 1 : 6;
    
    // Move forward one square
    Position oneForward(from.row + direction, from.col);
    if (oneForward.isValid() && board.isEmpty(oneForward)) {
        moves.push_back(oneForward);
        
        // Move forward two squares from starting position
        if (from.row == startRow) {
            Position twoForward(from.row + 2 * direction, from.col);
            if (twoForward.isValid() && board.isEmpty(twoForward)) {
                moves.push_back(twoForward);
            }
        }
    }
    
    // Capture diagonally
    Position captureLeft(from.row + direction, from.col - 1);
    Position captureRight(from.row + direction, from.col + 1);
    
    for (const Position& pos : {captureLeft, captureRight}) {
        if (pos.isValid()) {
            Piece target = board.getPiece(pos);
            if (!target.isEmpty() && target.color != color) {
                moves.push_back(pos);
            }
        }
    }
    
    return moves;
}

std::vector<Position> MoveGenerator::getKnightMoves(const Board& board, Position from, Color color) {
    std::vector<Position> moves;
    int offsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (int i = 0; i < 8; i++) {
        Position pos(from.row + offsets[i][0], from.col + offsets[i][1]);
        if (pos.isValid()) {
            Piece target = board.getPiece(pos);
            if (target.isEmpty() || target.color != color) {
                moves.push_back(pos);
            }
        }
    }
    
    return moves;
}

std::vector<Position> MoveGenerator::getBishopMoves(const Board& board, Position from, Color color) {
    std::vector<std::pair<int, int>> directions = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    return getLinearMoves(board, from, color, directions);
}

std::vector<Position> MoveGenerator::getRookMoves(const Board& board, Position from, Color color) {
    std::vector<std::pair<int, int>> directions = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    return getLinearMoves(board, from, color, directions);
}

std::vector<Position> MoveGenerator::getQueenMoves(const Board& board, Position from, Color color) {
    std::vector<std::pair<int, int>> directions = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1}, {0, 1},
        {1, -1}, {1, 0}, {1, 1}
    };
    return getLinearMoves(board, from, color, directions);
}

std::vector<Position> MoveGenerator::getKingMoves(const Board& board, Position from, Color color) {
    std::vector<Position> moves;
    int offsets[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1}, {0, 1},
        {1, -1}, {1, 0}, {1, 1}
    };
    
    for (int i = 0; i < 8; i++) {
        Position pos(from.row + offsets[i][0], from.col + offsets[i][1]);
        if (pos.isValid()) {
            Piece target = board.getPiece(pos);
            if (target.isEmpty() || target.color != color) {
                moves.push_back(pos);
            }
        }
    }
    
    return moves;
}

std::vector<Position> MoveGenerator::getValidMovesForPiece(
    const Board& board, Position from, const Piece& piece) {
    
    switch (piece.type) {
        case PieceType::PAWN:
            return getPawnMoves(board, from, piece.color);
        case PieceType::KNIGHT:
            return getKnightMoves(board, from, piece.color);
        case PieceType::BISHOP:
            return getBishopMoves(board, from, piece.color);
        case PieceType::ROOK:
            return getRookMoves(board, from, piece.color);
        case PieceType::QUEEN:
            return getQueenMoves(board, from, piece.color);
        case PieceType::KING:
            return getKingMoves(board, from, piece.color);
        default:
            return std::vector<Position>();
    }
}

bool MoveGenerator::isKingInCheck(const Board& board, Color color) {
    // Find the king
    Position kingPos;
    bool found = false;
    for (int row = 0; row < 8 && !found; row++) {
        for (int col = 0; col < 8 && !found; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING && piece.color == color) {
                kingPos = Position(row, col);
                found = true;
            }
        }
    }
    
    if (!found) return false;
    
    // Check if any opponent piece can attack the king
    Color opponentColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (!piece.isEmpty() && piece.color == opponentColor) {
                std::vector<Position> moves = getValidMovesForPiece(board, Position(row, col), piece);
                for (const Position& move : moves) {
                    if (move == kingPos) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

bool MoveGenerator::wouldMoveResultInCheck(const Board& board, const Move& move) {
    Board testBoard = board.clone();
    testBoard.applyMove(move);
    return isKingInCheck(testBoard, move.piece.color);
}

std::vector<Move> MoveGenerator::generateMoves(
    const Board& board, Color color, const CastlingRights& castling) {
    
    std::vector<Move> allMoves;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position from(row, col);
            Piece piece = board.getPiece(from);
            
            if (piece.isEmpty() || piece.color != color) continue;
            
            std::vector<Position> targetSquares = getValidMovesForPiece(board, from, piece);
            
            for (const Position& to : targetSquares) {
                Move move(from, to, piece);
                move.captured = board.getPiece(to);
                
                // Check for pawn promotion
                if (piece.type == PieceType::PAWN) {
                    int promotionRow = (color == Color::WHITE) ? 7 : 0;
                    if (to.row == promotionRow) {
                        move.isPromotion = true;
                        move.promotionType = PieceType::QUEEN; // Auto-promote to queen
                    }
                }
                
                // Don't allow moves that would put own king in check
                if (!wouldMoveResultInCheck(board, move)) {
                    allMoves.push_back(move);
                }
            }
            
            // Add castling moves for the king
            if (piece.type == PieceType::KING && !isKingInCheck(board, color)) {
                int homeRow = (color == Color::WHITE) ? 0 : 7;
                
                // King-side castling (King moves 3→1, Rook 0→2)
                if (color == Color::WHITE && castling.whiteKingSide) {
                    if (board.isEmpty(Position(homeRow, 1)) && 
                        board.isEmpty(Position(homeRow, 2))) {
                        // Check if squares king passes through are not under attack
                        Move testMove1(Position(homeRow, 3), Position(homeRow, 2), piece);
                        Move testMove2(Position(homeRow, 3), Position(homeRow, 1), piece);
                        
                        if (!wouldMoveResultInCheck(board, testMove1) && 
                            !wouldMoveResultInCheck(board, testMove2)) {
                            Move castleMove(Position(homeRow, 3), Position(homeRow, 1), piece);
                            castleMove.isCastling = true;
                            allMoves.push_back(castleMove);
                        }
                    }
                } else if (color == Color::BLACK && castling.blackKingSide) {
                    if (board.isEmpty(Position(homeRow, 1)) && 
                        board.isEmpty(Position(homeRow, 2))) {
                        Move testMove1(Position(homeRow, 3), Position(homeRow, 2), piece);
                        Move testMove2(Position(homeRow, 3), Position(homeRow, 1), piece);
                        
                        if (!wouldMoveResultInCheck(board, testMove1) && 
                            !wouldMoveResultInCheck(board, testMove2)) {
                            Move castleMove(Position(homeRow, 3), Position(homeRow, 1), piece);
                            castleMove.isCastling = true;
                            allMoves.push_back(castleMove);
                        }
                    }
                }
                
                // Queen-side castling (King moves 3→5, Rook 7→4)
                if (color == Color::WHITE && castling.whiteQueenSide) {
                    if (board.isEmpty(Position(homeRow, 4)) && 
                        board.isEmpty(Position(homeRow, 5)) && 
                        board.isEmpty(Position(homeRow, 6))) {
                        Move testMove1(Position(homeRow, 3), Position(homeRow, 4), piece);
                        Move testMove2(Position(homeRow, 3), Position(homeRow, 5), piece);
                        
                        if (!wouldMoveResultInCheck(board, testMove1) && 
                            !wouldMoveResultInCheck(board, testMove2)) {
                            Move castleMove(Position(homeRow, 3), Position(homeRow, 5), piece);
                            castleMove.isCastling = true;
                            allMoves.push_back(castleMove);
                        }
                    }
                } else if (color == Color::BLACK && castling.blackQueenSide) {
                    if (board.isEmpty(Position(homeRow, 4)) && 
                        board.isEmpty(Position(homeRow, 5)) && 
                        board.isEmpty(Position(homeRow, 6))) {
                        Move testMove1(Position(homeRow, 3), Position(homeRow, 4), piece);
                        Move testMove2(Position(homeRow, 3), Position(homeRow, 5), piece);
                        
                        if (!wouldMoveResultInCheck(board, testMove1) && 
                            !wouldMoveResultInCheck(board, testMove2)) {
                            Move castleMove(Position(homeRow, 3), Position(homeRow, 5), piece);
                            castleMove.isCastling = true;
                            allMoves.push_back(castleMove);
                        }
                    }
                }
            }
        }
    }
    
    return allMoves;
}

} // namespace Chess
