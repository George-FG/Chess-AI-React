#include "MinimaxEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>

namespace Chess {

// ============================
// PIECE-SQUARE TABLES
// From Simplified Evaluation Function & PeSTO
// ============================

// Pawn PST (opening/middlegame)
static const int PAWN_PST_MG[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

// Pawn PST (endgame)
static const int PAWN_PST_EG[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

// Knight PST (opening/middlegame)
static const int KNIGHT_PST_MG[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

// Knight PST (endgame)
static const int KNIGHT_PST_EG[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

// Bishop PST (opening/middlegame)
static const int BISHOP_PST_MG[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

// Bishop PST (endgame)
static const int BISHOP_PST_EG[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

// Rook PST (opening/middlegame)
static const int ROOK_PST_MG[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

// Rook PST (endgame)
static const int ROOK_PST_EG[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};

// Queen PST (opening/middlegame)
static const int QUEEN_PST_MG[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

// Queen PST (endgame)
static const int QUEEN_PST_EG[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

// King PST (opening/middlegame)
static const int KING_PST_MG[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

// King PST (endgame)
static const int KING_PST_EG[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

// ============================
// GAME PHASE CALCULATION
// ============================
static int computeGamePhase(const Board& board) {
    // Calculate phase based on remaining material
    // Phase: 0 = endgame, 256 = opening
    int phase = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) continue;
            
            switch (p.type) {
                case PieceType::PAWN:   break; // Pawns don't affect phase
                case PieceType::KNIGHT: phase += 1; break;
                case PieceType::BISHOP: phase += 1; break;
                case PieceType::ROOK:   phase += 2; break;
                case PieceType::QUEEN:  phase += 4; break;
                default: break;
            }
        }
    }
    
    // Scale to 0-256 (max phase is 24 = 4 knights + 4 bishops + 4 rooks + 2 queens)
    return std::min(256, (phase * 256 + 12) / 24);
}

// Tapered eval: interpolate between middlegame and endgame scores
static int taperedEval(int mg, int eg, int phase) {
    return ((mg * phase) + (eg * (256 - phase))) / 256;
}

// ============================
// PST LOOKUP (with color flip for black)
// ============================
static int getPST(PieceType type, Position pos, Color color, bool isEndgame) {
    // Flip position for black (they see board upside down)
    int sq = (color == Color::WHITE) ? (pos.row * 8 + pos.col) : ((7 - pos.row) * 8 + pos.col);
    
    if (isEndgame) {
        switch (type) {
            case PieceType::PAWN:   return PAWN_PST_EG[sq];
            case PieceType::KNIGHT: return KNIGHT_PST_EG[sq];
            case PieceType::BISHOP: return BISHOP_PST_EG[sq];
            case PieceType::ROOK:   return ROOK_PST_EG[sq];
            case PieceType::QUEEN:  return QUEEN_PST_EG[sq];
            case PieceType::KING:   return KING_PST_EG[sq];
            default: return 0;
        }
    } else {
        switch (type) {
            case PieceType::PAWN:   return PAWN_PST_MG[sq];
            case PieceType::KNIGHT: return KNIGHT_PST_MG[sq];
            case PieceType::BISHOP: return BISHOP_PST_MG[sq];
            case PieceType::ROOK:   return ROOK_PST_MG[sq];
            case PieceType::QUEEN:  return QUEEN_PST_MG[sq];
            case PieceType::KING:   return KING_PST_MG[sq];
            default: return 0;
        }
    }
}

// ============================
// EVALUATOR
// ============================

const int Evaluator::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    20000,// KING
    0     // NONE
};

// Main evaluation function
int Evaluator::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, int moveCount) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int phase = computeGamePhase(board);
    bool isEarly = (moveCount < 10);
    bool isLate = (phase < 64); // Entering endgame
    
    int mgScore[2] = {0, 0}; // [WHITE, BLACK]
    int egScore[2] = {0, 0};
    
    // Material + PST
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) continue;
            
            int colorIdx = (p.color == Color::WHITE) ? 0 : 1;
            int material = PIECE_VALUES[static_cast<int>(p.type)];
            
            int pstMG = getPST(p.type, Position(r, c), p.color, false);
            int pstEG = getPST(p.type, Position(r, c), p.color, true);
            
            mgScore[colorIdx] += material + pstMG;
            egScore[colorIdx] += material + pstEG;
        }
    }
    
    // Pawn structure bonuses
    for (int c = 0; c < 8; c++) {
        int whitePawns = 0, blackPawns = 0;
        for (int r = 0; r < 8; r++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.type == PieceType::PAWN) {
                if (p.color == Color::WHITE) whitePawns++;
                else blackPawns++;
            }
        }
        
        // Doubled pawns penalty
        if (whitePawns > 1) {
            mgScore[0] -= 10 * (whitePawns - 1);
            egScore[0] -= 20 * (whitePawns - 1);
        }
        if (blackPawns > 1) {
            mgScore[1] -= 10 * (blackPawns - 1);
            egScore[1] -= 20 * (blackPawns - 1);
        }
        
        // Isolated pawns
        bool whiteIsolated = (whitePawns > 0);
        bool blackIsolated = (blackPawns > 0);
        
        if (c > 0) {
            for (int r = 0; r < 8; r++) {
                Piece p = board.getPiece(Position(r, c - 1));
                if (p.type == PieceType::PAWN) {
                    if (p.color == Color::WHITE) whiteIsolated = false;
                    if (p.color == Color::BLACK) blackIsolated = false;
                }
            }
        }
        if (c < 7) {
            for (int r = 0; r < 8; r++) {
                Piece p = board.getPiece(Position(r, c + 1));
                if (p.type == PieceType::PAWN) {
                    if (p.color == Color::WHITE) whiteIsolated = false;
                    if (p.color == Color::BLACK) blackIsolated = false;
                }
            }
        }
        
        if (whiteIsolated && whitePawns > 0) {
            mgScore[0] -= 15;
            egScore[0] -= 20;
        }
        if (blackIsolated && blackPawns > 0) {
            mgScore[1] -= 15;
            egScore[1] -= 20;
        }
    }
    
    // Mobility (simplified - count available moves)
    std::vector<Move> whiteMoves = MoveGenerator::generateMoves(board, Color::WHITE, castling);
    std::vector<Move> blackMoves = MoveGenerator::generateMoves(board, Color::BLACK, castling);
    
    mgScore[0] += whiteMoves.size() * 2;
    mgScore[1] += blackMoves.size() * 2;
    egScore[0] += whiteMoves.size() * 3;
    egScore[1] += blackMoves.size() * 3;
    
    // Bishop pair bonus
    int whiteBishops = 0, blackBishops = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.type == PieceType::BISHOP) {
                if (p.color == Color::WHITE) whiteBishops++;
                else blackBishops++;
            }
        }
    }
    if (whiteBishops >= 2) {
        mgScore[0] += 30;
        egScore[0] += 50;
    }
    if (blackBishops >= 2) {
        mgScore[1] += 30;
        egScore[1] += 50;
    }
    
    // Rook on open file bonus
    for (int c = 0; c < 8; c++) {
        bool hasPawn = false;
        for (int r = 0; r < 8; r++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.type == PieceType::PAWN) {
                hasPawn = true;
                break;
            }
        }
        
        if (!hasPawn) {
            for (int r = 0; r < 8; r++) {
                Piece p = board.getPiece(Position(r, c));
                if (p.type == PieceType::ROOK) {
                    if (p.color == Color::WHITE) {
                        mgScore[0] += 20;
                        egScore[0] += 20;
                    } else {
                        mgScore[1] += 20;
                        egScore[1] += 20;
                    }
                }
            }
        }
    }
    
    // Tempo bonus (side to move)
    int tempo = 10;
    
    // Taper evaluation
    int whiteFinal = taperedEval(mgScore[0], egScore[0], phase);
    int blackFinal = taperedEval(mgScore[1], egScore[1], phase);
    
    // Return from AI's perspective
    int score = (aiColor == Color::WHITE) ? 
                (whiteFinal - blackFinal + tempo) : 
                (blackFinal - whiteFinal + tempo);
    
    return score;
}

bool Evaluator::isInOpeningPhase(int moveCount) {
    return moveCount < 12;
}

bool Evaluator::isInEndgame(const Board& board) {
    return computeGamePhase(board) < 64;
}

int Evaluator::countMaterial(const Board& board, Color color) {
    int material = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.color == color && !p.isEmpty()) {
                material += PIECE_VALUES[static_cast<int>(p.type)];
            }
        }
    }
    return material;
}

// Stub implementations for compatibility
int Evaluator::evaluatePieceDevelopment(const Board&, Color) { return 0; }
int Evaluator::evaluateEndgame(const Board&, Color) { return 0; }
bool Evaluator::isPassedPawn(const Board&, Position, Color) { return false; }
int Evaluator::evaluateKingActivity(const Board&, Color, bool) { return 0; }

// ============================
// MINIMAX ENGINE
// ============================

MinimaxEngine::MinimaxEngine(int depth)
    : depth_(depth), maxTime_(0), timeExpired_(false), moveCount_(0) {}

void MinimaxEngine::setDepth(int depth) { depth_ = depth; }
void MinimaxEngine::setMaxTime(int milliseconds) { maxTime_ = milliseconds; }

bool MinimaxEngine::isTimeExpired() const {
    if (maxTime_ <= 0) return false;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStartTime_
    ).count();
    return elapsed >= maxTime_;
}

std::string MinimaxEngine::getPositionHash(const Board& board) {
    std::string hash;
    hash.reserve(64);
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) {
                hash += '.';
            } else {
                char ch = '?';
                switch (p.type) {
                    case PieceType::PAWN:   ch = 'p'; break;
                    case PieceType::KNIGHT: ch = 'n'; break;
                    case PieceType::BISHOP: ch = 'b'; break;
                    case PieceType::ROOK:   ch = 'r'; break;
                    case PieceType::QUEEN:  ch = 'q'; break;
                    case PieceType::KING:   ch = 'k'; break;
                    default: break;
                }
                if (p.color == Color::WHITE) ch = toupper(ch);
                hash += ch;
            }
        }
    }
    return hash;
}

CastlingRights MinimaxEngine::updateCastlingRights(const CastlingRights& rights, const Move& move, const Board&) {
    CastlingRights newRights = rights;
    
    if (move.piece.type == PieceType::KING) {
        if (move.piece.color == Color::WHITE) {
            newRights.whiteKingSide = false;
            newRights.whiteQueenSide = false;
        } else {
            newRights.blackKingSide = false;
            newRights.blackQueenSide = false;
        }
    }
    
    if (move.piece.type == PieceType::ROOK) {
        int homeRow = (move.piece.color == Color::WHITE) ? 0 : 7;
        if (move.from.row == homeRow) {
            if (move.piece.color == Color::WHITE) {
                if (move.from.col == 0) newRights.whiteKingSide = false;
                if (move.from.col == 7) newRights.whiteQueenSide = false;
            } else {
                if (move.from.col == 0) newRights.blackKingSide = false;
                if (move.from.col == 7) newRights.blackQueenSide = false;
            }
        }
    }
    
    return newRights;
}

// MVV-LVA scoring for captures
static int mvvLva(const Move& m) {
    if (m.captured.isEmpty()) return 0;
    int victim = static_cast<int>(m.captured.type);
    int attacker = static_cast<int>(m.piece.type);
    return (victim * 10) - attacker;
}

// Quiescence search
int MinimaxEngine::quiescence(const Board& board, Color currentColor, bool maximizing,
                              int alpha, int beta, const CastlingRights& castling, int moveCount) {
    if (isTimeExpired()) return Evaluator::evaluate(board, rootColor_, castling, moveCount);
    
    int standPat = Evaluator::evaluate(board, rootColor_, castling, moveCount);
    
    if (maximizing) {
        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;
    } else {
        if (standPat <= alpha) return alpha;
        if (beta > standPat) beta = standPat;
    }
    
    // Only search captures
    std::vector<Move> allMoves = MoveGenerator::generateMoves(board, currentColor, castling);
    std::vector<Move> captures;
    for (const auto& m : allMoves) {
        if (!m.captured.isEmpty()) captures.push_back(m);
    }
    
    // Order captures by MVV-LVA
    std::sort(captures.begin(), captures.end(), [](const Move& a, const Move& b) {
        return mvvLva(a) > mvvLva(b);
    });
    
    for (const auto& move : captures) {
        Board nb = board.clone();
        nb.applyMove(move);
        
        if (MoveGenerator::isKingInCheck(nb, currentColor)) continue;
        
        CastlingRights nc = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = quiescence(nb, nextColor, !maximizing, alpha, beta, nc, moveCount);
        
        if (maximizing) {
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        } else {
            if (score < beta) beta = score;
            if (beta <= alpha) break;
        }
    }
    
    return maximizing ? alpha : beta;
}

// Minimax with alpha-beta pruning
int MinimaxEngine::minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                          int alpha, int beta, const CastlingRights& castling, int moveCount) {
    if (isTimeExpired()) return Evaluator::evaluate(board, rootColor_, castling, moveCount);
    
    if (depth == 0) {
        return quiescence(board, currentColor, maximizing, alpha, beta, castling, moveCount);
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    if (moves.empty()) {
        bool inCheck = MoveGenerator::isKingInCheck(board, currentColor);
        if (inCheck) {
            // Checkmate
            int mateScore = maximizing ? -500000 : 500000;
            return mateScore + (maximizing ? depth * 10 : -depth * 10);
        }
        return 0; // Stalemate
    }
    
    // Move ordering
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        int aScore = 0, bScore = 0;
        
        if (a.isCastling) aScore += 2000;
        if (b.isCastling) bScore += 2000;
        
        if (!a.captured.isEmpty()) aScore += 500 + mvvLva(a);
        if (!b.captured.isEmpty()) bScore += 500 + mvvLva(b);
        
        return aScore > bScore;
    });
    
    int bestScore = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    
    for (const Move& move : moves) {
        Board nb = board.clone();
        nb.applyMove(move);
        
        if (MoveGenerator::isKingInCheck(nb, currentColor)) continue;
        
        CastlingRights nc = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = minimax(nb, nextColor, depth - 1, !maximizing, alpha, beta, nc, moveCount);
        
        if (maximizing) {
            if (score > bestScore) bestScore = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        } else {
            if (score < bestScore) bestScore = score;
            if (score < beta) beta = score;
            if (beta <= alpha) break;
        }
    }
    
    return bestScore;
}

Move MinimaxEngine::findBestMove(const Board& board, Color color, const CastlingRights& castling,
                                const std::vector<std::string>& positionHistory) {
    rootColor_ = color;
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();
    moveCount_ = static_cast<int>(positionHistory.size());
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    if (moves.empty()) return Move();
    
    // Check for immediate mate
    for (const auto& move : moves) {
        Board nb = board.clone();
        nb.applyMove(move);
        
        if (MoveGenerator::isKingInCheck(nb, color)) continue;
        
        Color oppColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        CastlingRights nc = updateCastlingRights(castling, move, board);
        
        std::vector<Move> replies = MoveGenerator::generateMoves(nb, oppColor, nc);
        if (replies.empty() && MoveGenerator::isKingInCheck(nb, oppColor)) {
            Move mateMove = move;
            mateMove.searchDepth = 1;
            return mateMove; // Checkmate in 1
        }
    }
    
    Move bestMove = moves[0];
    int bestScore = std::numeric_limits<int>::min();
    int depthReached = 0;
    
    for (int currentDepth = 1; currentDepth <= depth_; currentDepth++) {
        if (isTimeExpired()) break;
        
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();
        Move depthBestMove = bestMove;
        int depthBestScore = std::numeric_limits<int>::min();
        
        for (const Move& move : moves) {
            if (isTimeExpired()) break;
            
            Board nb = board.clone();
            nb.applyMove(move);
            
            if (MoveGenerator::isKingInCheck(nb, color)) continue;
            
            CastlingRights nc = updateCastlingRights(castling, move, board);
            Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            
            int score = minimax(nb, nextColor, currentDepth - 1, false, alpha, beta, nc, moveCount_);
            
            // Avoid repetitions
            if (!positionHistory.empty()) {
                std::string posHash = getPositionHash(nb);
                for (const auto& oldPos : positionHistory) {
                    if (oldPos == posHash) {
                        score -= 300;
                        break;
                    }
                }
            }
            
            if (score > depthBestScore) {
                depthBestScore = score;
                depthBestMove = move;
            }
            
            if (score > alpha) alpha = score;
        }
        
        // Update best move only if we completed this depth
        if (!isTimeExpired()) {
            bestMove = depthBestMove;
            bestScore = depthBestScore;
            depthReached = currentDepth;
        }
        
        // Early exit if mate found
        if (bestScore > 450000) break;
    }
    
    bestMove.searchDepth = depthReached;
    
    return bestMove;
}

} // namespace Chess
