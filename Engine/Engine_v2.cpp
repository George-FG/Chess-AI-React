#include "Engine_v2.h"
#include <algorithm>
#include <cmath>

namespace Chess {

// Piece values
const int EvaluatorV2::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    0,    // KING
    0     // NONE
};

// Piece-square tables (from white's perspective)
const int EvaluatorV2::PAWN_TABLE[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

const int EvaluatorV2::KNIGHT_TABLE[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int EvaluatorV2::BISHOP_TABLE[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

const int EvaluatorV2::ROOK_TABLE[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {0,  0,  0,  5,  5,  0,  0,  0}
};

const int EvaluatorV2::QUEEN_TABLE[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-5,  0,  5,  5,  5,  5,  0, -5},
    {0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

const int EvaluatorV2::KING_MIDGAME_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    {20, 20,  0,  0,  0,  0, 20, 20},
    {20, 30, 10,  0,  0, 10, 30, 20}
};

const int EvaluatorV2::KING_ENDGAME_TABLE[8][8] = {
    {-50,-40,-30,-20,-20,-30,-40,-50},
    {-30,-20,-10,  0,  0,-10,-20,-30},
    {-30,-10, 20, 30, 30, 20,-10,-30},
    {-30,-10, 30, 40, 40, 30,-10,-30},
    {-30,-10, 30, 40, 40, 30,-10,-30},
    {-30,-10, 20, 30, 30, 20,-10,-30},
    {-30,-30,  0,  0,  0,  0,-30,-30},
    {-50,-30,-30,-30,-30,-30,-30,-50}
};

// Phase detection based on material
GamePhase EvaluatorV2::detectPhase(const Board& board) {
    int totalMaterial = getMaterialCount(board);
    int pieceCount = getPieceCount(board);
    
    // Opening: more than 6200 material (most pieces on board)
    if (totalMaterial > 6200 && pieceCount > 28) {
        return GamePhase::OPENING;
    }
    // Endgame: less than 2600 material (few pieces remain)
    else if (totalMaterial < 2600 || pieceCount < 12) {
        return GamePhase::ENDGAME;
    }
    // Midgame: everything in between
    else {
        return GamePhase::MIDGAME;
    }
}

int EvaluatorV2::getMaterialCount(const Board& board) {
    int total = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (!piece.isEmpty() && piece.type != PieceType::KING) {
                total += PIECE_VALUES[static_cast<int>(piece.type)];
            }
        }
    }
    return total;
}

int EvaluatorV2::getPieceCount(const Board& board) {
    int count = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (!board.getPiece(Position(row, col)).isEmpty()) {
                count++;
            }
        }
    }
    return count;
}

// Main evaluation function
int EvaluatorV2::evaluate(const Board& board, Color aiColor, GamePhase phase) {
    switch (phase) {
        case GamePhase::OPENING:
            return evaluateOpening(board, aiColor);
        case GamePhase::MIDGAME:
            return evaluateMidgame(board, aiColor);
        case GamePhase::ENDGAME:
            return evaluateEndgame(board, aiColor);
        default:
            return 0;
    }
}

// OPENING evaluation: Focus on development and positioning
int EvaluatorV2::evaluateOpening(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    // Material count
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty()) continue;
            
            int value = PIECE_VALUES[static_cast<int>(piece.type)];
            
            // Piece-square table bonus
            int psqtBonus = 0;
            int tableRow = (piece.color == Color::WHITE) ? row : (7 - row);
            
            switch (piece.type) {
                case PieceType::PAWN:
                    psqtBonus = PAWN_TABLE[tableRow][col];
                    break;
                case PieceType::KNIGHT:
                    psqtBonus = KNIGHT_TABLE[tableRow][col];
                    break;
                case PieceType::BISHOP:
                    psqtBonus = BISHOP_TABLE[tableRow][col];
                    break;
                case PieceType::ROOK:
                    psqtBonus = ROOK_TABLE[tableRow][col];
                    break;
                case PieceType::QUEEN:
                    psqtBonus = QUEEN_TABLE[tableRow][col];
                    break;
                case PieceType::KING:
                    psqtBonus = KING_MIDGAME_TABLE[tableRow][col];
                    break;
                default:
                    break;
            }
            
            if (piece.color == aiColor) {
                myScore += value + psqtBonus;
            } else {
                opponentScore += value + psqtBonus;
            }
        }
    }
    
    // Opening-specific bonuses
    myScore += evaluateDevelopment(board, aiColor) * 3;
    myScore += evaluateCenterControl(board, aiColor) * 2;
    myScore += evaluateKingSafety(board, aiColor) * 2;
    
    opponentScore += evaluateDevelopment(board, opponentColor) * 3;
    opponentScore += evaluateCenterControl(board, opponentColor) * 2;
    opponentScore += evaluateKingSafety(board, opponentColor) * 2;
    
    return myScore - opponentScore;
}

// MIDGAME evaluation: Focus on tactics and piece activity
int EvaluatorV2::evaluateMidgame(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    // Material count with piece-square tables
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty()) continue;
            
            int value = PIECE_VALUES[static_cast<int>(piece.type)];
            
            int psqtBonus = 0;
            int tableRow = (piece.color == Color::WHITE) ? row : (7 - row);
            
            switch (piece.type) {
                case PieceType::PAWN:
                    psqtBonus = PAWN_TABLE[tableRow][col];
                    break;
                case PieceType::KNIGHT:
                    psqtBonus = KNIGHT_TABLE[tableRow][col];
                    break;
                case PieceType::BISHOP:
                    psqtBonus = BISHOP_TABLE[tableRow][col];
                    break;
                case PieceType::ROOK:
                    psqtBonus = ROOK_TABLE[tableRow][col];
                    break;
                case PieceType::QUEEN:
                    psqtBonus = QUEEN_TABLE[tableRow][col];
                    break;
                case PieceType::KING:
                    psqtBonus = KING_MIDGAME_TABLE[tableRow][col];
                    break;
                default:
                    break;
            }
            
            if (piece.color == aiColor) {
                myScore += value + psqtBonus;
            } else {
                opponentScore += value + psqtBonus;
            }
        }
    }
    
    // Midgame-specific bonuses
    myScore += evaluateMobility(board, aiColor) * 2;
    myScore += evaluatePawnStructure(board, aiColor);
    myScore += evaluateKingSafety(board, aiColor);
    myScore += evaluateCenterControl(board, aiColor);
    
    opponentScore += evaluateMobility(board, opponentColor) * 2;
    opponentScore += evaluatePawnStructure(board, opponentColor);
    opponentScore += evaluateKingSafety(board, opponentColor);
    opponentScore += evaluateCenterControl(board, opponentColor);
    
    return myScore - opponentScore;
}

// ENDGAME evaluation: Focus on king activity and pawn promotion
int EvaluatorV2::evaluateEndgame(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    // Material count with endgame piece-square tables
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty()) continue;
            
            int value = PIECE_VALUES[static_cast<int>(piece.type)];
            
            int psqtBonus = 0;
            int tableRow = (piece.color == Color::WHITE) ? row : (7 - row);
            
            switch (piece.type) {
                case PieceType::PAWN:
                    psqtBonus = PAWN_TABLE[tableRow][col] * 2; // Pawns more valuable in endgame
                    break;
                case PieceType::KNIGHT:
                    psqtBonus = KNIGHT_TABLE[tableRow][col] / 2; // Knights less valuable
                    break;
                case PieceType::BISHOP:
                    psqtBonus = BISHOP_TABLE[tableRow][col];
                    break;
                case PieceType::ROOK:
                    psqtBonus = ROOK_TABLE[tableRow][col];
                    break;
                case PieceType::QUEEN:
                    psqtBonus = QUEEN_TABLE[tableRow][col];
                    break;
                case PieceType::KING:
                    psqtBonus = KING_ENDGAME_TABLE[tableRow][col]; // King should be active
                    break;
                default:
                    break;
            }
            
            if (piece.color == aiColor) {
                myScore += value + psqtBonus;
            } else {
                opponentScore += value + psqtBonus;
            }
        }
    }
    
    // Endgame-specific bonuses
    myScore += evaluateKingActivity(board, aiColor) * 3;
    myScore += evaluatePawnAdvancement(board, aiColor) * 3;
    myScore += evaluatePassedPawns(board, aiColor) * 4;
    
    opponentScore += evaluateKingActivity(board, opponentColor) * 3;
    opponentScore += evaluatePawnAdvancement(board, opponentColor) * 3;
    opponentScore += evaluatePassedPawns(board, opponentColor) * 4;
    
    return myScore - opponentScore;
}

// Helper evaluation functions
int EvaluatorV2::evaluateDevelopment(const Board& board, Color color) {
    int score = 0;
    int backRank = (color == Color::WHITE) ? 0 : 7;
    
    // Penalize pieces still on back rank (except rooks and king)
    for (int col = 0; col < 8; col++) {
        Piece piece = board.getPiece(Position(backRank, col));
        if (!piece.isEmpty() && piece.color == color) {
            if (piece.type == PieceType::KNIGHT || piece.type == PieceType::BISHOP) {
                score -= 10;
            } else if (piece.type == PieceType::QUEEN && col == 3) {
                score -= 15; // Queen shouldn't move too early
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluateCenterControl(const Board& board, Color color) {
    int score = 0;
    
    // Center squares (d4, d5, e4, e5)
    Position centerSquares[] = {Position(3, 3), Position(3, 4), Position(4, 3), Position(4, 4)};
    
    for (const Position& pos : centerSquares) {
        Piece piece = board.getPiece(pos);
        if (!piece.isEmpty() && piece.color == color) {
            if (piece.type == PieceType::PAWN) {
                score += 20;
            } else {
                score += 10;
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluateKingSafety(const Board& board, Color color) {
    int score = 0;
    
    // Find king position
    Position kingPos(-1, -1);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING && piece.color == color) {
                kingPos = Position(row, col);
                break;
            }
        }
        if (kingPos.isValid()) break;
    }
    
    if (!kingPos.isValid()) return 0;
    
    // Bonus for pawn shield in front of king
    int direction = (color == Color::WHITE) ? 1 : -1;
    for (int colOffset = -1; colOffset <= 1; colOffset++) {
        Position shieldPos(kingPos.row + direction, kingPos.col + colOffset);
        if (shieldPos.isValid()) {
            Piece piece = board.getPiece(shieldPos);
            if (piece.type == PieceType::PAWN && piece.color == color) {
                score += 15;
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluatePawnStructure(const Board& board, Color color) {
    int score = 0;
    
    // Penalize doubled pawns, reward connected pawns
    for (int col = 0; col < 8; col++) {
        int pawnCount = 0;
        bool hasConnected = false;
        
        for (int row = 0; row < 8; row++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::PAWN && piece.color == color) {
                pawnCount++;
                
                // Check for connected pawns (on adjacent files)
                if (col > 0) {
                    Piece left = board.getPiece(Position(row, col - 1));
                    if (left.type == PieceType::PAWN && left.color == color) {
                        hasConnected = true;
                    }
                }
                if (col < 7) {
                    Piece right = board.getPiece(Position(row, col + 1));
                    if (right.type == PieceType::PAWN && right.color == color) {
                        hasConnected = true;
                    }
                }
            }
        }
        
        if (pawnCount > 1) score -= 20 * (pawnCount - 1); // Doubled pawns penalty
        if (hasConnected) score += 10;
    }
    
    return score;
}

int EvaluatorV2::evaluateMobility(const Board& board, Color color) {
    int score = 0;
    
    // Count number of legal moves (simplified - just count piece moves)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (!piece.isEmpty() && piece.color == color) {
                std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                    board, Position(row, col), piece);
                score += moves.size();
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluateKingActivity(const Board& board, Color color) {
    int score = 0;
    
    // Find king and count how central/active it is
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING && piece.color == color) {
                // Reward king being in center in endgame
                int distFromCenter = std::abs(row - 3.5) + std::abs(col - 3.5);
                score += 20 - (distFromCenter * 3);
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluatePawnAdvancement(const Board& board, Color color) {
    int score = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::PAWN && piece.color == color) {
                int advancement = (color == Color::WHITE) ? row : (7 - row);
                score += advancement * advancement; // Quadratic bonus for advanced pawns
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluatePassedPawns(const Board& board, Color color) {
    int score = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::PAWN && piece.color == color) {
                bool isPassedPawn = true;
                
                // Check if there are enemy pawns blocking or attacking
                int direction = (color == Color::WHITE) ? 1 : -1;
                for (int checkRow = row + direction; checkRow >= 0 && checkRow < 8; checkRow += direction) {
                    for (int checkCol = std::max(0, col - 1); checkCol <= std::min(7, col + 1); checkCol++) {
                        Piece checkPiece = board.getPiece(Position(checkRow, checkCol));
                        if (checkPiece.type == PieceType::PAWN && checkPiece.color != color) {
                            isPassedPawn = false;
                            break;
                        }
                    }
                    if (!isPassedPawn) break;
                }
                
                if (isPassedPawn) {
                    int advancement = (color == Color::WHITE) ? row : (7 - row);
                    score += 50 + (advancement * 10);
                }
            }
        }
    }
    
    return score;
}

// MinimaxEngineV2 implementation
MinimaxEngineV2::MinimaxEngineV2(int depth) : depth_(depth), maxTime_(0), timeExpired_(false) {}

void MinimaxEngineV2::setDepth(int depth) {
    depth_ = depth;
}

void MinimaxEngineV2::setMaxTime(int milliseconds) {
    maxTime_ = milliseconds;
}

bool MinimaxEngineV2::isTimeExpired() const {
    if (maxTime_ <= 0) return false; // No time limit
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStartTime_
    ).count();
    
    return elapsed >= maxTime_;
}

bool MinimaxEngineV2::isCheckmate(const Board& board, Color color, const CastlingRights& castling) {
    // Check if king is in check
    if (!MoveGenerator::isKingInCheck(board, color)) {
        return false;
    }
    
    // Check if there are any legal moves
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    return moves.empty();
}

bool MinimaxEngineV2::isStalemate(const Board& board, Color color, const CastlingRights& castling) {
    // Check if king is NOT in check
    if (MoveGenerator::isKingInCheck(board, color)) {
        return false;
    }
    
    // Check if there are any legal moves
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    return moves.empty();
}

CastlingRights MinimaxEngineV2::updateCastlingRights(
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

int MinimaxEngineV2::minimax(const Board& board, Color currentColor, int depth,
                             bool maximizing, int alpha, int beta,
                             const CastlingRights& castling, int ply) {
    
    // Check time limit
    if (isTimeExpired()) {
        timeExpired_ = true;
        return EvaluatorV2::evaluate(board, rootColor_, currentPhase_);
    }
    
    // Check for checkmate or stalemate at this position
    if (isCheckmate(board, currentColor, castling)) {
        // Checkmate is very bad for the current player
        // Use ply to prefer quicker checkmates
        return maximizing ? -(CHECKMATE_SCORE - ply) : (CHECKMATE_SCORE - ply);
    }
    
    if (isStalemate(board, currentColor, castling)) {
        return STALEMATE_SCORE;
    }
    
    if (depth == 0) {
        return EvaluatorV2::evaluate(board, rootColor_, currentPhase_);
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    if (moves.empty()) {
        return STALEMATE_SCORE;
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
        
        int score = minimax(newBoard, nextColor, depth - 1, !maximizing, alpha, beta, newCastling, ply + 1);
        
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

Move MinimaxEngineV2::findBestMove(const Board& board, Color color, const CastlingRights& castling) {
    rootColor_ = color;
    currentPhase_ = EvaluatorV2::detectPhase(board);
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
    
    // Check for immediate checkmate moves
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        if (isCheckmate(newBoard, nextColor, newCastling)) {
            // Found an immediate checkmate - return it immediately!
            return move;
        }
    }
    
    // No immediate checkmate, do full minimax search
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = minimax(newBoard, nextColor, depth_ - 1, false, alpha, beta, newCastling, 1);
        
        // Prioritize checkmate moves
        if (score >= CHECKMATE_SCORE - 100) {
            return move;
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        
        alpha = std::max(alpha, score);
    }
    
    return bestMove;
}

} // namespace Chess
