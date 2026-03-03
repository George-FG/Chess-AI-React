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

// Check if we're in the opening phase (roughly first 15 moves)
bool Evaluator::isInOpeningPhase(int moveCount) {
    return moveCount < 15;
}

// Evaluate piece development for opening
int Evaluator::evaluatePieceDevelopment(const Board& board, Color color) {
    int score = 0;
    int backRank = (color == Color::WHITE) ? 0 : 7;
    int pawnRank = (color == Color::WHITE) ? 1 : 6;
    
    // Check knights - should move off back rank
    Piece knight1 = board.getPiece(Position(backRank, 1));
    Piece knight2 = board.getPiece(Position(backRank, 6));
    
    if (knight1.type == PieceType::KNIGHT && knight1.color == color) {
        score -= 30; // Penalty for knight still on back rank
    } else {
        score += 25; // Bonus for developing knight
    }
    
    if (knight2.type == PieceType::KNIGHT && knight2.color == color) {
        score -= 30;
    } else {
        score += 25;
    }
    
    // Check bishops - should move off back rank
    Piece bishop1 = board.getPiece(Position(backRank, 2));
    Piece bishop2 = board.getPiece(Position(backRank, 5));
    
    if (bishop1.type == PieceType::BISHOP && bishop1.color == color) {
        score -= 25;
    } else {
        score += 20;
    }
    
    if (bishop2.type == PieceType::BISHOP && bishop2.color == color) {
        score -= 25;
    } else {
        score += 20;
    }
    
    // Penalty for queen moving in opening (develops too early)
    Piece queenHome = board.getPiece(Position(backRank, 3));
    if (queenHome.isEmpty() || queenHome.type != PieceType::QUEEN || queenHome.color != color) {
        // Find queen elsewhere
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                Piece p = board.getPiece(Position(row, col));
                if (p.type == PieceType::QUEEN && p.color == color && row != backRank) {
                    score -= 40; // Penalty for moving queen in opening
                    break;
                }
            }
        }
    }
    
    // Bonus for center pawn development
    Piece centerPawn1 = board.getPiece(Position(pawnRank, 3));
    Piece centerPawn2 = board.getPiece(Position(pawnRank, 4));
    
    if (centerPawn1.isEmpty() || centerPawn1.type != PieceType::PAWN || centerPawn1.color != color) {
        score += 15; // Center pawn moved
    }
    if (centerPawn2.isEmpty() || centerPawn2.type != PieceType::PAWN || centerPawn2.color != color) {
        score += 15; // Center pawn moved
    }
    
    return score;
}

// Evaluator implementation
int Evaluator::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, int moveCount) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    bool isOpening = isInOpeningPhase(moveCount);
    
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
                    // tableRow = how far the pawn has advanced from its home rank perspective (0..7)
                    int tableRow = (aiColor == Color::WHITE) ? (7 - row) : row;
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
                    // tableRow = how far the pawn has advanced from its home rank perspective (0..7)
                    int tableRow = (opponentColor == Color::WHITE) ? (7 - row) : row;
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
    
    // King safety evaluation: strongly favor castling and penalize premature king moves
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING) {
                // Most common representation: row 0 is Black's home rank (8th rank),
                // row 7 is White's home rank (1st rank).
                int backRank = (piece.color == Color::WHITE) ? 7 : 0;
                
                if (piece.color == aiColor) {
                    if (row == backRank) {
                        // King is on back rank
                        if (col == 6 || col == 2) {
                            // King has castled - HUGE bonus
                            myScore += 150;
                        } else if (col == 4) {
                            // King still on starting square - neutral/small penalty for not castling yet
                            myScore -= 10;
                        } else {
                            // King moved but didn't castle - LARGE penalty (unless avoiding check)
                            myScore -= 80;
                        }
                    } else {
                        // King moved off back rank - MASSIVE penalty (very dangerous)
                        myScore -= 100;
                    }
                } else {
                    // Opponent king evaluation (same logic)
                    if (row == backRank) {
                        if (col == 6 || col == 2) {
                            opponentScore += 150;
                        } else if (col == 4) {
                            opponentScore -= 10;
                        } else {
                            opponentScore -= 80;
                        }
                    } else {
                        opponentScore -= 100;
                    }
                }
            }
        }
    }
    
    // Opening phase bonuses
    if (isOpening) {
        int myDevelopment = evaluatePieceDevelopment(board, aiColor);
        int opponentDevelopment = evaluatePieceDevelopment(board, opponentColor);
        
        myScore += myDevelopment;
        opponentScore += opponentDevelopment;
        
        // Extra bonus for castling in opening - CASTLE AS SOON AS POSSIBLE
        Piece myKing = board.getPiece(Position((aiColor == Color::WHITE) ? 0 : 7, 4));
        if (myKing.isEmpty() || myKing.type != PieceType::KING) {
            // King has moved - check if it castled
            int backRank = (aiColor == Color::WHITE) ? 0 : 7;
            Piece kingPos1 = board.getPiece(Position(backRank, 6));
            Piece kingPos2 = board.getPiece(Position(backRank, 2));
            
            if ((kingPos1.type == PieceType::KING && kingPos1.color == aiColor) ||
                (kingPos2.type == PieceType::KING && kingPos2.color == aiColor)) {
                myScore += 200; // MASSIVE bonus for castling in opening
            }
        }
        
        // Same for opponent
        Piece oppKing = board.getPiece(Position((opponentColor == Color::WHITE) ? 0 : 7, 4));
        if (oppKing.isEmpty() || oppKing.type != PieceType::KING) {
            int backRank = (opponentColor == Color::WHITE) ? 0 : 7;
            Piece kingPos1 = board.getPiece(Position(backRank, 6));
            Piece kingPos2 = board.getPiece(Position(backRank, 2));
            
            if ((kingPos1.type == PieceType::KING && kingPos1.color == opponentColor) ||
                (kingPos2.type == PieceType::KING && kingPos2.color == opponentColor)) {
                opponentScore += 200;
            }
        }
    }
    
    // Castling rights evaluation: having the ability to castle is valuable
    // Give bonus for having castling rights AVAILABLE (not yet used)
    if (aiColor == Color::WHITE) {
        // Check if king is still on starting square (hasn't castled or moved)
        Piece kingPiece = board.getPiece(Position(7, 4));
        if (kingPiece.type == PieceType::KING && kingPiece.color == Color::WHITE) {
            // King hasn't moved yet, value having castling rights
            if (castling.whiteKingSide) myScore += 40;  // Can castle kingside
            if (castling.whiteQueenSide) myScore += 40; // Can castle queenside
        }
    } else {
        // Black's turn
        Piece kingPiece = board.getPiece(Position(0, 4));
        if (kingPiece.type == PieceType::KING && kingPiece.color == Color::BLACK) {
            if (castling.blackKingSide) myScore += 40;
            if (castling.blackQueenSide) myScore += 40;
        }
    }
    
    // Same for opponent (we want to preserve our options, they theirs)
    if (opponentColor == Color::WHITE) {
        Piece kingPiece = board.getPiece(Position(7, 4));
        if (kingPiece.type == PieceType::KING && kingPiece.color == Color::WHITE) {
            if (castling.whiteKingSide) opponentScore += 40;
            if (castling.whiteQueenSide) opponentScore += 40;
        }
    } else {
        Piece kingPiece = board.getPiece(Position(0, 4));
        if (kingPiece.type == PieceType::KING && kingPiece.color == Color::BLACK) {
            if (castling.blackKingSide) opponentScore += 40;
            if (castling.blackQueenSide) opponentScore += 40;
        }
    }
    
    return myScore - opponentScore;
}

// MinimaxEngine implementation
MinimaxEngine::MinimaxEngine(int depth) : depth_(depth), maxTime_(0), timeExpired_(false), moveCount_(0) {}

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
    
    // If a rook is captured on its original square, lose that side's castling right
    if (!move.captured.isEmpty() && move.captured.type == PieceType::ROOK) {
        if (move.captured.color == Color::WHITE) {
            if (move.to.row == 7 && move.to.col == 0) newRights.whiteQueenSide = false;
            if (move.to.row == 7 && move.to.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.to.row == 0 && move.to.col == 0) newRights.blackQueenSide = false;
            if (move.to.row == 0 && move.to.col == 7) newRights.blackKingSide = false;
        }
    }
    
    return newRights;
}

int MinimaxEngine::minimax(const Board& board, Color currentColor, int depth,
                          bool maximizing, int alpha, int beta,
                          const CastlingRights& castling, int moveCount) {
    
    // Check time limit
    if (isTimeExpired()) {
        timeExpired_ = true;
        return Evaluator::evaluate(board, rootColor_, castling, moveCount);
    }
    
    if (depth == 0) {
        return Evaluator::evaluate(board, rootColor_, castling, moveCount);
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
    
    // Move ordering: prioritize castling, then captures, penalize king and rook moves
    std::sort(moves.begin(), moves.end(), [&castling, currentColor](const Move& a, const Move& b) {
        int aScore = 0;
        int bScore = 0;

        // Prefer castling VERY strongly (highest priority)
        if (a.isCastling) aScore += 500;
        if (b.isCastling) bScore += 500;

        // HEAVILY discourage rook moves from corner when we can still castle on that side
        if (a.piece.type == PieceType::ROOK && !a.isCastling) {
            if (currentColor == Color::WHITE) {
                if (a.from.row == 7) {
                    if (a.from.col == 7 && castling.whiteKingSide) aScore -= 200;  // h1 rook
                    if (a.from.col == 0 && castling.whiteQueenSide) aScore -= 200; // a1 rook
                }
            } else {
                if (a.from.row == 0) {
                    if (a.from.col == 7 && castling.blackKingSide) aScore -= 200;  // h8 rook
                    if (a.from.col == 0 && castling.blackQueenSide) aScore -= 200; // a8 rook
                }
            }
        }
        if (b.piece.type == PieceType::ROOK && !b.isCastling) {
            if (currentColor == Color::WHITE) {
                if (b.from.row == 7) {
                    if (b.from.col == 7 && castling.whiteKingSide) bScore -= 200;
                    if (b.from.col == 0 && castling.whiteQueenSide) bScore -= 200;
                }
            } else {
                if (b.from.row == 0) {
                    if (b.from.col == 7 && castling.blackKingSide) bScore -= 200;
                    if (b.from.col == 0 && castling.blackQueenSide) bScore -= 200;
                }
            }
        }

        // Discourage early king moves that aren't castling
        if (a.piece.type == PieceType::KING && !a.isCastling) aScore -= 120;
        if (b.piece.type == PieceType::KING && !b.isCastling) bScore -= 120;

        // Captures next
        if (!a.captured.isEmpty()) aScore += 20;
        if (!b.captured.isEmpty()) bScore += 20;

        return aScore > bScore;
    });
    
    int bestScore = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = minimax(newBoard, nextColor, depth - 1, !maximizing, alpha, beta, newCastling, moveCount + 1);
        
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

std::string MinimaxEngine::getPositionHash(const Board& board) {
    // Simple position hash: concatenate piece positions
    std::string hash = "";
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPiece(Position(row, col));
            if (!p.isEmpty()) {
                hash += std::to_string(row) + std::to_string(col);
                hash += std::to_string(static_cast<int>(p.type));
                hash += std::to_string(static_cast<int>(p.color));
            }
        }
    }
    return hash;
}

Move MinimaxEngine::findBestMove(const Board& board, Color color, const CastlingRights& castling,
                                 const std::vector<std::string>& positionHistory) {
    rootColor_ = color;
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();
    
    // Estimate move count from position history
    moveCount_ = positionHistory.size();
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    
    if (moves.empty()) {
        return Move(); // No valid moves
    }
    
    // Move ordering at root: in opening, HEAVILY prioritize castling and development
    bool isOpening = Evaluator::isInOpeningPhase(moveCount_);
    
    std::sort(moves.begin(), moves.end(), [&castling, color, isOpening](const Move& a, const Move& b) {
        int aScore = 0;
        int bScore = 0;
        
        // In opening: MASSIVE priority for castling
        if (a.isCastling) aScore += (isOpening ? 5000 : 1000);
        if (b.isCastling) bScore += (isOpening ? 5000 : 1000);
        
        // In opening: Bonus for developing knights and bishops
        if (isOpening) {
            int aFromRank = (color == Color::WHITE) ? 0 : 7;
            int bFromRank = (color == Color::WHITE) ? 0 : 7;
            
            // Bonus for moving knights off back rank
            if (a.piece.type == PieceType::KNIGHT && a.from.row == aFromRank) {
                aScore += 300;
            }
            if (b.piece.type == PieceType::KNIGHT && b.from.row == bFromRank) {
                bScore += 300;
            }
            
            // Bonus for moving bishops off back rank
            if (a.piece.type == PieceType::BISHOP && a.from.row == aFromRank) {
                aScore += 250;
            }
            if (b.piece.type == PieceType::BISHOP && b.from.row == bFromRank) {
                bScore += 250;
            }
            
            // Penalty for queen moves in opening
            if (a.piece.type == PieceType::QUEEN) aScore -= 400;
            if (b.piece.type == PieceType::QUEEN) bScore -= 400;
            
            // Bonus for center pawn moves (e4, d4, e5, d5)
            if (a.piece.type == PieceType::PAWN && (a.to.col == 3 || a.to.col == 4)) {
                aScore += 200;
            }
            if (b.piece.type == PieceType::PAWN && (b.to.col == 3 || b.to.col == 4)) {
                bScore += 200;
            }
        }
        
        // HEAVILY penalize rook moves from corner when we can still castle
        if (a.piece.type == PieceType::ROOK) {
            if (color == Color::WHITE && a.from.row == 7) {
                if (a.from.col == 7 && castling.whiteKingSide) aScore -= 300;
                if (a.from.col == 0 && castling.whiteQueenSide) aScore -= 300;
            } else if (color == Color::BLACK && a.from.row == 0) {
                if (a.from.col == 7 && castling.blackKingSide) aScore -= 300;
                if (a.from.col == 0 && castling.blackQueenSide) aScore -= 300;
            }
        }
        if (b.piece.type == PieceType::ROOK) {
            if (color == Color::WHITE && b.from.row == 7) {
                if (b.from.col == 7 && castling.whiteKingSide) bScore -= 300;
                if (b.from.col == 0 && castling.whiteQueenSide) bScore -= 300;
            } else if (color == Color::BLACK && b.from.row == 0) {
                if (b.from.col == 7 && castling.blackKingSide) bScore -= 300;
                if (b.from.col == 0 && castling.blackQueenSide) bScore -= 300;
            }
        }
        
        // Penalize non-castling king moves (search these last)
        if (a.piece.type == PieceType::KING && !a.isCastling) aScore -= 50;
        if (b.piece.type == PieceType::KING && !b.isCastling) bScore -= 50;
        
        // Then captures
        if (!a.captured.isEmpty()) aScore += 10;
        if (!b.captured.isEmpty()) bScore += 10;
        
        return aScore > bScore;
    });
    
    // Iterative deepening: start at depth 1, increase until time runs out or max depth reached
    Move bestMove = moves[0];
    int maxDepth = depth_;
    
    for (int currentDepth = 1; currentDepth <= maxDepth; currentDepth++) {
        if (isTimeExpired()) {
            // Time expired, return best move found so far
            break;
        }
        
        int bestScore = std::numeric_limits<int>::min();
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();
        Move depthBestMove = moves[0];
        
        bool searchCompleted = true;
        
        for (const Move& move : moves) {
            if (isTimeExpired()) {
                // Time expired during this depth, keep previous depth's best move
                searchCompleted = false;
                break;
            }
            
            Board newBoard = board.clone();
            newBoard.applyMove(move);
            
            CastlingRights newCastling = updateCastlingRights(castling, move, board);
            Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            
            int score = minimax(newBoard, nextColor, currentDepth - 1, false, alpha, beta, newCastling, moveCount_);
            
            // Check for position repetition - avoid draws when not losing
            if (!positionHistory.empty()) {
                std::string posHash = getPositionHash(newBoard);
                int repetitionCount = 0;
                for (const auto& oldPos : positionHistory) {
                    if (oldPos == posHash) repetitionCount++;
                }
                
                // If position repeats and we're not losing, heavily penalize
                if (repetitionCount > 0 && score >= -100) {
                    score -= 300; // Strong penalty for repetition in equal/winning positions
                }
            }
            
            if (score > bestScore) {
                bestScore = score;
                depthBestMove = move;
            }
            
            alpha = std::max(alpha, score);
        }
        
        // Only update best move if we completed this depth's search
        if (searchCompleted) {
            bestMove = depthBestMove;
            
            // Re-order moves based on scores for next iteration (move ordering optimization)
            // This helps alpha-beta pruning in deeper searches
        }
    }
    
    return bestMove;
}

} // namespace Chess
