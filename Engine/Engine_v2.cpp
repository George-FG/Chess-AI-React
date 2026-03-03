#include "Engine_v2.h"
#include <algorithm>
#include <cmath>
#include <cassert>

namespace Chess {

// ============================================================================
// PIECE VALUES
// ============================================================================

const int EvaluatorV2::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    20000,// KING (high value for safety evaluation)
    0     // NONE
};

const int EvaluatorV2::ENDGAME_PIECE_VALUES[7] = {
    120,  // PAWN (more valuable in endgame)
    300,  // KNIGHT (less valuable in endgame)
    320,  // BISHOP
    550,  // ROOK (more valuable in endgame)
    950,  // QUEEN
    20000,// KING
    0     // NONE
};

const int EvaluatorV2::PASSED_PAWN_BONUS[8] = {
    0, 5, 10, 20, 35, 60, 100, 150
};

// ============================================================================
// PIECE-SQUARE TABLES (from white's perspective)
// ============================================================================

const int EvaluatorV2::PAWN_TABLE_OPENING[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    { 50, 50, 50, 50, 50, 50, 50, 50},
    { 10, 10, 20, 30, 30, 20, 10, 10},
    {  5,  5, 10, 27, 27, 10,  5,  5},
    {  0,  0,  0, 25, 25,  0,  0,  0},
    {  5, -5,-10,  0,  0,-10, -5,  5},
    {  5, 10, 10,-25,-25, 10, 10,  5},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

const int EvaluatorV2::PAWN_TABLE_ENDGAME[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {100,100,100,100,100,100,100,100},
    { 80, 80, 80, 80, 80, 80, 80, 80},
    { 50, 50, 50, 50, 50, 50, 50, 50},
    { 30, 30, 30, 30, 30, 30, 30, 30},
    { 20, 20, 20, 20, 20, 20, 20, 20},
    { 10, 10, 10, 10, 10, 10, 10, 10},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

const int EvaluatorV2::KNIGHT_TABLE_OPENING[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int EvaluatorV2::KNIGHT_TABLE_ENDGAME[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20, -5, -5, -5, -5,-20,-40},
    {-30, -5,  5, 10, 10,  5, -5,-30},
    {-30, -5, 10, 15, 15, 10, -5,-30},
    {-30, -5, 10, 15, 15, 10, -5,-30},
    {-30, -5,  5, 10, 10,  5, -5,-30},
    {-40,-20, -5, -5, -5, -5,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int EvaluatorV2::BISHOP_TABLE[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  0, 10, 15, 15, 10,  0,-10},
    {-10,  5, 10, 15, 15, 10,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

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
    { 10, 10, 10, 10, 10, 10, 10, 10},
    { 10, 10, 10, 10, 10, 10, 10, 10},
    {  5,  5,  5,  5,  5,  5,  5,  5},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0}
};

const int EvaluatorV2::QUEEN_TABLE[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {  0,  0,  5,  5,  5,  5,  0, -5},
    { -5,  0,  5,  5,  5,  5,  0, -5},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

const int EvaluatorV2::KING_OPENING_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

const int EvaluatorV2::KING_MIDGAME_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 35, 10,  0,  0, 10, 35, 20}
};

const int EvaluatorV2::KING_ENDGAME_TABLE[8][8] = {
    {-50,-30,-30,-30,-30,-30,-30,-50},
    {-30,-20,  0,  0,  0,  0,-20,-30},
    {-30,  0, 20, 30, 30, 20,  0,-30},
    {-30,  0, 30, 40, 40, 30,  0,-30},
    {-30,  0, 30, 40, 40, 30,  0,-30},
    {-30,  0, 20, 30, 30, 20,  0,-30},
    {-30,-20,  0,  0,  0,  0,-20,-30},
    {-50,-30,-30,-30,-30,-30,-30,-50}
};

// ============================================================================
// EVALUATORV2: HELPER FUNCTIONS
// ============================================================================

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

int EvaluatorV2::getManhattanDistance(Position a, Position b) {
    return std::abs(a.row - b.row) + std::abs(a.col - b.col);
}

int EvaluatorV2::getChebyshevDistance(Position a, Position b) {
    return std::max(std::abs(a.row - b.row), std::abs(a.col - b.col));
}

bool EvaluatorV2::isSquareAttacked(const Board& board, Position pos, Color attackingColor) {
    // Check all opponent pieces to see if they can attack this square
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.color != attackingColor) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), piece);
            
            for (const Position& move : moves) {
                if (move.row == pos.row && move.col == pos.col) {
                    return true;
                }
            }
        }
    }
    return false;
}

int EvaluatorV2::countAttackers(const Board& board, Position pos, Color attackingColor) {
    int count = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.color != attackingColor) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), piece);
            
            for (const Position& move : moves) {
                if (move.row == pos.row && move.col == pos.col) {
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

bool EvaluatorV2::isPinned(const Board& board, Position pos, Color pieceColor) {
    Piece piece = board.getPiece(pos);
    if (piece.isEmpty() || piece.color != pieceColor) return false;
    
    // Find king position
    Position kingPos(-1, -1);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPiece(Position(row, col));
            if (p.type == PieceType::KING && p.color == pieceColor) {
                kingPos = Position(row, col);
                break;
            }
        }
        if (kingPos.isValid()) break;
    }
    
    if (!kingPos.isValid()) return false;
    
    // Check if piece is on a line with the king
    int rowDiff = kingPos.row - pos.row;
    int colDiff = kingPos.col - pos.col;
    
    if (rowDiff != 0 && colDiff != 0 && std::abs(rowDiff) != std::abs(colDiff)) {
        return false; // Not on same rank, file, or diagonal
    }
    
    // Simulate removing the piece and check if king is attacked
    Board testBoard = board.clone();
    testBoard.setPiece(pos, Piece());
    
    Color enemyColor = (pieceColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    return isSquareAttacked(testBoard, kingPos, enemyColor);
}

bool EvaluatorV2::isPassedPawn(const Board& board, Position pos, Color pawnColor) {
    Piece piece = board.getPiece(pos);
    if (piece.type != PieceType::PAWN || piece.color != pawnColor) return false;
    
    int direction = (pawnColor == Color::WHITE) ? 1 : -1;
    int endRow = (pawnColor == Color::WHITE) ? 8 : -1;
    
    // Check squares ahead and on adjacent files
    for (int row = pos.row + direction; row != endRow; row += direction) {
        // Check same file and adjacent files
        for (int colOffset = -1; colOffset <= 1; colOffset++) {
            int col = pos.col + colOffset;
            if (col < 0 || col >= 8) continue;
            
            Piece p = board.getPiece(Position(row, col));
            if (p.type == PieceType::PAWN && p.color != pawnColor) {
                return false;
            }
        }
    }
    
    return true;
}

bool EvaluatorV2::isOutpost(const Board& board, Position pos, Color color) {
    Piece piece = board.getPiece(pos);
    if (piece.isEmpty() || piece.color != color) return false;
    if (piece.type != PieceType::KNIGHT && piece.type != PieceType::BISHOP) return false;
    
    // Check if protected by own pawn
    int direction = (color == Color::WHITE) ? -1 : 1;
    bool protectedByPawn = false;
    
    for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
        int col = pos.col + colOffset;
        int row = pos.row + direction;
        if (row >= 0 && row < 8 && col >= 0 && col < 8) {
            Piece p = board.getPiece(Position(row, col));
            if (p.type == PieceType::PAWN && p.color == color) {
                protectedByPawn = true;
                break;
            }
        }
    }
    
    if (!protectedByPawn) return false;
    
    // Check if enemy pawns can attack
    Color enemyColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int enemyDir = (enemyColor == Color::WHITE) ? -1 : 1;
    
    for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
        int col = pos.col + colOffset;
        for (int row = pos.row + enemyDir; row >= 0 && row < 8; row += enemyDir) {
            if (col >= 0 && col < 8) {
                Piece p = board.getPiece(Position(row, col));
                if (p.type == PieceType::PAWN && p.color == enemyColor) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool EvaluatorV2::isOpenFile(const Board& board, int col) {
    for (int row = 0; row < 8; row++) {
        Piece piece = board.getPiece(Position(row, col));
        if (piece.type == PieceType::PAWN) {
            return false;
        }
    }
    return true;
}

bool EvaluatorV2::isSemiOpenFile(const Board& board, int col, Color color) {
    bool hasOwnPawn = false;
    bool hasEnemyPawn = false;
    
    for (int row = 0; row < 8; row++) {
        Piece piece = board.getPiece(Position(row, col));
        if (piece.type == PieceType::PAWN) {
            if (piece.color == color) {
                hasOwnPawn = true;
            } else {
                hasEnemyPawn = true;
            }
        }
    }
    
    return !hasOwnPawn && hasEnemyPawn;
}

// ============================================================================
// EVALUATORV2: PHASE DETECTION
// ============================================================================

GamePhase EvaluatorV2::detectPhase(const Board& board) {
    int totalMaterial = getMaterialCount(board);
    int pieceCount = getPieceCount(board);
    
    // Opening: more than 6000 material (most pieces on board)
    if (totalMaterial > 6000 && pieceCount > 28) {
        return GamePhase::OPENING;
    }
    // Endgame: less than 2800 material or few pieces
    else if (totalMaterial < 2800 || pieceCount < 12) {
        return GamePhase::ENDGAME;
    }
    // Midgame: everything in between
    else {
        return GamePhase::MIDGAME;
    }
}

// ============================================================================
// EVALUATORV2: MAIN EVALUATION
// ============================================================================

int EvaluatorV2::evaluate(const Board& board, Color aiColor, GamePhase phase) {
    int score = 0;
    
    score += evaluateMaterial(board, aiColor);
    score += evaluatePosition(board, aiColor, phase);
    score += evaluatePawnStructure(board, aiColor);
    score += evaluateMobility(board, aiColor);
    score += evaluateKingSafety(board, aiColor, phase);
    score += evaluatePieceCoordination(board, aiColor);
    score += evaluateTactics(board, aiColor);
    score += evaluateThreats(board, aiColor);
    
    return score;
}

int EvaluatorV2::evaluateMaterial(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myMaterial = 0;
    int opponentMaterial = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.type == PieceType::KING) continue;
            
            int value = PIECE_VALUES[static_cast<int>(piece.type)];
            
            if (piece.color == aiColor) {
                myMaterial += value;
            } else {
                opponentMaterial += value;
            }
        }
    }
    
    return myMaterial - opponentMaterial;
}

int EvaluatorV2::evaluatePosition(const Board& board, Color aiColor, GamePhase phase) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int myScore = 0;
    int opponentScore = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty()) continue;
            
            int psqtBonus = 0;
            int tableRow = (piece.color == Color::WHITE) ? row : (7 - row);
            
            switch (piece.type) {
                case PieceType::PAWN:
                    psqtBonus = (phase == GamePhase::ENDGAME) ? 
                        PAWN_TABLE_ENDGAME[tableRow][col] : PAWN_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::KNIGHT:
                    psqtBonus = (phase == GamePhase::ENDGAME) ?
                        KNIGHT_TABLE_ENDGAME[tableRow][col] : KNIGHT_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::BISHOP:
                    psqtBonus = BISHOP_TABLE[tableRow][col];
                    break;
                case PieceType::ROOK:
                    psqtBonus = (phase == GamePhase::ENDGAME) ?
                        ROOK_TABLE_ENDGAME[tableRow][col] : ROOK_TABLE_OPENING[tableRow][col];
                    break;
                case PieceType::QUEEN:
                    psqtBonus = QUEEN_TABLE[tableRow][col];
                    break;
                case PieceType::KING:
                    if (phase == GamePhase::ENDGAME) {
                        psqtBonus = KING_ENDGAME_TABLE[tableRow][col];
                    } else if (phase == GamePhase::OPENING) {
                        psqtBonus = KING_OPENING_TABLE[tableRow][col];
                    } else {
                        psqtBonus = KING_MIDGAME_TABLE[tableRow][col];
                    }
                    break;
                default:
                    break;
            }
            
            if (piece.color == aiColor) {
                myScore += psqtBonus;
            } else {
                opponentScore += psqtBonus;
            }
        }
    }
    
    return myScore - opponentScore;
}

// ============================================================================
// EVALUATORV2: PAWN STRUCTURE
// ============================================================================

int EvaluatorV2::evaluatePawnStructure(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    myScore -= evaluateDoubledPawns(board, aiColor);
    myScore -= evaluateIsolatedPawns(board, aiColor);
    myScore -= evaluateBackwardPawns(board, aiColor);
    myScore += evaluatePassedPawns(board, aiColor);
    myScore += evaluatePawnChains(board, aiColor);
    
    opponentScore -= evaluateDoubledPawns(board, opponentColor);
    opponentScore -= evaluateIsolatedPawns(board, opponentColor);
    opponentScore -= evaluateBackwardPawns(board, opponentColor);
    opponentScore += evaluatePassedPawns(board, opponentColor);
    opponentScore += evaluatePawnChains(board, opponentColor);
    
    return myScore - opponentScore;
}

int EvaluatorV2::evaluateDoubledPawns(const Board& board, Color color) {
    int penalty = 0;
    
    for (int col = 0; col < 8; col++) {
        int pawnCount = 0;
        for (int row = 0; row < 8; row++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::PAWN && piece.color == color) {
                pawnCount++;
            }
        }
        if (pawnCount > 1) {
            penalty += DOUBLED_PAWN_PENALTY * (pawnCount - 1);
        }
    }
    
    return penalty;
}

int EvaluatorV2::evaluateIsolatedPawns(const Board& board, Color color) {
    int penalty = 0;
    
    for (int col = 0; col < 8; col++) {
        bool hasPawn = false;
        bool hasAdjacentPawn = false;
        
        for (int row = 0; row < 8; row++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::PAWN && piece.color == color) {
                hasPawn = true;
                break;
            }
        }
        
        if (hasPawn) {
            // Check adjacent files
            for (int adjacentCol = col - 1; adjacentCol <= col + 1; adjacentCol += 2) {
                if (adjacentCol < 0 || adjacentCol >= 8) continue;
                for (int row = 0; row < 8; row++) {
                    Piece piece = board.getPiece(Position(row, adjacentCol));
                    if (piece.type == PieceType::PAWN && piece.color == color) {
                        hasAdjacentPawn = true;
                        break;
                    }
                }
                if (hasAdjacentPawn) break;
            }
            
            if (!hasAdjacentPawn) {
                penalty += ISOLATED_PAWN_PENALTY;
            }
        }
    }
    
    return penalty;
}

int EvaluatorV2::evaluatePassedPawns(const Board& board, Color color) {
    int bonus = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position pos(row, col);
            if (isPassedPawn(board, pos, color)) {
                int rank = (color == Color::WHITE) ? row : (7 - row);
                bonus += PASSED_PAWN_BONUS[rank];
            }
        }
    }
    
    return bonus;
}

int EvaluatorV2::evaluateBackwardPawns(const Board& board, Color color) {
    int penalty = 0;
    int direction = (color == Color::WHITE) ? 1 : -1;
    
    for (int row = 1; row < 7; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type != PieceType::PAWN || piece.color != color) continue;
            
            bool isBackward = true;
            
            // Check if adjacent pawns are ahead
            for (int adjacentCol = col - 1; adjacentCol <= col + 1; adjacentCol += 2) {
                if (adjacentCol < 0 || adjacentCol >= 8) continue;
                
                bool hasAdjacentPawn = false;
                for (int checkRow = row + direction; checkRow >= 0 && checkRow < 8; checkRow += direction) {
                    Piece p = board.getPiece(Position(checkRow, adjacentCol));
                    if (p.type == PieceType::PAWN && p.color == color) {
                        hasAdjacentPawn = true;
                        break;
                    }
                }
                
                if (!hasAdjacentPawn) {
                    isBackward = false;
                    break;
                }
            }
            
            if (isBackward) {
                penalty += BACKWARD_PAWN_PENALTY;
            }
        }
    }
    
    return penalty;
}

int EvaluatorV2::evaluatePawnChains(const Board& board, Color color) {
    int bonus = 0;
    int direction = (color == Color::WHITE) ? -1 : 1;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type != PieceType::PAWN || piece.color != color) continue;
            
            // Check if protected by another pawn
            for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
                int adjacentCol = col + colOffset;
                int adjacentRow = row + direction;
                
                if (adjacentCol >= 0 && adjacentCol < 8 && adjacentRow >= 0 && adjacentRow < 8) {
                    Piece p = board.getPiece(Position(adjacentRow, adjacentCol));
                    if (p.type == PieceType::PAWN && p.color == color) {
                        bonus += 10;
                        break;
                    }
                }
            }
        }
    }
    
    return bonus;
}

// ============================================================================
// EVALUATORV2: PIECE-SPECIFIC EVALUATION
// ============================================================================

int EvaluatorV2::evaluateRooks(const Board& board, Color color, GamePhase phase) {
    int bonus = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type != PieceType::ROOK || piece.color != color) continue;
            
            if (isOpenFile(board, col)) {
                bonus += ROOK_OPEN_FILE_BONUS;
            } else if (isSemiOpenFile(board, col, color)) {
                bonus += ROOK_SEMI_OPEN_FILE_BONUS;
            }
            
            // Bonus for rooks on 7th rank
            int targetRank = (color == Color::WHITE) ? 6 : 1;
            if (row == targetRank) {
                bonus += 20;
            }
        }
    }
    
    return bonus;
}

int EvaluatorV2::evaluateBishops(const Board& board, Color color) {
    int bonus = 0;
    int bishopCount = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::BISHOP && piece.color == color) {
                bishopCount++;
            }
        }
    }
    
    // Bishop pair bonus
    if (bishopCount >= 2) {
        bonus += BISHOP_PAIR_BONUS;
    }
    
    return bonus;
}

int EvaluatorV2::evaluateKnights(const Board& board, Color color) {
    int bonus = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type != PieceType::KNIGHT || piece.color != color) continue;
            
            if (isOutpost(board, Position(row, col), color)) {
                bonus += KNIGHT_OUTPOST_BONUS;
            }
        }
    }
    
    return bonus;
}

int EvaluatorV2::evaluateQueens(const Board& board, Color color) {
    // Basic queen evaluation - already handled by piece-square tables
    return 0;
}

// ============================================================================
// EVALUATORV2: MOBILITY
// ============================================================================

int EvaluatorV2::evaluateMobility(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myMobility = 0;
    int opponentMobility = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty()) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), piece);
            
            int moveCount = moves.size();
            
            if (piece.color == aiColor) {
                myMobility += moveCount;
            } else {
                opponentMobility += moveCount;
            }
        }
    }
    
    return (myMobility - opponentMobility) * 2;
}

// ============================================================================
// EVALUATORV2: KING SAFETY
// ============================================================================

int EvaluatorV2::evaluateKingSafety(const Board& board, Color aiColor, GamePhase phase) {
    if (phase == GamePhase::ENDGAME) {
        return evaluateKingActivity(board, aiColor) - 
               evaluateKingActivity(board, (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE);
    }
    
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    // Find kings
    Position myKingPos(-1, -1);
    Position oppKingPos(-1, -1);
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING) {
                if (piece.color == aiColor) {
                    myKingPos = Position(row, col);
                } else {
                    oppKingPos = Position(row, col);
                }
            }
        }
    }
    
    // Evaluate pawn shield
    if (myKingPos.isValid()) {
        int direction = (aiColor == Color::WHITE) ? 1 : -1;
        for (int colOffset = -1; colOffset <= 1; colOffset++) {
            Position shieldPos(myKingPos.row + direction, myKingPos.col + colOffset);
            if (shieldPos.isValid()) {
                Piece piece = board.getPiece(shieldPos);
                if (piece.type == PieceType::PAWN && piece.color == aiColor) {
                    myScore += 20;
                }
            }
        }
    }
    
    if (oppKingPos.isValid()) {
        int direction = (opponentColor == Color::WHITE) ? 1 : -1;
        for (int colOffset = -1; colOffset <= 1; colOffset++) {
            Position shieldPos(oppKingPos.row + direction, oppKingPos.col + colOffset);
            if (shieldPos.isValid()) {
                Piece piece = board.getPiece(shieldPos);
                if (piece.type == PieceType::PAWN && piece.color == opponentColor) {
                    opponentScore += 20;
                }
            }
        }
    }
    
    return myScore - opponentScore;
}

int EvaluatorV2::evaluateKingActivity(const Board& board, Color color) {
    int score = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.type == PieceType::KING && piece.color == color) {
                // Reward centralization in endgame
                int distFromCenter = std::abs(row - 3.5) + std::abs(col - 3.5);
                score += (14 - distFromCenter * 2);
                
                // Mobility bonus
                std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                    board, Position(row, col), piece);
                score += moves.size() * 3;
            }
        }
    }
    
    return score;
}

int EvaluatorV2::evaluatePawnRaces(const Board& board, Color color) {
    // Simplified pawn race evaluation
    return 0;
}

int EvaluatorV2::evaluateOpposition(const Board& board, Color aiColor) {
    // Simplified opposition evaluation
    return 0;
}

// ============================================================================
// EVALUATORV2: PIECE COORDINATION
// ============================================================================

int EvaluatorV2::evaluatePieceCoordination(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int myScore = 0;
    int opponentScore = 0;
    
    GamePhase phase = detectPhase(board);
    
    myScore += evaluateRooks(board, aiColor, phase);
    myScore += evaluateBishops(board, aiColor);
    myScore += evaluateKnights(board, aiColor);
    myScore += evaluateQueens(board, aiColor);
    
    opponentScore += evaluateRooks(board, opponentColor, phase);
    opponentScore += evaluateBishops(board, opponentColor);
    opponentScore += evaluateKnights(board, opponentColor);
    opponentScore += evaluateQueens(board, opponentColor);
    
    return myScore - opponentScore;
}

// ============================================================================
// EVALUATORV2: TACTICS
// ============================================================================

int EvaluatorV2::evaluateTactics(const Board& board, Color aiColor) {
    int score = 0;
    
    score += detectPins(board, aiColor);
    score += detectForks(board, aiColor);
    score += detectHangingPieces(board, aiColor);
    score += detectDiscoveredAttacks(board, aiColor);
    
    return score;
}

int EvaluatorV2::detectPins(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int score = 0;
    
    // Check if we're pinning opponent pieces
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position pos(row, col);
            if (isPinned(board, pos, opponentColor)) {
                Piece piece = board.getPiece(pos);
                score += PIECE_VALUES[static_cast<int>(piece.type)] / 5;
            }
        }
    }
    
    // Penalty if our pieces are pinned
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Position pos(row, col);
            if (isPinned(board, pos, aiColor)) {
                Piece piece = board.getPiece(pos);
                score -= PIECE_VALUES[static_cast<int>(piece.type)] / 5;
            }
        }
    }
    
    return score;
}

int EvaluatorV2::detectForks(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int score = 0;
    
    // Check our pieces for fork opportunities
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.color != aiColor) continue;
            
            std::vector<Position> moves = MoveGenerator::getValidMovesForPiece(
                board, Position(row, col), piece);
            
            for (const Position& move : moves) {
                int threatenedValue = 0;
                
                // Check what this piece attacks from the move square
                Board testBoard = board.clone();
                testBoard.setPiece(move, piece);
                testBoard.setPiece(Position(row, col), Piece());
                
                std::vector<Position> attacks = MoveGenerator::getValidMovesForPiece(
                    testBoard, move, piece);
                
                for (const Position& attack : attacks) {
                    Piece target = board.getPiece(attack);
                    if (!target.isEmpty() && target.color == opponentColor) {
                        threatenedValue += PIECE_VALUES[static_cast<int>(target.type)];
                    }
                }
                
                // If attacking multiple pieces, it's a fork
                if (threatenedValue > PIECE_VALUES[static_cast<int>(piece.type)] * 2) {
                    score += 30;
                }
            }
        }
    }
    
    return score;
}

int EvaluatorV2::detectHangingPieces(const Board& board, Color aiColor) {
    Color opponentColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    int score = 0;
    
    // Detect opponent hanging pieces (good for us)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.color != opponentColor) continue;
            
            Position pos(row, col);
            int attackers = countAttackers(board, pos, aiColor);
            int defenders = countDefenders(board, pos, opponentColor);
            
            if (attackers > defenders) {
                score += HANGING_PIECE_PENALTY;
            }
        }
    }
    
    // Detect our hanging pieces (bad for us)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board.getPiece(Position(row, col));
            if (piece.isEmpty() || piece.color != aiColor) continue;
            
            Position pos(row, col);
            int attackers = countAttackers(board, pos, opponentColor);
            int defenders = countDefenders(board, pos, aiColor);
            
            if (attackers > defenders) {
                score -= HANGING_PIECE_PENALTY;
            }
        }
    }
    
    return score;
}

int EvaluatorV2::detectDiscoveredAttacks(const Board& board, Color aiColor) {
    // Simplified - would require more complex analysis
    return 0;
}

int EvaluatorV2::evaluateThreats(const Board& board, Color aiColor) {
    // Combined threat evaluation
    return detectHangingPieces(board, aiColor);
}

// ============================================================================
// MINIMAXENGINEV2: CONSTRUCTOR AND SETUP
// ============================================================================

MinimaxEngineV2::MinimaxEngineV2(int depth) 
    : depth_(depth), maxTime_(0), timeExpired_(false) {
    
    killerMoves_.resize(MAX_PLY);
    for (auto& km : killerMoves_) {
        km.reserve(2);
    }
    
    // Initialize history table
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                for (int l = 0; l < 8; l++) {
                    historyTable_[i][j][k][l] = 0;
                }
            }
        }
    }
}

void MinimaxEngineV2::setDepth(int depth) {
    depth_ = depth;
}

void MinimaxEngineV2::setMaxTime(int milliseconds) {
    maxTime_ = milliseconds;
}

bool MinimaxEngineV2::isTimeExpired() const {
    if (maxTime_ <= 0) return false;
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStartTime_
    ).count();
    
    return elapsed >= maxTime_;
}

void MinimaxEngineV2::resetSearchData() {
    for (auto& km : killerMoves_) {
        km.clear();
    }
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                for (int l = 0; l < 8; l++) {
                    historyTable_[i][j][k][l] = 0;
                }
            }
        }
    }
}

// ============================================================================
// MINIMAXENGINEV2: HELPER FUNCTIONS
// ============================================================================

bool MinimaxEngineV2::isCheckmate(const Board& board, Color color, const CastlingRights& castling) {
    if (!MoveGenerator::isKingInCheck(board, color)) {
        return false;
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    return moves.empty();
}

bool MinimaxEngineV2::isStalemate(const Board& board, Color color, const CastlingRights& castling) {
    if (MoveGenerator::isKingInCheck(board, color)) {
        return false;
    }
    
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
            if (move.from.row == 0) {
                if (move.from.col == 0) newRights.whiteQueenSide = false;
                if (move.from.col == 7) newRights.whiteKingSide = false;
            }
        } else {
            if (move.from.row == 7) {
                if (move.from.col == 0) newRights.blackQueenSide = false;
                if (move.from.col == 7) newRights.blackKingSide = false;
            }
        }
    }
    
    return newRights;
}

// ============================================================================
// MINIMAXENGINEV2: MOVE ORDERING
// ============================================================================

int MinimaxEngineV2::getMoveScore(const Move& move, const Board& board, int ply) {
    int score = 0;
    
    // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
    if (!move.captured.isEmpty()) {
        int victimValue = EvaluatorV2::PIECE_VALUES[static_cast<int>(move.captured.type)];
        int attackerValue = EvaluatorV2::PIECE_VALUES[static_cast<int>(move.piece.type)];
        score += (victimValue * 10 - attackerValue);
    }
    
    // Killer moves
    if (ply < MAX_PLY) {
        for (const Move& killer : killerMoves_[ply]) {
            if (killer.from.row == move.from.row && killer.from.col == move.from.col &&
                killer.to.row == move.to.row && killer.to.col == move.to.col) {
                score += 1000;
                break;
            }
        }
    }
    
    // History heuristic
    score += historyTable_[move.from.row][move.from.col][move.to.row][move.to.col] / 10;
    
    // Promotions
    if (move.promotionType != PieceType::NONE) {
        score += EvaluatorV2::PIECE_VALUES[static_cast<int>(move.promotionType)];
    }
    
    return score;
}

std::vector<Move> MinimaxEngineV2::orderMoves(const std::vector<Move>& moves, 
                                              const Board& board, Color color, int ply) {
    std::vector<std::pair<Move, int>> scoredMoves;
    scoredMoves.reserve(moves.size());
    
    for (const Move& move : moves) {
        int score = getMoveScore(move, board, ply);
        scoredMoves.push_back({move, score});
    }
    
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<Move> orderedMoves;
    orderedMoves.reserve(moves.size());
    for (const auto& sm : scoredMoves) {
        orderedMoves.push_back(sm.first);
    }
    
    return orderedMoves;
}

// ============================================================================
// MINIMAXENGINEV2: QUIESCENCE SEARCH
// ============================================================================

int MinimaxEngineV2::quiescenceSearch(const Board& board, Color currentColor, 
                                     int alpha, int beta, const CastlingRights& castling) {
    
    if (isTimeExpired()) {
        timeExpired_ = true;
        return EvaluatorV2::evaluate(board, rootColor_, currentPhase_);
    }
    
    int standPat = EvaluatorV2::evaluate(board, rootColor_, currentPhase_);
    
    if (currentColor != rootColor_) {
        standPat = -standPat;
    }
    
    if (standPat >= beta) {
        return beta;
    }
    
    if (alpha < standPat) {
        alpha = standPat;
    }
    
    // Only consider captures
    std::vector<Move> allMoves = MoveGenerator::generateMoves(board, currentColor, castling);
    std::vector<Move> captures;
    
    for (const Move& move : allMoves) {
        if (!move.captured.isEmpty()) {
            captures.push_back(move);
        }
    }
    
    // Order captures by MVV-LVA
    std::sort(captures.begin(), captures.end(), [](const Move& a, const Move& b) {
        int aValue = EvaluatorV2::PIECE_VALUES[static_cast<int>(a.captured.type)] * 10 -
                     EvaluatorV2::PIECE_VALUES[static_cast<int>(a.piece.type)];
        int bValue = EvaluatorV2::PIECE_VALUES[static_cast<int>(b.captured.type)] * 10 -
                     EvaluatorV2::PIECE_VALUES[static_cast<int>(b.piece.type)];
        return aValue > bValue;
    });
    
    for (const Move& move : captures) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = -quiescenceSearch(newBoard, nextColor, -beta, -alpha, newCastling);
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

// ============================================================================
// MINIMAXENGINEV2: ALPHA-BETA SEARCH
// ============================================================================

int MinimaxEngineV2::alphaBetaSearch(const Board& board, Color currentColor, 
                                    int depth, int alpha, int beta,
                                    const CastlingRights& castling, int ply, bool allowNull) {
    
    if (isTimeExpired()) {
        timeExpired_ = true;
        return EvaluatorV2::evaluate(board, rootColor_, currentPhase_);
    }
    
    // Check for terminal conditions
    if (isCheckmate(board, currentColor, castling)) {
        return (currentColor == rootColor_) ? -(CHECKMATE_SCORE - ply) : (CHECKMATE_SCORE - ply);
    }
    
    if (isStalemate(board, currentColor, castling)) {
        return STALEMATE_SCORE;
    }
    
    // Reach depth limit - go into quiescence search
    if (depth <= 0) {
        return quiescenceSearch(board, currentColor, alpha, beta, castling);
    }
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, currentColor, castling);
    
    if (moves.empty()) {
        return STALEMATE_SCORE;
    }
    
    // Order moves for better pruning
    moves = orderMoves(moves, board, currentColor, ply);
    
    bool isMaximizing = (currentColor == rootColor_);
    int bestScore = isMaximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (currentColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        int score = alphaBetaSearch(newBoard, nextColor, depth - 1, alpha, beta, 
                                   newCastling, ply + 1, true);
        
        if (isMaximizing) {
            if (score > bestScore) {
                bestScore = score;
            }
            if (score > alpha) {
                alpha = score;
            }
        } else {
            if (score < bestScore) {
                bestScore = score;
            }
            if (score < beta) {
                beta = score;
            }
        }
        
        // Alpha-beta pruning
        if (beta <= alpha) {
            // Store killer move
            if (move.captured.isEmpty() && ply < MAX_PLY) {
                bool found = false;
                for (const Move& killer : killerMoves_[ply]) {
                    if (killer.from.row == move.from.row && killer.from.col == move.from.col &&
                        killer.to.row == move.to.row && killer.to.col == move.to.col) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (killerMoves_[ply].size() >= 2) {
                        killerMoves_[ply][1] = killerMoves_[ply][0];
                        killerMoves_[ply][0] = move;
                    } else {
                        killerMoves_[ply].push_back(move);
                    }
                }
            }
            
            // Update history
            historyTable_[move.from.row][move.from.col][move.to.row][move.to.col] += depth * depth;
            
            break;
        }
    }
    
    return bestScore;
}

// ============================================================================
// MINIMAXENGINEV2: MAIN SEARCH
// ============================================================================

Move MinimaxEngineV2::findBestMove(const Board& board, Color color, const CastlingRights& castling) {
    rootColor_ = color;
    currentPhase_ = EvaluatorV2::detectPhase(board);
    timeExpired_ = false;
    searchStartTime_ = std::chrono::steady_clock::now();
    resetSearchData();
    
    std::vector<Move> moves = MoveGenerator::generateMoves(board, color, castling);
    
    if (moves.empty()) {
        return Move();
    }
    
    if (moves.size() == 1) {
        return moves[0];
    }
    
    // Check for immediate checkmate
    for (const Move& move : moves) {
        Board newBoard = board.clone();
        newBoard.applyMove(move);
        
        CastlingRights newCastling = updateCastlingRights(castling, move, board);
        Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
        
        if (isCheckmate(newBoard, nextColor, newCastling)) {
            return move;
        }
    }
    
    Move bestMove = moves[0];
    int bestScore = std::numeric_limits<int>::min();
    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();
    
    // Iterative deepening
    for (int currentDepth = 1; currentDepth <= depth_; currentDepth++) {
        if (isTimeExpired()) break;
        
        // Order moves based on previous iteration
        moves = orderMoves(moves, board, color, 0);
        
        int iterBestScore = std::numeric_limits<int>::min();
        Move iterBestMove = moves[0];
        
        for (const Move& move : moves) {
            if (isTimeExpired()) break;
            
            Board newBoard = board.clone();
            newBoard.applyMove(move);
            
            CastlingRights newCastling = updateCastlingRights(castling, move, board);
            Color nextColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            
            int score = alphaBetaSearch(newBoard, nextColor, currentDepth - 1, 
                                       alpha, beta, newCastling, 1, true);
            
            // Prefer quicker checkmates
            if (score >= CHECKMATE_SCORE - 100) {
                score += (100 - currentDepth);
            }
            
            if (score > iterBestScore) {
                iterBestScore = score;
                iterBestMove = move;
            }
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        if (!isTimeExpired()) {
            bestScore = iterBestScore;
            bestMove = iterBestMove;
        }
    }
    
    return bestMove;
}

} // namespace Chess
