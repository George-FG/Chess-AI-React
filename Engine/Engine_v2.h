#ifndef ENGINE_V2_H
#define ENGINE_V2_H

#include "Engine.h"
#include <vector>
#include <limits>
#include <chrono>
#include <unordered_map>

namespace Chess {

// Game phase detection
enum class GamePhase {
    OPENING,
    MIDGAME,
    ENDGAME
};

// Advanced evaluation with tactical awareness
class EvaluatorV2 {
public:
    static int evaluate(const Board& board, Color aiColor, GamePhase phase);
    static GamePhase detectPhase(const Board& board);
    
private:
    // Core evaluation functions
    static int evaluateMaterial(const Board& board, Color aiColor);
    static int evaluatePosition(const Board& board, Color aiColor, GamePhase phase);
    static int evaluateTactics(const Board& board, Color aiColor);
    static int evaluatePawnStructure(const Board& board, Color aiColor);
    static int evaluateMobility(const Board& board, Color aiColor);
    static int evaluateKingSafety(const Board& board, Color aiColor, GamePhase phase);
    static int evaluatePieceCoordination(const Board& board, Color aiColor);
    static int evaluateThreats(const Board& board, Color aiColor);
    
    // Tactical detection
    static int detectPins(const Board& board, Color aiColor);
    static int detectForks(const Board& board, Color aiColor);
    static int detectHangingPieces(const Board& board, Color aiColor);
    static int detectDiscoveredAttacks(const Board& board, Color aiColor);
    
    // Advanced pawn evaluation
    static int evaluateDoubledPawns(const Board& board, Color color);
    static int evaluateIsolatedPawns(const Board& board, Color color);
    static int evaluatePassedPawns(const Board& board, Color color);
    static int evaluateBackwardPawns(const Board& board, Color color);
    static int evaluatePawnChains(const Board& board, Color color);
    
    // Piece-specific evaluation
    static int evaluateRooks(const Board& board, Color color, GamePhase phase);
    static int evaluateBishops(const Board& board, Color color);
    static int evaluateKnights(const Board& board, Color color);
    static int evaluateQueens(const Board& board, Color color);
    
    // Endgame specific
    static int evaluateKingActivity(const Board& board, Color color);
    static int evaluatePawnRaces(const Board& board, Color color);
    static int evaluateOpposition(const Board& board, Color aiColor);
    
    // Helper functions
    static bool isSquareAttacked(const Board& board, Position pos, Color attackingColor);
    static int countAttackers(const Board& board, Position pos, Color attackingColor);
    static int countDefenders(const Board& board, Position pos, Color defendingColor);
    static bool isPinned(const Board& board, Position pos, Color pieceColor);
    static bool isPassedPawn(const Board& board, Position pos, Color pawnColor);
    static bool isOutpost(const Board& board, Position pos, Color color);
    static bool isOpenFile(const Board& board, int col);
    static bool isSemiOpenFile(const Board& board, int col, Color color);
    static int getManhattanDistance(Position a, Position b);
    static int getChebyshevDistance(Position a, Position b);
    
    static int getMaterialCount(const Board& board);
    static int getPieceCount(const Board& board);
    
public:
    // Piece values
    static const int PIECE_VALUES[7];
    static const int ENDGAME_PIECE_VALUES[7];
    
    // Advanced piece-square tables
    static const int PAWN_TABLE_OPENING[8][8];
    static const int PAWN_TABLE_ENDGAME[8][8];
    static const int KNIGHT_TABLE_OPENING[8][8];
    static const int KNIGHT_TABLE_ENDGAME[8][8];
    static const int BISHOP_TABLE[8][8];
    static const int ROOK_TABLE_OPENING[8][8];
    static const int ROOK_TABLE_ENDGAME[8][8];
    static const int QUEEN_TABLE[8][8];
    static const int KING_OPENING_TABLE[8][8];
    static const int KING_MIDGAME_TABLE[8][8];
    static const int KING_ENDGAME_TABLE[8][8];
    
    // Evaluation weights
    static const int DOUBLED_PAWN_PENALTY = 15;
    static const int ISOLATED_PAWN_PENALTY = 20;
    static const int BACKWARD_PAWN_PENALTY = 10;
    static const int PASSED_PAWN_BONUS[8];  // Bonus by rank
    static const int BISHOP_PAIR_BONUS = 50;
    static const int ROOK_OPEN_FILE_BONUS = 20;
    static const int ROOK_SEMI_OPEN_FILE_BONUS = 10;
    static const int KNIGHT_OUTPOST_BONUS = 30;
    static const int HANGING_PIECE_PENALTY = 50;
};

class MinimaxEngineV2 {
public:
    MinimaxEngineV2(int depth = 4);
    
    Move findBestMove(const Board& board, Color color, const CastlingRights& castling);
    void setDepth(int depth);
    void setMaxTime(int milliseconds); // 0 = no limit
    
private:
    int depth_;
    int maxTime_; // in milliseconds
    Color rootColor_;
    GamePhase currentPhase_;
    std::chrono::steady_clock::time_point searchStartTime_;
    bool timeExpired_;
    
    // Killer moves for move ordering
    std::vector<std::vector<Move>> killerMoves_;
    
    // History heuristic
    int historyTable_[8][8][8][8];
    
    bool isTimeExpired() const;
    void resetSearchData();
    
    // Search functions
    int alphaBetaSearch(const Board& board, Color currentColor, int depth, int alpha, int beta, 
                       const CastlingRights& castling, int ply, bool allowNull);
    int quiescenceSearch(const Board& board, Color currentColor, int alpha, int beta, 
                        const CastlingRights& castling);
    
    // Move ordering
    std::vector<Move> orderMoves(const std::vector<Move>& moves, const Board& board, 
                                 Color color, int ply);
    int getMoveScore(const Move& move, const Board& board, int ply);
    
    // Checkmate detection
    bool isCheckmate(const Board& board, Color color, const CastlingRights& castling);
    bool isStalemate(const Board& board, Color color, const CastlingRights& castling);
    
    // Helper functions
    CastlingRights updateCastlingRights(const CastlingRights& rights, const Move& move, 
                                       const Board& board);
    
    // Checkmate score constants
    static constexpr int CHECKMATE_SCORE = 1000000;
    static constexpr int STALEMATE_SCORE = 0;
    static constexpr int MAX_PLY = 64;
};

} // namespace Chess

#endif // ENGINE_V2_H
