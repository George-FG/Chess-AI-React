#include "Engine.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace Chess {

// Piece values for evaluation
const int Evaluator::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    0,    // KING
    0     // NONE
};

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
    
    // Set up other pieces
    PieceType backRow[8] = {
        PieceType::ROOK, PieceType::KNIGHT, PieceType::BISHOP, PieceType::QUEEN,
        PieceType::KING, PieceType::BISHOP, PieceType::KNIGHT, PieceType::ROOK
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
        if (move.to.col > move.from.col) { // King side
            Piece rook = getPiece(Position(move.from.row, 7));
            setPiece(Position(move.from.row, 7), Piece());
            setPiece(Position(move.from.row, 5), rook);
        } else { // Queen side
            Piece rook = getPiece(Position(move.from.row, 0));
            setPiece(Position(move.from.row, 0), Piece());
            setPiece(Position(move.from.row, 3), rook);
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
        }
    }
    
    return allMoves;
}

// Evaluator implementation
int Evaluator::evaluate(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            
            if (piece.isEmpty()) continue;
            
            int value = PIECE_VALUES[static_cast<int>(piece.type)];
            
            if (piece.color == aiColor) {
                myScore += value;
                
                // Positional bonuses for pawns
                if (piece.type == PieceType::PAWN) {
                    int tableRow = (aiColor == Color::WHITE) ? row : (7 - row);
                    bool isCenterPawn = (col == 3 || col == 4);
                    
                    if (isCenterPawn) {
                        if (tableRow == 2) myScore += 2;
                        else if (tableRow == 3) myScore += 5;
                        else if (tableRow == 4) myScore += 8;
                    }
                    
                    if (tableRow >= 5) {
                        myScore += tableRow * 3;
                    }
                }
            } else {
                opponentScore += value;
                
                // Positional bonuses for opponent pawns
                if (piece.type == PieceType::PAWN) {
                    int tableRow = (opponentColor == Color::WHITE) ? row : (7 - row);
                    bool isCenterPawn = (col == 3 || col == 4);
                    
                    if (isCenterPawn) {
                        if (tableRow == 2) opponentScore += 2;
                        else if (tableRow == 3) opponentScore += 5;
                        else if (tableRow == 4) opponentScore += 8;
                    }
                    
                    if (tableRow >= 5) {
                        opponentScore += tableRow * 3;
                    }
                }
            }
        }
    }
    
    return myScore - opponentScore;
}

// MinimaxEngine implementation
MinimaxEngine::MinimaxEngine(int depth) : depth_(depth) {}

void MinimaxEngine::setDepth(int depth) {
    depth_ = depth;
}

CastlingRights MinimaxEngine::updateCastlingRights(
    const CastlingRights& rights, const Move& move, const Board& board) {
    
    CastlingRights newRights = rights;
    
    // If king moves, lose all castling rights for that color
    if (move.piece.type == PieceType::KING) {
        if (move.piece.color == Color::WHITE) {
            newRights.whiteKingSide = false;
            newRights.whiteQueenSide = false;
        } else {
            newRights.blackKingSide = false;
            newRights.blackQueenSide = false;
        }
    }
    
    // If rook moves, lose castling right for that side
    if (move.piece.type == PieceType::ROOK) {
        if (move.piece.color == Color::WHITE) {
            if (move.from.col == 0) newRights.whiteQueenSide = false;
            if (move.from.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.from.col == 0) newRights.blackQueenSide = false;
            if (move.from.col == 7) newRights.blackKingSide = false;
        }
    }
    
    return newRights;
}

int MinimaxEngine::minimax(const Board& board, Color currentColor, int depth,
                          bool maximizing, int alpha, int beta,
                          const CastlingRights& castling) {
    
    if (depth == 0) {
        return Evaluator::evaluate(board, rootColor_);
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    if (moves.empty()) {
        // Terminal position: checkmate or stalemate
        bool inCheck = MoveGenerator::isKingInCheck(board, currentColor);
        
        if (inCheck) {
            // Checkmate - prefer quicker checkmates
            int mateScore = maximizing ? -100000 : 100000;
            return mateScore + (maximizing ? depth : -depth);
        } else {
            // Stalemate
            return 0;
        }
    }
    
    // Move ordering: prioritize captures
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        int aScore = a.captured.isEmpty() ? 0 : 10;
        int bScore = b.captured.isEmpty() ? 0 : 10;
        return bScore < aScore;
    });
    
    int bestScore = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = minimax(newBoard, nextColor, depth - 1, !maximizing, alpha, beta, newCastling);
        
        if (maximizing) {
            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, score);
        } else {
            bestScore = std::min(bestScore, score);
            beta = std::min(beta, score);
        }
        
        // Alpha-beta pruning
        if (beta <= alpha) {
            break;
        }
    }
    
    return bestScore;
}

Move MinimaxEngine::findBestMove(const Board& board, Color color, const CastlingRights& castling) {
    rootColor_ = color;
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    
    if (moves.empty()) {
        return Move(); // No valid moves
    }
    
    Move bestMove = moves[0];
    int bestScore = std::numeric_limits<int>::min();
    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();
    
    // Move ordering: prioritize captures
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        int aScore = a.captured.isEmpty() ? 0 : 10;
        int bScore = b.captured.isEmpty() ? 0 : 10;
        return bScore < aScore;
    });
    
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = minimax(newBoard, nextColor, depth_ - 1, false, alpha, beta, newCastling);
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        
        alpha = std::max(alpha, score);
    }
    
    return bestMove;
}

} // namespace Chess
