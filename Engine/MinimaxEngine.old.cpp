#include "MinimaxEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Chess {

// ============================
// Helpers
// ============================
static inline Color other(Color c) { return (c == Color::WHITE) ? Color::BLACK : Color::WHITE; }

// Assumes common orientation: row 7 is White home rank (1st rank), row 0 is Black home rank (8th rank).
static inline int homeBackRank(Color c) { return (c == Color::WHITE) ? 7 : 0; }
static inline int homePawnRank(Color c) { return (c == Color::WHITE) ? 6 : 1; }

// ============================
// Evaluator
// ============================

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
bool Evaluator::isInOpeningPhase(int moveCount) { return moveCount < 15; }

// Evaluate piece development for opening
int Evaluator::evaluatePieceDevelopment(const Board& board, Color color) {
    int score = 0;
    const int backRank = homeBackRank(color);
    const int pawnRank = homePawnRank(color);

    // Knights should move off back rank
    {
        Piece n1 = board.getPiece(Position(backRank, 1)); // b-file
        Piece n2 = board.getPiece(Position(backRank, 6)); // g-file
        score += (n1.type == PieceType::KNIGHT && n1.color == color) ? -30 : +25;
        score += (n2.type == PieceType::KNIGHT && n2.color == color) ? -30 : +25;
    }

    // Bishops should move off back rank
    {
        Piece b1 = board.getPiece(Position(backRank, 2)); // c-file
        Piece b2 = board.getPiece(Position(backRank, 5)); // f-file
        score += (b1.type == PieceType::BISHOP && b1.color == color) ? -25 : +20;
        score += (b2.type == PieceType::BISHOP && b2.color == color) ? -25 : +20;
    }

    // Penalize early queen moves in opening (queen home is d-file col 3)
    {
        Piece qHome = board.getPiece(Position(backRank, 3));
        bool queenOnHome = (qHome.type == PieceType::QUEEN && qHome.color == color);
        if (!queenOnHome) {
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(Position(r, c));
                    if (p.type == PieceType::QUEEN && p.color == color) {
                        score -= 40;
                        r = 8;
                        break;
                    }
                }
            }
        }
    }

    // Bonus for center pawn development (d/e pawns moved)
    {
        Piece dPawn = board.getPiece(Position(pawnRank, 3));
        Piece ePawn = board.getPiece(Position(pawnRank, 4));
        if (!(dPawn.type == PieceType::PAWN && dPawn.color == color)) score += 15;
        if (!(ePawn.type == PieceType::PAWN && ePawn.color == color)) score += 15;
    }

    return score;
}

// Check if we're in the endgame (few pieces left)
bool Evaluator::isInEndgame(const Board& board) {
    int totalMaterial = 0;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (!piece.isEmpty() && piece.type != PieceType::KING && piece.type != PieceType::PAWN) {
                totalMaterial += PIECE_VALUES[static_cast<int>(piece.type)];
            }
        }
    }

    return totalMaterial < 1300;
}

// Count total material for a color (including pawns)
int Evaluator::countMaterial(const Board& board, Color color) {
    int material = 0;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (!piece.isEmpty() && piece.color == color) {
                material += PIECE_VALUES[static_cast<int>(piece.type)];
            }
        }
    }

    return material;
}

// Check if a pawn is passed (no enemy pawns in front or on adjacent files)
bool Evaluator::isPassedPawn(const Board& board, Position pawnPos, Color pawnColor) {
    int direction = (pawnColor == Color::WHITE) ? -1 : 1;

    for (int fileOffset = -1; fileOffset <= 1; fileOffset++) {
        int checkCol = pawnPos.col + fileOffset;
        if (checkCol < 0 || checkCol >= 8) continue;

        for (int row = pawnPos.row + direction; row >= 0 && row < 8; row += direction) {
            Piece piece = board.getPiece(Position(row, checkCol));
            if (piece.type == PieceType::PAWN && piece.color != pawnColor) return false;
        }
    }

    return true;
}

// Evaluate king activity in endgame
int Evaluator::evaluateKingActivity(const Board& board, Color color, bool isEndgame) {
    if (!isEndgame) return 0;

    int score = 0;
    Position kingPos(-1, -1);

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (piece.type == PieceType::KING && piece.color == color) {
                kingPos = Position(r, c);
                break;
            }
        }
        if (kingPos.isValid()) break;
    }

    if (!kingPos.isValid()) return 0;

    // Centralization bonus
    double centerR = 3.5, centerC = 3.5;
    double dist = std::abs(kingPos.row - centerR) + std::abs(kingPos.col - centerC);
    score += static_cast<int>((7.0 - dist) * 5.0);

    // Support/blockade pawns
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (piece.type != PieceType::PAWN) continue;
            int d = std::abs(r - kingPos.row) + std::abs(c - kingPos.col);
            if (d <= 2) score += (piece.color == color) ? 20 : 15;
        }
    }

    return score;
}

// Comprehensive endgame evaluation
int Evaluator::evaluateEndgame(const Board& board, Color aiColor) {
    int score = 0;
    Color opp = other(aiColor);

    int myMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, opp);
    bool winning = myMaterial > oppMaterial + 300;

    // Pawns
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (piece.type != PieceType::PAWN) continue;

            int adv = (piece.color == Color::WHITE) ? (7 - r) : r;

            if (piece.color == aiColor) {
                score += adv * adv * 5;
                if (isPassedPawn(board, Position(r, c), piece.color)) {
                    score += 50 + adv * 25;
                    if (adv >= 5) score += 100;

                    // King support
                    for (int kr = 0; kr < 8; kr++) {
                        for (int kc = 0; kc < 8; kc++) {
                            Piece k = board.getPiece(Position(kr, kc));
                            if (k.type == PieceType::KING && k.color == aiColor) {
                                if (std::abs(kr - r) + std::abs(kc - c) <= 2) score += 30;
                            }
                        }
                    }
                }
            } else {
                if (isPassedPawn(board, Position(r, c), piece.color)) {
                    score -= 100 + adv * 30;
                    if (adv >= 5) score -= 200;
                }
            }
        }
    }

    score += evaluateKingActivity(board, aiColor, true);
    score -= evaluateKingActivity(board, opp, true);

    // When winning, push pieces closer to enemy king
    if (winning) {
        Position enemyKing(-1, -1);
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece p = board.getPiece(Position(r, c));
                if (p.type == PieceType::KING && p.color == opp) {
                    enemyKing = Position(r, c);
                    break;
                }
            }
            if (enemyKing.isValid()) break;
        }

        if (enemyKing.isValid()) {
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(Position(r, c));
                    if (p.isEmpty() || p.color != aiColor) continue;

                    int d = std::abs(r - enemyKing.row) + std::abs(c - enemyKing.col);
                    int closeness = 14 - d;

                    switch (p.type) {
                        case PieceType::QUEEN:  score += closeness * 8; break;
                        case PieceType::ROOK:   score += closeness * 5; break;
                        case PieceType::BISHOP:
                        case PieceType::KNIGHT: score += closeness * 3; break;
                        case PieceType::KING:   score += closeness * 6; break;
                        default: break;
                    }
                }
            }
        }
    }

    // Shape: pawns slightly more, knights slightly less
    int pawnDelta = 0;
    int knightDelta = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) continue;

            if (p.color == aiColor) {
                if (p.type == PieceType::PAWN) pawnDelta++;
                if (p.type == PieceType::KNIGHT) knightDelta--;
            } else {
                if (p.type == PieceType::PAWN) pawnDelta--;
                if (p.type == PieceType::KNIGHT) knightDelta++;
            }
        }
    }
    score += pawnDelta * 20;
    score += knightDelta * 15;

    return score;
}

// Main evaluation
int Evaluator::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, int moveCount) {
    Color opp = other(aiColor);
    bool opening = isInOpeningPhase(moveCount);

    int myScore = 0;
    int oppScore = 0;

    // Material + simple pawn bonuses
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (piece.isEmpty()) continue;

            int value = PIECE_VALUES[static_cast<int>(piece.type)];

            if (piece.color == aiColor) {
                myScore += value;
                if (piece.type == PieceType::PAWN) {
                    int tr = (aiColor == Color::WHITE) ? (7 - r) : r;
                    bool center = (c == 3 || c == 4);
                    if (center) {
                        if (tr == 2) myScore += 2;
                        else if (tr == 3) myScore += 5;
                        else if (tr == 4) myScore += 8;
                    }
                    if (tr >= 5) myScore += tr * 3;
                }
            } else {
                oppScore += value;
                if (piece.type == PieceType::PAWN) {
                    int tr = (opp == Color::WHITE) ? (7 - r) : r;
                    bool center = (c == 3 || c == 4);
                    if (center) {
                        if (tr == 2) oppScore += 2;
                        else if (tr == 3) oppScore += 5;
                        else if (tr == 4) oppScore += 8;
                    }
                    if (tr >= 5) oppScore += tr * 3;
                }
            }
        }
    }

    // King safety: reward castling, penalize early king wandering
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece k = board.getPiece(Position(r, c));
            if (k.type != PieceType::KING) continue;

            int br = homeBackRank(k.color);

            auto apply = [&](bool mine) {
                int& s = mine ? myScore : oppScore;

                if (r == br) {
                    if (c == 6 || c == 2) {
                        s += 150; // castled
                    } else if (c == 4) {
                        s -= opening ? 20 : 10;
                    } else {
                        s -= opening ? 120 : 80;
                    }
                } else {
                    s -= opening ? 160 : 100;
                }
            };

            apply(k.color == aiColor);
        }
    }

    bool endgame = isInEndgame(board);

    if (opening) {
        myScore += evaluatePieceDevelopment(board, aiColor);
        oppScore += evaluatePieceDevelopment(board, opp);

        // Extra bonus for castling completed in opening
        auto castledBonus = [&](Color side, int& scoreRef) {
            int br = homeBackRank(side);
            Piece kStart = board.getPiece(Position(br, 4));
            bool onStart = (kStart.type == PieceType::KING && kStart.color == side);
            if (!onStart) {
                Piece kG = board.getPiece(Position(br, 6));
                Piece kC = board.getPiece(Position(br, 2));
                if ((kG.type == PieceType::KING && kG.color == side) ||
                    (kC.type == PieceType::KING && kC.color == side)) {
                    scoreRef += 200;
                }
            }
        };

        castledBonus(aiColor, myScore);
        castledBonus(opp, oppScore);
    }

    // Castling rights are valuable only if king still on e-file home square
    auto addCastlingRightsValue = [&](Color side, int& scoreRef) {
        int br = homeBackRank(side);
        Piece k = board.getPiece(Position(br, 4));
        if (!(k.type == PieceType::KING && k.color == side)) return;

        if (side == Color::WHITE) {
            if (castling.whiteKingSide) scoreRef += 40;
            if (castling.whiteQueenSide) scoreRef += 40;
        } else {
            if (castling.blackKingSide) scoreRef += 40;
            if (castling.blackQueenSide) scoreRef += 40;
        }
    };

    addCastlingRightsValue(aiColor, myScore);
    addCastlingRightsValue(opp, oppScore);

    // CRITICAL: Detect hanging pieces (undefended pieces that can be captured)
    auto detectHangingPieces = [&](Color side, int& scoreRef) {
        Color enemy = (side == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        // For each piece of this color, check if it's attacked and if so, if it's defended
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece piece = board.getPiece(Position(r, c));
                if (piece.isEmpty() || piece.color != side) continue;
                if (piece.type == PieceType::KING) continue; // Kings can't hang
                
                Position pos(r, c);
                
                // Check if any enemy piece attacks this square
                bool isAttacked = false;
                std::vector<Move> enemyMoves = MoveGenerator::generateMoves(board, enemy, castling);
                for (const auto& enemyMove : enemyMoves) {
                    if (enemyMove.to.row == r && enemyMove.to.col == c) {
                        isAttacked = true;
                        break;
                    }
                }
                
                if (isAttacked) {
                    // Check if it's defended by checking if capturing it would leave attacker hanging
                    // Simplified: just heavily penalize attacked pieces
                    int pieceValue = PIECE_VALUES[static_cast<int>(piece.type)];
                    scoreRef -= pieceValue / 2; // Penalty for being attacked
                }
            }
        }
    };
    
    detectHangingPieces(aiColor, myScore);
    detectHangingPieces(opp, oppScore);

    if (endgame) {
        int eg = evaluateEndgame(board, aiColor);
        return (myScore - oppScore) + eg;
    }

    return myScore - oppScore;
}

// ============================
// MinimaxEngine
//   - Mate-in-1 immediate selection at root
//   - Quiescence at leaf
//   - Check extension
//   - Simple TT
//   - Safety filter for self-check
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

// Assumes columns: a=0 ... h=7, kingside rook at col 7, queenside rook at col 0
CastlingRights MinimaxEngine::updateCastlingRights(
    const CastlingRights& rights, const Move& move, const Board& /*board*/) {

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
        int br = homeBackRank(move.piece.color);
        if (move.piece.color == Color::WHITE) {
            if (move.from.row == br && move.from.col == 0) newRights.whiteQueenSide = false;
            if (move.from.row == br && move.from.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.from.row == br && move.from.col == 0) newRights.blackQueenSide = false;
            if (move.from.row == br && move.from.col == 7) newRights.blackKingSide = false;
        }
    }

    if (!move.captured.isEmpty() && move.captured.type == PieceType::ROOK) {
        int br = homeBackRank(move.captured.color);
        if (move.captured.color == Color::WHITE) {
            if (move.to.row == br && move.to.col == 0) newRights.whiteQueenSide = false;
            if (move.to.row == br && move.to.col == 7) newRights.whiteKingSide = false;
        } else {
            if (move.to.row == br && move.to.col == 0) newRights.blackQueenSide = false;
            if (move.to.row == br && move.to.col == 7) newRights.blackKingSide = false;
        }
    }

    return newRights;
}

std::string MinimaxEngine::getPositionHash(const Board& board) {
    std::string hash;
    hash.reserve(128);
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (!p.isEmpty()) {
                hash += char('0' + r);
                hash += char('0' + c);
                hash += char('0' + static_cast<int>(p.type));
                hash += char('0' + static_cast<int>(p.color));
                hash += ';';
            }
        }
    }
    return hash;
}

static inline int pieceValueApprox(PieceType t) {
    switch (t) {
        case PieceType::PAWN: return 100;
        case PieceType::KNIGHT: return 320;
        case PieceType::BISHOP: return 330;
        case PieceType::ROOK: return 500;
        case PieceType::QUEEN: return 900;
        case PieceType::KING: return 20000;
        default: return 0;
    }
}

static inline int mvvLvaScore(const Move& m) {
    if (m.captured.isEmpty()) return 0;
    return pieceValueApprox(m.captured.type) * 10 - pieceValueApprox(m.piece.type);
}

// Basic TT
struct TTEntry {
    int depth = -1;
    int score = 0;
};
static std::unordered_map<std::string, TTEntry> g_tt;

static inline std::string makeTTKey(const std::string& posHash, Color stm, const CastlingRights& c) {
    std::string key = posHash;
    key += (stm == Color::WHITE) ? "|w|" : "|b|";
    key += c.whiteKingSide ? 'K' : '-';
    key += c.whiteQueenSide ? 'Q' : '-';
    key += c.blackKingSide ? 'k' : '-';
    key += c.blackQueenSide ? 'q' : '-';
    return key;
}

// Quiescence
int MinimaxEngine::quiescence(const Board& board,
                             Color currentColor,
                             bool maximizing,
                             int alpha,
                             int beta,
                             const CastlingRights& castling,
                             int moveCount) {
    if (isTimeExpired()) {
        timeExpired_ = true;
        return Evaluator::evaluate(board, rootColor_, castling, moveCount);
    }

    int standPat = Evaluator::evaluate(board, rootColor_, castling, moveCount);

    if (maximizing) {
        if (standPat >= beta) return standPat;
        alpha = std::max(alpha, standPat);
    } else {
        if (standPat <= alpha) return standPat;
        beta = std::min(beta, standPat);
    }

    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);

    std::vector<Move> caps;
    caps.reserve(moves.size());
    for (const auto& m : moves) {
        if (!m.captured.isEmpty()) caps.push_back(m);
    }

    std::sort(caps.begin(), caps.end(), [](const Move& a, const Move& b) {
        return mvvLvaScore(a) > mvvLvaScore(b);
    });

    for (const Move& move : caps) {
        Board nb = board.clone();
        nb.applyMove(move);

        // Safety: reject illegal self-check moves if generator leaks them
        if (MoveGenerator::isKingInCheck(nb, currentColor)) continue;

        CastlingRights nc = updateCastlingRights(castling, move, board);
        Color next = other(currentColor);

        int score = quiescence(nb, next, !maximizing, alpha, beta, nc, moveCount + 1);

        if (maximizing) {
            alpha = std::max(alpha, score);
            if (alpha >= beta) break;
        } else {
            beta = std::min(beta, score);
            if (beta <= alpha) break;
        }
    }

    return maximizing ? alpha : beta;
}

// Minimax
int MinimaxEngine::minimax(const Board& board,
                           Color currentColor,
                           int depth,
                           bool maximizing,
                           int alpha,
                           int beta,
                           const CastlingRights& castling,
                           int moveCount) {
    if (isTimeExpired()) {
        timeExpired_ = true;
        return Evaluator::evaluate(board, rootColor_, castling, moveCount);
    }

    // Check extension helps mates and defense
    if (depth > 0 && MoveGenerator::isKingInCheck(board, currentColor)) depth += 1;

    // TT lookup
    {
        std::string key = makeTTKey(getPositionHash(board), currentColor, castling);
        auto it = g_tt.find(key);
        if (it != g_tt.end() && it->second.depth >= depth) return it->second.score;
    }

    if (depth == 0) {
        return quiescence(board, currentColor, maximizing, alpha, beta, castling, moveCount);
    }

    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);

    if (moves.empty()) {
        bool inCheck = MoveGenerator::isKingInCheck(board, currentColor);
        if (inCheck) {
            // Mate: HUGE score to ensure it dominates, prefer quicker mates
            int mateScore = maximizing ? -500000 : 500000;
            int plyBonus = (maximizing ? depth * 10 : -depth * 10);
            return mateScore + plyBonus;
        }
        return 0; // stalemate
    }

    // Move ordering: checks first (when winning), castling, MVV-LVA captures, discourage king/rook moves
    std::sort(moves.begin(), moves.end(), [&castling, currentColor, &board](const Move& a, const Move& b) {
        auto scoreMove = [&](const Move& m) -> int {
            int s = 0;

            // Prioritize checks - test if move gives check
            Board testBoard = board.clone();
            testBoard.applyMove(m);
            Color enemyColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
            if (MoveGenerator::isKingInCheck(testBoard, enemyColor)) {
                s += 5000; // MASSIVE bonus for checking moves
            }

            if (m.isCastling) s += 2000;

            if (m.piece.type == PieceType::KING && !m.isCastling) s -= 400;

            if (m.piece.type == PieceType::ROOK && !m.isCastling) {
                int br = homeBackRank(currentColor);
                if (m.from.row == br && m.from.col == 7) {
                    if (currentColor == Color::WHITE && castling.whiteKingSide) s -= 300;
                    if (currentColor == Color::BLACK && castling.blackKingSide) s -= 300;
                }
                if (m.from.row == br && m.from.col == 0) {
                    if (currentColor == Color::WHITE && castling.whiteQueenSide) s -= 300;
                    if (currentColor == Color::BLACK && castling.blackQueenSide) s -= 300;
                }
            }

            if (!m.captured.isEmpty()) s += 500 + mvvLvaScore(m);

            return s;
        };

        return scoreMove(a) > scoreMove(b);
    });

    int bestScore = maximizing ? std::numeric_limits<int>::min()
                               : std::numeric_limits<int>::max();

    for (const Move& move : moves) {
        Board nb = board.clone();
        nb.applyMove(move);

        // Safety
        if (MoveGenerator::isKingInCheck(nb, currentColor)) continue;

        CastlingRights nc = updateCastlingRights(castling, move, board);
        Color next = other(currentColor);

        int score = minimax(nb, next, depth - 1, !maximizing, alpha, beta, nc, moveCount + 1);

        if (maximizing) {
            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, score);
        } else {
            bestScore = std::min(bestScore, score);
            beta = std::min(beta, score);
        }

        if (beta <= alpha) break;
    }

    // TT store
    {
        std::string key = makeTTKey(getPositionHash(board), currentColor, castling);
        auto& e = g_tt[key];
        if (e.depth < depth) {
            e.depth = depth;
            e.score = bestScore;
        }
    }

    return bestScore;
}

// ===== NEW: forced mate-in-1 detection at root =====
// If any legal move delivers immediate checkmate, return it immediately and skip search.
static bool isCheckmateAfterMove(const Board& boardAfterMove,
                                 Color sideToMoveNow,
                                 const CastlingRights& castlingNow) {
    // sideToMoveNow is the side that must respond.
    // Checkmate = in check AND has no legal moves.
    if (!MoveGenerator::isKingInCheck(boardAfterMove, sideToMoveNow)) return false;

    std::vector<Move> replies = MoveGenerator::generateMoves(boardAfterMove, sideToMoveNow, castlingNow);

    // Safety: ensure we only count truly legal replies
    for (const auto& reply : replies) {
        Board nb = boardAfterMove.clone();
        nb.applyMove(reply);
        if (!MoveGenerator::isKingInCheck(nb, sideToMoveNow)) {
            return false; // found at least one legal escape
        }
    }

    return true; // in check, no legal replies
}

Move MinimaxEngine::findBestMove(const Board& board,
                                Color color,
                                const CastlingRights& castling,
                                const std::vector<std::string>& positionHistory) {
    rootColor_ = color;
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();

    moveCount_ = static_cast<int>(positionHistory.size());

    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    if (moves.empty()) return Move();

    // --- Immediate mate-in-1 shortcut (what you asked for) ---
    // If any move checkmates the opponent immediately, play it and end search.
    for (const auto& move : moves) {
        Board nb = board.clone();
        nb.applyMove(move);

        // Reject illegal moves (self-check)
        if (MoveGenerator::isKingInCheck(nb, color)) continue;

        CastlingRights nc = updateCastlingRights(castling, move, board);
        Color opp = other(color);

        if (isCheckmateAfterMove(nb, opp, nc)) {
            return move; // FOUND CHECKMATE IN 1 -> stop here
        }
    }

    // Root ordering
    bool opening = Evaluator::isInOpeningPhase(moveCount_);

    auto rootScoreMove = [&](const Move& m) -> int {
        int s = 0;

        // Prioritize checking moves at root level too
        Board testBoard = board.clone();
        testBoard.applyMove(m);
        Color enemyColor = other(color);
        if (MoveGenerator::isKingInCheck(testBoard, enemyColor)) {
            s += 8000; // HUGE bonus for checks at root
        }

        if (m.isCastling) s += opening ? 6000 : 2000;

        if (opening) {
            int br = homeBackRank(color);
            if (m.piece.type == PieceType::KNIGHT && m.from.row == br) s += 600;
            if (m.piece.type == PieceType::BISHOP && m.from.row == br) s += 500;
            if (m.piece.type == PieceType::QUEEN) s -= 700;
            if (m.piece.type == PieceType::PAWN && (m.to.col == 3 || m.to.col == 4)) s += 350;
        }

        if (m.piece.type == PieceType::ROOK && !m.isCastling) {
            int br = homeBackRank(color);
            if (m.from.row == br && m.from.col == 7) {
                if (color == Color::WHITE && castling.whiteKingSide) s -= 600;
                if (color == Color::BLACK && castling.blackKingSide) s -= 600;
            }
            if (m.from.row == br && m.from.col == 0) {
                if (color == Color::WHITE && castling.whiteQueenSide) s -= 600;
                if (color == Color::BLACK && castling.blackQueenSide) s -= 600;
            }
        }

        if (m.piece.type == PieceType::KING && !m.isCastling) s -= 300;

        if (!m.captured.isEmpty()) s += 200 + mvvLvaScore(m);

        return s;
    };

    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return rootScoreMove(a) > rootScoreMove(b);
    });

    Move bestMove = moves[0];

    std::vector<std::pair<Move, int>> scored;
    scored.reserve(moves.size());

    for (int currentDepth = 1; currentDepth <= depth_; currentDepth++) {
        if (isTimeExpired()) break;

        int bestScore = std::numeric_limits<int>::min();
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();

        Move depthBestMove = moves[0];
        scored.clear();

        bool searchCompleted = true;

        for (const Move& move : moves) {
            if (isTimeExpired()) {
                searchCompleted = false;
                break;
            }

            Board nb = board.clone();
            nb.applyMove(move);

            if (MoveGenerator::isKingInCheck(nb, color)) continue;

            CastlingRights nc = updateCastlingRights(castling, move, board);
            Color next = other(color);

            int score = minimax(nb, next, currentDepth - 1, false, alpha, beta, nc, moveCount_);

            // CRITICAL: Detect if we're hanging a piece - HUGE penalty
            if (!move.captured.isEmpty()) {
                // Moving into a capture - check if the square is defended
            } else {
                // Check if we're leaving a piece undefended on the destination
                Piece movedPiece = nb.getPiece(move.to);
                if (!movedPiece.isEmpty() && movedPiece.type != PieceType::PAWN) {
                    // See if this square is attacked by opponent
                    Color opp = other(color);
                    std::vector<Move> oppMoves = MoveGenerator::generateMoves(nb, opp, nc);
                    for (const auto& oppMove : oppMoves) {
                        if (oppMove.to.row == move.to.row && oppMove.to.col == move.to.col) {
                            // Opponent can capture our piece - BIG penalty
                            int pieceValue = Evaluator::PIECE_VALUES[static_cast<int>(movedPiece.type)];
                            score -= pieceValue * 2; // Double penalty for hanging pieces
                            break;
                        }
                    }
                }
            }

            // Avoid repetition draws if not losing
            if (!positionHistory.empty()) {
                std::string posHash = getPositionHash(nb);
                int rep = 0;
                for (const auto& oldPos : positionHistory) if (oldPos == posHash) rep++;
                if (rep > 0 && score >= -100) score -= 300;
            }

            scored.push_back({move, score});

            if (score > bestScore) {
                bestScore = score;
                depthBestMove = move;
            }

            alpha = std::max(alpha, score);
        }

        if (searchCompleted && !scored.empty()) {
            bestMove = depthBestMove;

            // PV-ish reorder by last depth scores
            std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

            moves.clear();
            moves.reserve(scored.size());
            for (const auto& ms : scored) moves.push_back(ms.first);

            // If we found a forced mate score at root depth, stop early
            if (bestScore >= 450000) break; // Updated for new mate score (500000)
        }
    }

    return bestMove;
}

} // namespace Chess