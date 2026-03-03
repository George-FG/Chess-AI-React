#include "Engine_v2.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Chess {

// Enhanced piece values
const int EvaluatorV2::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    20000, // KING (high value to avoid trades)
    0     // NONE
};

const int EvaluatorV2::ENDGAME_PIECE_VALUES[7] = {
    120,  // PAWN (more valuable in endgame)
    300,  // KNIGHT (slightly less valuable)
    330,  // BISHOP
    550,  // ROOK (more valuable)
    950,  // QUEEN
    20000, // KING
    0     // NONE
};

// Passed pawn bonus by rank (white perspective)
const int EvaluatorV2::PASSED_PAWN_BONUS[8] = {
    0, 10, 20, 40, 70, 120, 200, 0
};

// Advanced piece-square tables (from white's perspective)
// Opening pawn table: encourage center control and development
const int EvaluatorV2::PAWN_TABLE_OPENING[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    { 50, 50, 50, 50, 50, 50, 50, 50},
    { 10, 10, 20, 30, 30, 20, 10, 10},
    {  5,  5, 15, 35, 35, 15,  5,  5},
    {  0,  0, 10, 30, 30, 10,  0,  0},
    {  5, -5,(-10), 5,  5,(-10), -5,  5},
    {  5, 10, 10,(-25),(-25), 10, 10,  5},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

// Endgame pawn table: push for promotion
const int EvaluatorV2::PAWN_TABLE_ENDGAME[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {180,180,180,180,180,180,180,180},
    { 90, 90, 90, 90, 90, 90, 90, 90},
    { 50, 50, 50, 50, 50, 50, 50, 50},
    { 30, 30, 30, 30, 30, 30, 30, 30},
    { 20, 20, 20, 20, 20, 20, 20, 20},
    { 10, 10, 10, 10, 10, 10, 10, 10},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

// Knights: prefer center and avoid edges
const int EvaluatorV2::KNIGHT_TABLE_OPENING[8][8] = {
    {(-50),(-40),(-30),(-30),(-30),(-30),(-40),(-50)},
    {(-40),(-20),  0,   5,   5,   0,(-20),(-40)},
    {(-30),  5,  10,  15,  15,  10,   5,(-30)},
    {(-30),  10,  15,  25,  25,  15,  10,(-30)},
    {(-30),  10,  15,  25,  25,  15,  10,(-30)},
    {(-30),  5,  15,  20,  20,  15,   5,(-30)},
    {(-40),(-20),  0,   0,   0,   0,(-20),(-40)},
    {(-50),(-40),(-30),(-30),(-30),(-30),(-40),(-50)}
};

const int EvaluatorV2::KNIGHT_TABLE_ENDGAME[8][8] = {
    {(-50),(-40),(-30),(-30),(-30),(-30),(-40),(-50)},
    {(-40),(-20),  0,   0,   0,   0,(-20),(-40)},
    {(-30),  0,  10,  15,  15,  10,   0,(-30)},
    {(-30),   5,  15,  20,  20,  15,   5,(-30)},
    {(-30),   5,  15,  20,  20,  15,   5,(-30)},
    {(-30),  0,  10,  15,  15,  10,   0,(-30)},
    {(-40),(-20),  0,   0,   0,   0,(-20),(-40)},
    {(-50),(-40),(-30),(-30),(-30),(-30),(-40),(-50)}
};

// Bishops: prefer long diagonals
const int EvaluatorV2::BISHOP_TABLE[8][8] = {
    {(-20),(-10),(-10),(-10),(-10),(-10),(-10),(-20)},
    {(-10),   5,   0,   0,   0,   0,   5,(-10)},
    {(-10),  10,  10,  10,  10,  10,  10,(-10)},
    {(-10),   0,  10,  15,  15,  10,   0,(-10)},
    {(-10),   5,   5,  15,  15,   5,   5,(-10)},
    {(-10),   0,   5,  10,  10,   5,   0,(-10)},
    {(-10),   0,   0,   0,   0,   0,   0,(-10)},
    {(-20),(-10),(-10),(-10),(-10),(-10),(-10),(-20)}
};

// Rooks: prefer open files and 7th rank
const int EvaluatorV2::ROOK_TABLE_OPENING[8][8] = {
    {  0,  0,  0,  5,  5,  0,  0,  0},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0, -5},
    {  5, 10, 10, 10, 10, 10, 10,  5},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

const int EvaluatorV2::ROOK_TABLE_ENDGAME[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    { 10, 10, 10, 10, 10, 10, 10, 10},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

// Queen: slight preference for center
const int EvaluatorV2::QUEEN_TABLE[8][8] = {
    {(-20),(-10),(-10), (-5), (-5),(-10),(-10),(-20)},
    {(-10),   0,   0,   0,   0,   5,   0,(-10)},
    {(-10),   0,   5,   5,   5,   5,   5,(-10)},
    {  (-5),   0,   5,   5,   5,   5,   0,   0},
    {  (-5),   0,   5,   5,   5,   5,   0,  (-5)},
    {(-10),   0,   5,   5,   5,   5,   0,(-10)},
    {(-10),   0,   0,   0,   0,   0,   0,(-10)},
    {(-20),(-10),(-10), (-5), (-5),(-10),(-10),(-20)}
};

// King opening: stay safe, castle
const int EvaluatorV2::KING_OPENING_TABLE[8][8] = {
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-20),(-30),(-30),(-40),(-40),(-30),(-30),(-20)},
    {(-10),(-20),(-20),(-20),(-20),(-20),(-20),(-10)},
    { 20,  20,   0,   0,   0,   0,  20,  20},
    { 20,  35,  10,   0,   0,  10,  35,  20}
};

const int EvaluatorV2::KING_MIDGAME_TABLE[8][8] = {
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-30),(-40),(-40),(-50),(-50),(-40),(-40),(-30)},
    {(-20),(-30),(-30),(-40),(-40),(-30),(-30),(-20)},
    {(-10),(-20),(-20),(-20),(-20),(-20),(-20),(-10)},
    { 20,  20,   0,   0,   0,   0,  20,  20},
    { 20,  30,  10,   0,   0,  10,  30,  20}
};

// King endgame: be active and centralized
const int EvaluatorV2::KING_ENDGAME_TABLE[8][8] = {
    {(-50),(-30),(-30),(-30),(-30),(-30),(-30),(-50)},
    {(-30),(-30),  0,   0,   0,   0,(-30),(-30)},
    {(-30),(-10), 20,  30,  30,  20,(-10),(-30)},
    {(-30),(-10), 30,  40,  40,  30,(-10),(-30)},
    {(-30),(-10), 30,  40,  40,  30,(-10),(-30)},
    {(-30),(-10), 20,  30,  30,  20,(-10),(-30)},
    {(-30),(-20),(-10),  0,   0,(-10),(-20),(-30)},
    {(-50),(-40),(-30),(-20),(-20),(-30),(-40),(-50)}
};

// ============================================================================
//  PHASE DETECTION
// ============================================================================

GamePhase EvaluatorV2::detectPhase(const Board& board) {
    int totalMaterial = getMaterialCount(board);
    int pieceCount = getPieceCount(board);
    
    // Count queens and heavy pieces
    int queens = 0, rooks = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPiece(Position(row, col));
            if (p.type == PieceType::QUEEN) queens++;
            if (p.type == PieceType::ROOK) rooks++;
        }
    }
    
    // Opening: Most pieces, both queens
    if (pieceCount > 28 && queens >= 2) {
        return GamePhase::OPENING;
    }
    // Endgame: Few pieces or low material
    else if (pieceCount <= 10 || totalMaterial < 2800 || queens == 0) {
        return GamePhase::ENDGAME;
    }
    // Midgame
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

// ============================================================================
//  MAIN EVALUATION
// ============================================================================

int EvaluatorV2::evaluate(const Board& board, Color aiColor, GamePhase phase) {
    int score = 0;
    
    // Material evaluation
    score += evaluateMaterial(board, aiColor);
    
    // Positional evaluation  
    score += evaluatePosition(board, aiColor, phase);
    
    // Tactical evaluation
    score += evaluateTactics(board, aiColor);
    
    // Pawn structure
    score += evaluatePawnStructure(board, aiColor);
    
    // Mobility
    score += evaluateMobility(board, aiColor);
    
    // King safety
    score += evaluateKingSafety(board, aiColor, phase);
    
    // Piece coordination
    score += evaluatePieceCoordination(board, aiColor);
    
    // Threats
    score += evaluateThreats(board, aiColor);
    
    return score;
}

// ============================================================================
//  MATERIAL EVALUATION
// ============================================================================

int EvaluatorV2::evaluateMaterial(const Board& board, Color aiColor) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int myMaterial = 0, oppMaterial = 0;
    
    GamePhase phase = detectPhase(board);
    const int* values = (phase == GamePhase::ENDGAME) ? ENDGAME_PIECE_VALUES : PIECE_VALUES;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPiece(Position(row, col));
            if (p.isEmpty() || p.type == PieceType::KING) continue;
            
            int value = values[static_cast<int>(p.type)];
            
            if (p.color == aiColor) {
                myMaterial += value;
            } else {
                oppMaterial += value;
            }
        }
    }
    
    // Bishop pair bonus
    int myBishops = 0, oppBishops = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPiece(Position(row, col));
            if (p.type == PieceType::BISHOP) {
                if (p.color == aiColor) myBishops++;
                else oppBishops++;
            }
        }
    }
    
    if (myBishops >= 2) myMaterial += BISHOP_PAIR_BONUS;
    if (oppBishops >= 2) oppMaterial += BISHOP_PAIR_BONUS;
    
    return myMaterial - oppMaterial;
}

// ============================================================================
//  POSITIONAL EVALUATION
// ============================================================================

int EvaluatorV2::evaluatePosition(const Board& board, Color aiColor, GamePhase phase) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int myScore = 0, oppScore = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position pos(row, col);
            Piece p = board.getPiece(pos);
            if (p.isEmpty()) continue;
            
            int psqt = 0;
            int tableRow = (p.color == Color::WHITE) ? row : (7 - row);
            
            switch (p.type) {
                case PieceType::PAWN:
                    psqt = (phase == GamePhase::ENDGAME) ? 
                        PAWN_TABLE_ENDGAME[tableRow][col] : 
                        PAWN_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::KNIGHT:
                    psqt = (phase == GamePhase::ENDGAME) ? 
                        KNIGHT_TABLE_ENDGAME[tableRow][col] : 
                        KNIGHT_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::BISHOP:
                    psqt = BISHOP_TABLE[tableRow][col];
                    break;
                case PieceType::ROOK:
                    psqt = (phase == GamePhase::ENDGAME) ?
                        ROOK_TABLE_ENDGAME[tableRow][col] :
                        ROOK_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::QUEEN:
                    psqt = QUEEN_TABLE[tableRow][col];
                    break;
                case PieceType::KING:
                    if (phase == GamePhase::OPENING) {
                        psqt = KING_OPENING_TABLE[tableRow][col];
                    } else if (phase == GamePhase::ENDGAME) {
                        psqt = KING_ENDGAME_TABLE[tableRow][col];
                    } else {
                        psqt = KING_MIDGAME_TABLE[tableRow][col];
                    }
                    break;
                default:
                    break;
            }
            
            if (p.color == aiColor) {
                myScore += psqt;
            } else {
                oppScore += psqt;
            }
        }
    }
    
    return myScore - oppScore;
}

// ============================================================================
//  HELPER FUNCTIONS
// ============================================================================

bool EvaluatorV2::isSquareAttacked(const Board& board, Position pos, Color attackingColor) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece attacker = board.getPiece(Position(row, col));
            if (attacker.isEmpty() || attacker.color != attackingColor) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), attacker);
            
            for (const Position& target : moves) {
                if (target == pos) return true;
            }
        }
    }
    return false;
}

int EvaluatorV2::countAttackers(const Board& board, Position pos, Color attackingColor) {
    int count = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece attacker = board.getPiece(Position(row, col));
            if (attacker.isEmpty() || attacker.color != attackingColor) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), attacker);
            
            for (const Position& target : moves) {
                if (target == pos) {
                    count++;
                    break;
                }
            }
        }
    }
    return count;
}

int EvaluatorV2::countDefenders(const Board& board, Position pos, Color defendingColor) {
    return countAttackers(board, pos, defendingColor);
}

int EvaluatorV2::getManhattanDistance(Position a, Position b) {
    return std::abs(a.row - b.row) + std::abs(a.col - b.col);
}

int EvaluatorV2::getChebyshevDistance(Position a, Position b) {
    return std::max(std::abs(a.row - b.row), std::abs(a.col - b.col));
}

bool EvaluatorV2::isOpenFile(const Board& board, int col) {
    for (int row = 0; row < 8; row++) {
        Piece p = board.getPiece(Position(row, col));
        if (p.type == PieceType::PAWN) return false;
    }
    return true;
}

bool EvaluatorV2::isSemiOpenFile(const Board& board, int col, Color color) {
    bool hasPawn = false;
    for (int row = 0; row < 8; row++) {
        Piece p = board.getPiece(Position(row, col));
        if (p.type == PieceType::PAWN) {
            if (p.color == color) return false;
            hasPawn = true;
        }
    }
    return hasPawn;
}

// Continued in next part...
