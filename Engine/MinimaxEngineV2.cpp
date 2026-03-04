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

// Material-first evaluation: material is KING, positional factors are minimal
int EvaluatorV2::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, 
                          bool whiteHasCastled, bool blackHasCastled, int moveCount) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int score = 0;
    
    // MATERIAL IS EVERYTHING - this is the primary score
    int aiMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, oppColor);
    int materialDiff = aiMaterial - oppMaterial;
    
    // Base material score - multiply by 10 to ensure it dominates
    score = materialDiff * 10;
    
    // Extra penalty for material deficit to prevent blunders
    if (materialDiff < -50) { // Down more than half a pawn
        score += materialDiff * 20; // Massive additional penalty
    }
    
    // PIECE COORDINATION: Small bonus for pieces attacking/defending other pieces
    // This encourages active, coordinated play without overriding material
    int aiCoordination = evaluatePieceCoordination(board, aiColor);
    int oppCoordination = evaluatePieceCoordination(board, oppColor);
    score += (aiCoordination - oppCoordination); // Small ~2-5 point bonuses per piece
    
    // Piece development - TINY bonus/penalty (can't override material)
    if (moveCount < 18 && materialDiff >= -100) { // Only if not badly down material
        int aiDevelopment = evaluatePieceDevelopment(board, aiColor, moveCount);
        int oppDevelopment = evaluatePieceDevelopment(board, oppColor, moveCount);
        // Heavily scaled down - development matters but material matters WAY more
        score += (aiDevelopment - oppDevelopment) / 5; // Divide by 5 to make tiny
    }
    
    // Mobility bonus - MINIMAL (piece activity) 
    if (materialDiff >= -50) { // Only consider mobility if not down material
        int aiMobility = evaluateMobility(board, aiColor, castling);
        int oppMobility = evaluateMobility(board, oppColor, castling);
        score += (aiMobility - oppMobility) / 50; // Heavily scaled down (was /15)
    }
    
    // Castling bonus - small encouragement (can't override material)
    if (materialDiff >= -100) {
        const int CASTLING_BONUS = 15; // Reduced from 25
        
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
        
        // Tiny bonus for keeping castling rights
        const int CASTLING_RIGHTS_BONUS = 5; // Reduced from 8
        
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
    
    // Minimal positional bonus: center control (tiny values)
    if (materialDiff >= -50) {
        const int CENTER_BONUS[8][8] = {
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 2, 2, 2, 2, 0, 0},  // Reduced from 5
            {0, 0, 2, 3, 3, 2, 0, 0},  // Reduced from 5,8,8,5
            {0, 0, 2, 3, 3, 2, 0, 0},  // Material must dominate
            {0, 0, 2, 2, 2, 2, 0, 0},
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
    
    // Endgame evaluation - only when material is very low
    if (isEndgame(board)) {
        int endgameBonus = evaluateEndgame(board, aiColor, castling);
        score += endgameBonus;
    }
    
    return score;
}

// Check if we're in endgame (few pieces left)
bool EvaluatorV2::isEndgame(const Board& board) {
    int totalPieces = 0;
    int totalQueensRooks = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (!p.isEmpty() && p.type != PieceType::KING) {
                totalPieces++;
                if (p.type == PieceType::QUEEN || p.type == PieceType::ROOK) {
                    totalQueensRooks++;
                }
            }
        }
    }
    
    // Endgame if: very few pieces OR few heavy pieces
    return totalPieces <= 10 || totalQueensRooks <= 2;
}

Position EvaluatorV2::findKing(const Board& board, Color color) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (!p.isEmpty() && p.type == PieceType::KING && p.color == color) {
                return Position(r, c);
            }
        }
    }
    return Position(-1, -1);
}

// Endgame evaluation for checkmate patterns
int EvaluatorV2::evaluateEndgame(const Board& board, Color aiColor, const CastlingRights& castling) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int aiMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, oppColor);
    
    int score = 0;
    
    // If we have significantly more material, push for checkmate
    if (aiMaterial - oppMaterial > 300) { // Up by more than a minor piece
        Position oppKing = findKing(board, oppColor);
        Position aiKing = findKing(board, aiColor);
        
        if (oppKing.isValid()) {
            // Push enemy king to edge of board
            int oppKingFile = oppKing.col;
            int oppKingRank = oppKing.row;
            
            int distToEdge = std::min({oppKingFile, 7 - oppKingFile, oppKingRank, 7 - oppKingRank});
            score += (7 - distToEdge) * 30; // Reward pushing king to edge
            
            // Bring our king closer for checkmate
            if (aiKing.isValid()) {
                int kingDistance = std::abs(aiKing.row - oppKing.row) + std::abs(aiKing.col - oppKing.col);
                score += (14 - kingDistance) * 20; // Reward king proximity
            }
            
            // Strongly encourage pawn promotion when winning
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(Position(r, c));
                    if (p.isEmpty() || p.color != aiColor || p.type != PieceType::PAWN) continue;
                    
                    // Check if pawn is advanced and relatively safe
                    int advancedRank = (aiColor == Color::WHITE) ? (7 - r) : r;
                    if (advancedRank >= 4) {
                        // Heavily reward advanced pawns in endgame
                        score += advancedRank * 40;
                        
                        // Extra bonus if close to promotion
                        if (advancedRank >= 6) {
                            score += 150;
                        }
                        
                        // Check if pawn is protected
                        bool protected_pawn = false;
                        
                        // Check diagonal protection
                        int pawnDirection = (aiColor == Color::WHITE) ? -1 : 1;
                        for (int dc = -1; dc <= 1; dc += 2) {
                            Position protectorPos(r + pawnDirection, c + dc);
                            if (protectorPos.isValid()) {
                                Piece protector = board.getPiece(protectorPos);
                                if (!protector.isEmpty() && protector.color == aiColor && 
                                    protector.type == PieceType::PAWN) {
                                    protected_pawn = true;
                                    break;
                                }
                            }
                        }
                        
                        // Check if king protects pawn
                        if (aiKing.isValid()) {
                            int distKingToPawn = std::max(std::abs(aiKing.row - r), std::abs(aiKing.col - c));
                            if (distKingToPawn <= 1) {
                                protected_pawn = true;
                            }
                        }
                        
                        if (protected_pawn) {
                            score += 50; // Bonus for protected advanced pawn
                        }
                    }
                }
            }
            
            // Count our queens and rooks to determine mating potential
            int queens = 0, rooks = 0;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(Position(r, c));
                    if (p.isEmpty() || p.color != aiColor) continue;
                    if (p.type == PieceType::QUEEN) queens++;
                    if (p.type == PieceType::ROOK) rooks++;
                }
            }
            
            // If we have sufficient mating material, aggressively pursue checkmate
            if (queens >= 1 || rooks >= 2 || (queens >= 1 && rooks >= 1)) {
                // Extra pressure - coordinate pieces to corner the king
                score += 100;
                
                // Reward limiting opponent king mobility
                std::vector<Move> oppMoves = MoveGenerator::generateMoves(board, oppColor, castling);
                int oppMobility = 0;
                for (const Move& m : oppMoves) {
                    if (m.piece.type == PieceType::KING) {
                        oppMobility++;
                    }
                }
                score += (8 - oppMobility) * 15; // Reward limiting king moves
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
    
    // SMALL penalty for undeveloped pieces (can't override material)
    if (moveCount > 5) {
        development -= undevelopedCount * 10; // Reduced from 30
    }
    
    // SMALL penalty for moving rooks before minor development
    if (hasUndevelopedMinors && rooksDeveloped && moveCount < 15) {
        development -= 15; // Reduced from 40
    }
    
    // Early game development bonuses
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty() || p.color != color) continue;
            
            Position pos(r, c);
            
            // Small reward for developing knights and bishops
            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development += 10; // Reduced from 25
                } else if (moveCount > 8) {
                    development -= 8; // Reduced from 20
                }
            }
            
            // Tiny encouragement for central pawn advancement
            if (p.type == PieceType::PAWN && moveCount < 10) {
                if (color == Color::WHITE) {
                    if ((c == 3 || c == 4) && r >= 3) {
                        development += 6; // Reduced from 15
                    }
                } else {
                    if ((c == 3 || c == 4) && r <= 4) {
                        development += 6; // Reduced from 15
                    }
                }
            }
            
            // Small penalty for early queen development
            if (p.type == PieceType::QUEEN && moveCount < 10) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 10; // Reduced from 25
                }
            }
            
            // Small penalty for moving rooks too early
            if (p.type == PieceType::ROOK && moveCount < 10 && hasUndevelopedMinors) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 12; // Reduced from 35
                }
            }
        }
    }
    
    return development;
}

// Evaluate piece coordination: pieces that attack/defend other pieces
// Small bonuses to favor coordinated, active play without overriding material
int EvaluatorV2::evaluatePieceCoordination(const Board& board, Color color) {
    int coordination = 0;
    Color oppColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    // For each piece of our color, check what it attacks/defends
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(Position(r, c));
            if (piece.isEmpty() || piece.color != color) continue;
            
            Position from(r, c);
            
            // Check all squares this piece can attack/defend
            for (int tr = 0; tr < 8; tr++) {
                for (int tc = 0; tc < 8; tc++) {
                    Position to(tr, tc);
                    if (from == to) continue;
                    
                    Piece target = board.getPiece(to);
                    if (target.isEmpty()) continue;
                    
                    // Check if this piece can attack/defend that square
                    // Simple check: would moving there be legal in terms of piece movement?
                    bool canReach = false;
                    
                    switch (piece.type) {
                        case PieceType::PAWN: {
                            // Pawns attack diagonally
                            int direction = (color == Color::WHITE) ? 1 : -1;
                            if (to.row == from.row + direction && abs(to.col - from.col) == 1) {
                                canReach = true;
                            }
                            break;
                        }
                        case PieceType::KNIGHT: {
                            int dr = abs(to.row - from.row);
                            int dc = abs(to.col - from.col);
                            if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
                                canReach = true;
                            }
                            break;
                        }
                        case PieceType::BISHOP: {
                            if (abs(to.row - from.row) == abs(to.col - from.col)) {
                                // Check diagonal is clear
                                int dr = (to.row > from.row) ? 1 : -1;
                                int dc = (to.col > from.col) ? 1 : -1;
                                bool clear = true;
                                int steps = abs(to.row - from.row) - 1;
                                for (int i = 1; i <= steps; i++) {
                                    if (!board.getPiece(Position(from.row + i*dr, from.col + i*dc)).isEmpty()) {
                                        clear = false;
                                        break;
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::ROOK: {
                            if (to.row == from.row || to.col == from.col) {
                                // Check line is clear
                                bool clear = true;
                                if (to.row == from.row) {
                                    int start = std::min(from.col, to.col) + 1;
                                    int end = std::max(from.col, to.col);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(Position(from.row, i)).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                } else {
                                    int start = std::min(from.row, to.row) + 1;
                                    int end = std::max(from.row, to.row);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(Position(i, from.col)).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::QUEEN: {
                            // Queen = rook + bishop
                            if (to.row == from.row || to.col == from.col) {
                                // Rook-like movement
                                bool clear = true;
                                if (to.row == from.row) {
                                    int start = std::min(from.col, to.col) + 1;
                                    int end = std::max(from.col, to.col);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(Position(from.row, i)).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                } else {
                                    int start = std::min(from.row, to.row) + 1;
                                    int end = std::max(from.row, to.row);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(Position(i, from.col)).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                canReach = clear;
                            } else if (abs(to.row - from.row) == abs(to.col - from.col)) {
                                // Bishop-like movement
                                int dr = (to.row > from.row) ? 1 : -1;
                                int dc = (to.col > from.col) ? 1 : -1;
                                bool clear = true;
                                int steps = abs(to.row - from.row) - 1;
                                for (int i = 1; i <= steps; i++) {
                                    if (!board.getPiece(Position(from.row + i*dr, from.col + i*dc)).isEmpty()) {
                                        clear = false;
                                        break;
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::KING: {
                            if (abs(to.row - from.row) <= 1 && abs(to.col - from.col) <= 1) {
                                canReach = true;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    
                    if (canReach) {
                        if (target.color == color) {
                            // Defending a friendly piece: small bonus
                            // More valuable pieces defended = bigger bonus
                            int defendValue = PIECE_VALUES[static_cast<int>(target.type)] / 200;
                            coordination += std::min(5, defendValue); // Cap at 5 points
                        } else {
                            // Attacking an enemy piece: tiny bonus for activity
                            coordination += 2;
                        }
                    }
                }
            }
        }
    }
    
    return coordination;
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
    // Optimized compact hash: use single character per square
    // 0 = empty, 1-6 = white pieces (PNBRQK), 7-12 = black pieces
    std::string hash;
    hash.reserve(64);
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(Position(r, c));
            if (p.isEmpty()) {
                hash += '0';
            } else {
                char code = '1' + static_cast<char>(p.type);
                if (p.color == Color::BLACK) code += 6;
                hash += code;
            }
        }
    }
    
    return hash;
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
// Use MVV-LVA (Most Valuable Victim - Least Valuable Attacker) + killer moves + castling priority
void MinimaxEngineV2::orderMoves(std::vector<Move>& moves, const Board& board, int ply) {
    std::sort(moves.begin(), moves.end(), [this, ply](const Move& a, const Move& b) {
        // HIGHEST PRIORITY: Castling moves (always prefer castling)
        if (a.isCastling && !b.isCastling) return true;
        if (!a.isCastling && b.isCastling) return false;
        
        // Check killer moves (moves that caused beta cutoffs at this depth)
        if (ply < 64) {
            bool aIsKiller = false;
            bool bIsKiller = false;
            
            // Check if moves match killer moves (compare positions)
            for (int i = 0; i < 2; i++) {
                if (killerMoves_[ply][i].from.isValid()) {
                    if (a.from == killerMoves_[ply][i].from && a.to == killerMoves_[ply][i].to) {
                        aIsKiller = true;
                    }
                    if (b.from == killerMoves_[ply][i].from && b.to == killerMoves_[ply][i].to) {
                        bIsKiller = true;
                    }
                }
            }
            
            if (aIsKiller && !bIsKiller) return true; 
            if (!aIsKiller && bIsKiller) return false;
        }
        
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
        
        return false;
    });
}

// Store killer moves for move ordering
void MinimaxEngineV2::storeKillerMove(const Move& move, int ply) {
    if (ply >= 64) return;
    
    // Don't store captures as killer moves (they're already prioritized)
    if (!move.captured.isEmpty()) return;
    
    // If not already stored as first killer
    if (!(killerMoves_[ply][0].from == move.from && killerMoves_[ply][0].to == move.to)) {
        // Shift first killer to second slot
        killerMoves_[ply][1] = killerMoves_[ply][0];
        // Store new killer in first slot
        killerMoves_[ply][0] = move;
    }
}

int MinimaxEngineV2::minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                              int alpha, int beta, const CastlingRights& castling, int moveCount, 
                              int ply, bool allowNullMove) {
    // Check time limit
    if (isTimeExpired()) {
        timeExpired_ = true;
        return 0;
    }
    
    // Transposition table lookup
    std::string posHash = getPositionHash(board);
    auto it = transpositionTable_.find(posHash);
    if (it != transpositionTable_.end() && it->second.depth >= depth) {
        TranspositionEntry& entry = it->second;
        if (entry.flag == TranspositionEntry::EXACT) {
            return entry.score;
        } else if (entry.flag == TranspositionEntry::LOWER_BOUND) {
            alpha = std::max(alpha, entry.score);
        } else if (entry.flag == TranspositionEntry::UPPER_BOUND) {
            beta = std::min(beta, entry.score);
        }
        if (alpha >= beta) {
            return entry.score;
        }
    }
    
    // Base case: reached depth limit
    if (depth == 0) {
        int eval = EvaluatorV2::evaluate(board, rootColor_, castling, whiteHasCastled_, blackHasCastled_, moveCount);
        // Store in transposition table
        TranspositionEntry entry;
        entry.depth = 0;
        entry.score = eval;
        entry.flag = TranspositionEntry::EXACT;
        transpositionTable_[posHash] = entry;
        return eval;
    }
    
    bool currentlyInCheck = MoveGenerator::isKingInCheck(board, currentColor);
    
    // Null move pruning - skip if in check, at low depth, or not allowed
    if (!maximizing && depth >= 3 && !currentlyInCheck && allowNullMove && moveCount > 5) {
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        int nullScore = minimax(board, nextColor, depth - 3, true, alpha, beta, castling, moveCount + 1, ply + 1, false);
        
        if (nullScore >= beta) {
            return beta; // Beta cutoff
        }
    }
    
    // Generate all legal moves
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    // If no legal moves, it's checkmate or stalemate
    if (moves.empty()) {
        // Check if in check
        bool inCheck = currentlyInCheck;
        
        if (inCheck) {
            // Checkmate - very bad if we're being checkmated, very good if opponent is
            return maximizing ? -100000 : 100000;
        } else {
            // Stalemate - neutral
            return 0;
        }
    }
    
    // Order moves for better alpha-beta pruning
    orderMoves(moves, board, ply);
    
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
            int eval = minimax(newBoard, nextColor, depth - 1, false, alpha, beta, newCastling, moveCount + 1, ply + 1, true);
            
            // Penalty for unnecessary king moves (not in check, not castling, before move 25)
            if (move.piece.type == PieceType::KING && !move.isCastling && !currentlyInCheck && moveCount < 25) {
                // Check if this color has already castled
                bool hasCastled = (currentColor == Color::WHITE) ? whiteHasCastled_ : blackHasCastled_;
                if (!hasCastled) {
                    // Heavy penalty for moving king before castling when not in check
                    eval -= 60;
                } else {
                    // Moderate penalty for moving king unnecessarily even after castling
                    eval -= 25;
                }
            }
            
            // Restore castling status
            whiteHasCastled_ = oldWhiteCastled;
            blackHasCastled_ = oldBlackCastled;
            
            if (eval > maxEval) {
                maxEval = eval;
            }
            
            alpha = std::max(alpha, eval);
            
            // Beta cutoff
            if (beta <= alpha) {
                // Store as killer move
                storeKillerMove(move, ply);
                break;
            }
            
            if (timeExpired_) break;
        }
        
        // Store in transposition table
        TranspositionEntry entry;
        entry.depth = depth;
        entry.score = maxEval;
        if (maxEval <= alpha) {
            entry.flag = TranspositionEntry::UPPER_BOUND;
        } else if (maxEval >= beta) {
            entry.flag = TranspositionEntry::LOWER_BOUND;
        } else {
            entry.flag = TranspositionEntry::EXACT;
        }
        transpositionTable_[posHash] = entry;
        
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
            int eval = minimax(newBoard, nextColor, depth - 1, true, alpha, beta, newCastling, moveCount + 1, ply + 1, true);
            
            // Penalty for unnecessary king moves (not in check, not castling, before move 25)
            if (move.piece.type == PieceType::KING && !move.isCastling && !currentlyInCheck && moveCount < 25) {
                // Check if this color has already castled
                bool hasCastled = (currentColor == Color::WHITE) ? whiteHasCastled_ : blackHasCastled_;
                if (!hasCastled) {
                    // Heavy penalty for moving king before castling when not in check
                    eval += 60; // Add because we're minimizing
                } else {
                    // Moderate penalty for moving king unnecessarily even after castling
                    eval += 25;
                }
            }
            
            // Restore castling status
            whiteHasCastled_ = oldWhiteCastled;
            blackHasCastled_ = oldBlackCastled;
            
            if (eval < minEval) {
                minEval = eval;
            }
            
            beta = std::min(beta, eval);
            
            // Alpha cutoff
            if (beta <= alpha) {
                // Store as killer move
                storeKillerMove(move, ply);
                break;
            }
            
            if (timeExpired_) break;
        }
        
        // Store in transposition table
        TranspositionEntry entry;
        entry.depth = depth;
        entry.score = minEval;
        if (minEval <= alpha) {
            entry.flag = TranspositionEntry::LOWER_BOUND;
        } else if (minEval >= beta) {
            entry.flag = TranspositionEntry::UPPER_BOUND;
        } else {
            entry.flag = TranspositionEntry::EXACT;
        }
        transpositionTable_[posHash] = entry;
        
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
    
    // Clear transposition table for new search
    transpositionTable_.clear();
    
    // Initialize killer moves  
    for (int i = 0; i < 64; i++) {
        killerMoves_[i][0] = Move();
        killerMoves_[i][1] = Move();
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    
    if (moves.empty()) {
        return Move(); // No legal moves
    }
    
    // Check if we have a castling move available
    bool castlingAvailable = false;
    for (const Move& move : moves) {
        if (move.isCastling) {
            castlingAvailable = true;
            break;
        }
    }
    
    // Order moves at root for better search
    orderMoves(moves, board, 0);
    
    Move bestMove = moves[0];
    int bestScore = std::numeric_limits<int>::min();
    int depthReached = 0; // Track the depth we actually completed
    
    // Check if currently in check
    bool inCheck = MoveGenerator::isKingInCheck(board, color);
    
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
            int score = minimax(newBoard, nextColor, currentDepth - 1, false, alpha, beta, newCastling, 1, 1, true);
            
            // HUGE bonus for castling moves - always prefer castling
            if (move.isCastling) {
                score += 200; // Massive bonus for castling
            }
            
            // Penalty for unnecessary king moves at root (not in check, not castling)
            // Only apply in early/mid game (approximated by position history size)
            if (move.piece.type == PieceType::KING && !move.isCastling && !inCheck && 
                positionHistory.size() < 25) {
                // If castling is available, make king moves extremely bad
                if (castlingAvailable && !whiteHasCastled && !blackHasCastled) {
                    score -= 300; // Extremely heavy penalty if castling is an option
                } else if (!whiteHasCastled && color == Color::WHITE) {
                    // Heavy penalty for moving king before castling when not in check
                    score -= 60;
                } else if (!blackHasCastled && color == Color::BLACK) {
                    score -= 60;
                } else {
                    // Moderate penalty for moving king unnecessarily even after castling
                    score -= 25;
                }
            }
            
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
            depthReached = currentDepth; // Update depth reached
        }
        
        // Early exit if mate found
        if (bestScore > 450000) break;
    }
    
    // Set the search depth in the move
    bestMove.searchDepth = depthReached;
    
    return bestMove;
}

} // namespace Chess
