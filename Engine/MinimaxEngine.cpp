#include "MinimaxEngine.h"
#include <algorithm>

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
MinimaxEngine::MinimaxEngine(int depth) : depth_(depth), maxTime_(0), timeExpired_(false) {}

void MinimaxEngine::setDepth(int depth) {
    depth_ = depth;
}

void MinimaxEngine::setMaxTime(int milliseconds) {
    maxTime_ = milliseconds;
}

bool MinimaxEngine::isTimeExpired() const {
    if (maxTime_ <= 0) return false; // No time limit
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStartTime_
    ).count();
    
    return elapsed >= maxTime_;
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
    
    // Check time limit
    if (isTimeExpired()) {
        timeExpired_ = true;
        return Evaluator::evaluate(board, rootColor_);
    }
    
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
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();
    
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
