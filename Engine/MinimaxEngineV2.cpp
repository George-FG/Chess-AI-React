#include "MinimaxEngineV2.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <sstream>

namespace Chess {

// ============================
// EVALUATOR V2
// ============================

const int EvaluatorV2::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    20000,// KING
    0     // NONE
};

// Improved material evaluation with castling bonus, development, and mobility
int EvaluatorV2::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, 
                          bool whiteHasCastled, bool blackHasCastled, int moveCount) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int score = 0;
    
    // Material evaluation (MOST important - heavily weighted)
    int aiMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, oppColor);
    int materialDiff = aiMaterial - oppMaterial;
    
    // Base material score
    score = materialDiff;
    
    // After opening, heavily penalize being down material
    // This ensures the engine won't sacrifice pieces for position
    if (moveCount > 2) {
        // Add extra weight to material deficit
        if (materialDiff < -50) { // Down more than half a pawn
            score += materialDiff * 2; // Triple the penalty (original + 2x more)
        }
    }
    
    // Piece development bonus/penalty (critical in early game)
    if (moveCount < 18) {
        int aiDevelopment = evaluatePieceDevelopment(board, aiColor, moveCount);
        int oppDevelopment = evaluatePieceDevelopment(board, oppColor, moveCount);
        // Apply full development score - penalties are important
        score += aiDevelopment - oppDevelopment;
    }
    
    // Mobility bonus (piece activity) - only if not losing material
    if (materialDiff >= -50) { // Only consider mobility if not significantly down material
        int aiMobility = evaluateMobility(board, aiColor, castling);
        int oppMobility = evaluateMobility(board, oppColor, castling);
        score += (aiMobility - oppMobility) / 15; // Further scaled down
    }
    
    // Castling bonus - encourage castling for king safety (only if not down material)
    if (materialDiff >= -100) {
        const int CASTLING_BONUS = 25;
        
        if (aiColor == Color::WHITE) {
            if (whiteHasCastled) {
                score += CASTLING_BONUS;
            }
            if (blackHasCastled) {
                score -= CASTLING_BONUS;
            }
        } else {
            if (blackHasCastled) {
                score += CASTLING_BONUS;
            }
            if (whiteHasCastled) {
                score -= CASTLING_BONUS;
            }
        }
        
        // Bonus for keeping castling rights (if not yet castled)
        const int CASTLING_RIGHTS_BONUS = 8;
        
        if (aiColor == Color::WHITE && !whiteHasCastled && moveCount < 15) {
            if (castling.whiteKingSide || castling.whiteQueenSide) {
                score += CASTLING_RIGHTS_BONUS;
            }
        } else if (aiColor == Color::BLACK && !blackHasCastled && moveCount < 15) {
            if (castling.blackKingSide || castling.blackQueenSide) {
                score += CASTLING_RIGHTS_BONUS;
            }
        }
        
        if (oppColor == Color::WHITE && !whiteHasCastled && moveCount < 15) {
            if (castling.whiteKingSide || castling.whiteQueenSide) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        } else if (oppColor == Color::BLACK && !blackHasCastled && moveCount < 15) {
            if (castling.blackKingSide || castling.blackQueenSide) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        }
    }
    
    // Positional bonus: center control (scaled down, only if not behind in material)
    if (materialDiff >= -50) {
        const int CENTER_BONUS[8][8] = {
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 5, 5, 5, 5, 0, 0},
            {0, 0, 5, 8, 8, 5, 0, 0},
            {0, 0, 5, 8, 8, 5, 0, 0},
            {0, 0, 5, 5, 5, 5, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0}
        };
        
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece p = board.getPiece(Position(r, c));
                if (p.isEmpty()) continue;
                
                int bonus = 0;
                
                // Center control, but not for kings in opening/middlegame
                if (p.type != PieceType::KING || moveCount > 30) {
                    bonus += CENTER_BONUS[r][c];
                }
                
                if (p.color == aiColor) {
                    score += bonus;
                } else {
                    score -= bonus;
                }
            }
        }
    }
    
    return score;
}

int EvaluatorV2::countMaterial(const Board& board, Color color) {
    int material = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (!p.isEmpty() && p.color == color) {
                int typeIdx = static_cast<int>(p.type);
                // Bounds check to prevent memory access errors
                if (typeIdx >= 0 && typeIdx < 7) {
                    material += PIECE_VALUES[typeIdx];
                }
            }
        }
    }
    
    return material;
}

// Check if a piece is on its starting square
bool EvaluatorV2::isPieceOnStartingSquare(PieceType type, Color color, Position pos) {
    if (color == Color::WHITE) {
        switch (type) {
            case PieceType::KNIGHT:
                return pos.row == 0 && (pos.col == 1 || pos.col == 6);
            case PieceType::BISHOP:
                return pos.row == 0 && (pos.col == 2 || pos.col == 5);
            case PieceType::ROOK:
                return pos.row == 0 && (pos.col == 0 || pos.col == 7);
            case PieceType::QUEEN:
                return pos.row == 0 && pos.col == 3;
            default:
                return false;
        }
    } else {
        switch (type) {
            case PieceType::KNIGHT:
                return pos.row == 7 && (pos.col == 1 || pos.col == 6);
            case PieceType::BISHOP:
                return pos.row == 7 && (pos.col == 2 || pos.col == 5);
            case PieceType::ROOK:
                return pos.row == 7 && (pos.col == 0 || pos.col == 7);
            case PieceType::QUEEN:
                return pos.row == 7 && pos.col == 3;
            default:
                return false;
        }
    }
}

// Count undeveloped pieces (knights, bishops still on starting squares)
int EvaluatorV2::countUndevelopedPieces(const Board& board, Color color) {
    int undeveloped = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty() || p.color != color) continue;
            
            Position pos(r, c);
            
            // Count knights and bishops on starting squares
            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (isPieceOnStartingSquare(p.type, color, pos)) {
                    undeveloped++;
                }
            }
        }
    }
    
    return undeveloped;
}

// Check if rooks have moved from starting position
bool EvaluatorV2::isRookDeveloped(const Board& board, Color color) {
    int startRow = (color == Color::WHITE) ? 0 : 7;
    
    // Check if either rook is off its starting square
    Piece leftRook = board.getPiece(Position(startRow, 0));
    Piece rightRook = board.getPiece(Position(startRow, 7));
    
    bool leftRookMoved = leftRook.isEmpty() || leftRook.type != PieceType::ROOK || leftRook.color != color;
    bool rightRookMoved = rightRook.isEmpty() || rightRook.type != PieceType::ROOK || rightRook.color != color;
    
    return leftRookMoved || rightRookMoved;
}

// Evaluate piece development - reward getting pieces off back rank
int EvaluatorV2::evaluatePieceDevelopment(const Board& board, Color color, int moveCount) {
    int development = 0;
    int undevelopedCount = countUndevelopedPieces(board, color);
    bool hasUndevelopedMinors = undevelopedCount > 0;
    bool rooksDeveloped = isRookDeveloped(board, color);
    
    // STRONG penalty for undeveloped pieces as game progresses
    if (moveCount > 5) {
        development -= undevelopedCount * 30; // Heavy penalty per undeveloped piece
    }
    
    // PENALTY for moving rooks before completing minor piece development
    if (hasUndevelopedMinors && rooksDeveloped && moveCount < 15) {
        development -= 40; // Heavy penalty for premature rook development
    }
    
    // Early game development bonuses
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty() || p.color != color) continue;
            
            Position pos(r, c);
            
            // Reward developing knights and bishops
            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development += 25;
                } else if (moveCount > 8) {
                    // Extra penalty for still being on starting square after move 8
                    development -= 20;
                }
            }
            
            // Encourage central pawn advancement in opening
            if (p.type == PieceType::PAWN && moveCount < 10) {
                if (color == Color::WHITE) {
                    if ((c == 3 || c == 4) && r >= 3) {
                        development += 15;
                    }
                } else {
                    if ((c == 3 || c == 4) && r <= 4) {
                        development += 15;
                    }
                }
            }
            
            // STRONG penalty for early queen development (before move 10)
            if (p.type == PieceType::QUEEN && moveCount < 10) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 25;
                }
            }
            
            // PENALTY for moving rooks too early (before move 10 and minors not developed)
            if (p.type == PieceType::ROOK && moveCount < 10 && hasUndevelopedMinors) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 35; // Strong discouragement
                }
            }
        }
    }
    
    return development;
}

// Evaluate piece mobility - count number of legal moves
int EvaluatorV2::evaluateMobility(const Board& board, Color color, const CastlingRights& castling) {
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    return moves.size();
}

// ============================
// MINIMAX ENGINE V2
// ============================

MinimaxEngineV2::MinimaxEngineV2(int depth) 
    : depth_(depth), maxTime_(0), timeExpired_(false), moveCount_(0),
      whiteHasCastled_(false), blackHasCastled_(false) {}

void MinimaxEngineV2::setDepth(int depth) {
    depth_ = depth;
}

void MinimaxEngineV2::setMaxTime(int milliseconds) {
    maxTime_ = milliseconds;
}

bool MinimaxEngineV2::isTimeExpired() const {
    if (maxTime_ == 0) return false;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime_);
    return duration.count() >= maxTime_;
}

std::string MinimaxEngineV2::getPositionHash(const Board& board) {
    std::ostringstream oss;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) {
                oss << ".";
            } else {
                char piece_char = '.';
                switch (p.type) {
                    case PieceType::PAWN: piece_char = 'p'; break;
                    case PieceType::KNIGHT: piece_char = 'n'; break;
                    case PieceType::BISHOP: piece_char = 'b'; break;
                    case PieceType::ROOK: piece_char = 'r'; break;
                    case PieceType::QUEEN: piece_char = 'q'; break;
                    case PieceType::KING: piece_char = 'k'; break;
                    default: piece_char = '.'; break;
                }
                if (p.color == Color::WHITE) {
                    piece_char = toupper(piece_char);
                }
                oss << piece_char;
            }
        }
    }
    
    return oss.str();
}

CastlingRights MinimaxEngineV2::updateCastlingRights(const CastlingRights& rights, const Move& move, const Board& board) {
    CastlingRights newRights = rights;
    
    // If king moves, lose both castling rights
    if (move.piece.type == PieceType::KING) {
        if (move.piece.color == Color::WHITE) {
            newRights.whiteKingSide = false;
            newRights.whiteQueenSide = false;
        } else {
            newRights.blackKingSide = false;
            newRights.blackQueenSide = false;
        }
    }
    
    // If rook moves from starting position, lose that side's castling
    if (move.piece.type == PieceType::ROOK) {
        if (move.piece.color == Color::WHITE) {
            if (move.from.row == 0 && move.from.col == 0) newRights.whiteQueenSide = false;
            if (move.from.row == 0 && move.from.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.from.row == 7 && move.from.col == 0) newRights.blackQueenSide = false;
            if (move.from.row == 7 && move.from.col == 7) newRights.blackKingSide = false;
        }
    }
    
    // If rook is captured, lose that castling right
    if (!move.captured.isEmpty() && move.captured.type == PieceType::ROOK) {
        if (move.captured.color == Color::WHITE) {
            if (move.to.row == 0 && move.to.col == 0) newRights.whiteQueenSide = false;
            if (move.to.row == 0 && move.to.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.to.row == 7 && move.to.col == 0) newRights.blackQueenSide = false;
            if (move.to.row == 7 && move.to.col == 7) newRights.blackKingSide = false;
        }
    }
    
    return newRights;
}

// Order moves to improve alpha-beta pruning efficiency
// Use MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
void MinimaxEngineV2::orderMoves(std::vector<Move>& moves, const Board& board) {
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        // Prioritize captures
        bool aIsCapture = !a.captured.isEmpty();
        bool bIsCapture = !b.captured.isEmpty();
        
        if (aIsCapture && !bIsCapture) return true;
        if (!aIsCapture && bIsCapture) return false;
        
        // If both are captures, use MVV-LVA heuristic
        // (Most Valuable Victim - Least Valuable Attacker)
        if (aIsCapture && bIsCapture) {
            // Safely get piece type indices with bounds checking
            int aCapturedIdx = static_cast<int>(a.captured.type);
            int bCapturedIdx = static_cast<int>(b.captured.type);
            int aAttackerIdx = static_cast<int>(a.piece.type);
            int bAttackerIdx = static_cast<int>(b.piece.type);
            
            // Ensure indices are within valid range [0-6]
            if (aCapturedIdx < 0 || aCapturedIdx >= 7) aCapturedIdx = 6; // Default to NONE
            if (bCapturedIdx < 0 || bCapturedIdx >= 7) bCapturedIdx = 6;
            if (aAttackerIdx < 0 || aAttackerIdx >= 7) aAttackerIdx = 6;
            if (bAttackerIdx < 0 || bAttackerIdx >= 7) bAttackerIdx = 6;
            
            // Get piece values
            int aVictimValue = EvaluatorV2::PIECE_VALUES[aCapturedIdx];
            int bVictimValue = EvaluatorV2::PIECE_VALUES[bCapturedIdx];
            int aAttackerValue = EvaluatorV2::PIECE_VALUES[aAttackerIdx];
            int bAttackerValue = EvaluatorV2::PIECE_VALUES[bAttackerIdx];
            
            // Prefer capturing high-value pieces with low-value pieces
            // MVV-LVA score = VictimValue * 100 - AttackerValue
            int aScore = aVictimValue * 100 - aAttackerValue;
            int bScore = bVictimValue * 100 - bAttackerValue;
            
            return aScore > bScore;
        }
        
        // Prioritize promotions
        if (a.isPromotion && !b.isPromotion) return true;
        if (!a.isPromotion && b.isPromotion) return false;
        
        // Prioritize castling in opening
        if (a.isCastling && !b.isCastling) return true;
        if (!a.isCastling && b.isCastling) return false;
        
        return false;
    });
}

int MinimaxEngineV2::minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                              int alpha, int beta, const CastlingRights& castling, int moveCount) {
    // Check time limit
    if (isTimeExpired()) {
        timeExpired_ = true;
        return 0;
    }
    
    // Base case: reached depth limit
    if (depth == 0) {
        return EvaluatorV2::evaluate(board, rootColor_, castling, whiteHasCastled_, blackHasCastled_, moveCount);
    }
    
    // Generate all legal moves
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    // If no legal moves, it's checkmate or stalemate
    if (moves.empty()) {
        // Check if in check
        bool inCheck = MoveGenerator::isKingInCheck(board, currentColor);
        
        if (inCheck) {
            // Checkmate - very bad if we're being checkmated, very good if opponent is
            return maximizing ? -100000 : 100000;
        } else {
            // Stalemate - neutral
            return 0;
        }
    }
    
    // Order moves for better alpha-beta pruning
    orderMoves(moves, board);
    
    // Alpha-beta pruning minimax
    if (maximizing) {
        int maxEval = std::numeric_limits<int>::min();
        
        for (const Move& move : moves) {
            Board newBoard = board.clone();
            newBoard.applyMove(move);
            
            CastlingRights newCastling = updateCastlingRights(castling, move, board);
            
            // Track if castling occurred
            bool newWhiteCastled = whiteHasCastled_;
            bool newBlackCastled = blackHasCastled_;
            if (move.isCastling) {
                if (currentColor == Color::WHITE) {
                    newWhiteCastled = true;
                } else {
                    newBlackCastled = true;
                }
            }
            
            // Temporarily update castling status for evaluation
            bool oldWhiteCastled = whiteHasCastled_;
            bool oldBlackCastled = blackHasCastled_;
            whiteHasCastled_ = newWhiteCastled;
            blackHasCastled_ = newBlackCastled;
            
            Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
            int eval = minimax(newBoard, nextColor, depth - 1, false, alpha, beta, newCastling, moveCount + 1);
            
            // Restore castling status
            whiteHasCastled_ = oldWhiteCastled;
            blackHasCastled_ = oldBlackCastled;
            
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            
            // Beta cutoff
            if (beta <= alpha) {
                break;
            }
            
            if (timeExpired_) break;
        }
        
        return maxEval;
    } else {
        int minEval = std::numeric_limits<int>::max();
        
        for (const Move& move : moves) {
            Board newBoard = board.clone();
            newBoard.applyMove(move);
            
            CastlingRights newCastling = updateCastlingRights(castling, move, board);
            
            // Track if castling occurred
            bool newWhiteCastled = whiteHasCastled_;
            bool newBlackCastled = blackHasCastled_;
            if (move.isCastling) {
                if (currentColor == Color::WHITE) {
                    newWhiteCastled = true;
                } else {
                    newBlackCastled = true;
                }
            }
            
            // Temporarily update castling status for evaluation
            bool oldWhiteCastled = whiteHasCastled_;
            bool oldBlackCastled = blackHasCastled_;
            whiteHasCastled_ = newWhiteCastled;
            blackHasCastled_ = newBlackCastled;
            
            Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
            int eval = minimax(newBoard, nextColor, depth - 1, true, alpha, beta, newCastling, moveCount + 1);
            
            // Restore castling status
            whiteHasCastled_ = oldWhiteCastled;
            blackHasCastled_ = oldBlackCastled;
            
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            
            // Alpha cutoff
            if (beta <= alpha) {
                break;
            }
            
            if (timeExpired_) break;
        }
        
        return minEval;
    }
}

Move MinimaxEngineV2::findBestMove(const Board& board, Color color, const CastlingRights& castling,
                                    const std::vector<std::string>& positionHistory,
                                    bool whiteHasCastled, bool blackHasCastled) {
    rootColor_ = color;
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();
    moveCount_ = 0;
    whiteHasCastled_ = whiteHasCastled;
    blackHasCastled_ = blackHasCastled;
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    
    if (moves.empty()) {
        return Move(); // No legal moves
    }
    
    // Order moves at root for better search
    orderMoves(moves, board);
    
    Move bestMove = moves[0];
    int bestScore = std::numeric_limits<int>::min();
    
    // Iterative deepening: search depth 1, 2, 3, ... up to depth_
    // This ensures we always have a move even if time expires
    for (int currentDepth = 1; currentDepth <= depth_; currentDepth++) {
        if (isTimeExpired()) break;
        
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();
        Move depthBestMove = bestMove; // Keep track of best at this depth
        int depthBestScore = std::numeric_limits<int>::min();
        
        for (const Move& move : moves) {
            if (isTimeExpired()) break;
            
            Board newBoard = board.clone();
            newBoard.applyMove(move);
            
            // Check for threefold repetition
            std::string posHash = getPositionHash(newBoard);
            int repetitions = 0;
            for (const std::string& oldPos : positionHistory) {
                if (oldPos == posHash) {
                    repetitions++;
                }
            }
            
            // Avoid repetition if not losing badly
            if (repetitions >= 2) {
                continue;
            }
            
            CastlingRights newCastling = updateCastlingRights(castling, move, board);
            
            // Track if castling occurred
            bool newWhiteCastled = whiteHasCastled_;
            bool newBlackCastled = blackHasCastled_;
            if (move.isCastling) {
                if (color == Color::WHITE) {
                    newWhiteCastled = true;
                } else {
                    newBlackCastled = true;
                }
            }
            
            // Temporarily update castling status for evaluation
            bool oldWhiteCastled = whiteHasCastled_;
            bool oldBlackCastled = blackHasCastled_;
            whiteHasCastled_ = newWhiteCastled;
            blackHasCastled_ = newBlackCastled;
            
            Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            int score = minimax(newBoard, nextColor, currentDepth - 1, false, alpha, beta, newCastling, 1);
            
            // Restore castling status
            whiteHasCastled_ = oldWhiteCastled;
            blackHasCastled_ = oldBlackCastled;
            
            if (score > depthBestScore) {
                depthBestScore = score;
                depthBestMove = move;
            }
            
            alpha = std::max(alpha, score);
        }
        
        // Update best move only if we completed this depth
        if (!isTimeExpired()) {
            bestMove = depthBestMove;
            bestScore = depthBestScore;
        }
        
        // Early exit if mate found
        if (bestScore > 450000) break;
    }
    
    return bestMove;
}

} // namespace Chess
