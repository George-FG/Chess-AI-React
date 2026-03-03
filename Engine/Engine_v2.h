#ifndef ENGINE_V2_H
#define ENGINE_V2_H

#include "Engine.h"
#include <vector>
#include <limits>
#include <chrono>

namespace Chess {

// Game phase detection
enum class GamePhase {
    OPENING,
    MIDGAME,
    ENDGAME
};

class EvaluatorV2 {
public:
    static int evaluate(const Board& board, Color aiColor, GamePhase phase);
    static GamePhase detectPhase(const Board& board);
    
private:
    static int evaluateOpening(const Board& board, Color aiColor);
    static int evaluateMidgame(const Board& board, Color aiColor);
    static int evaluateEndgame(const Board& board, Color aiColor);
    
    static int evaluateDevelopment(const Board& board, Color color);
    static int evaluateCenterControl(const Board& board, Color color);
    static int evaluateKingSafety(const Board& board, Color color);
    static int evaluatePawnStructure(const Board& board, Color color);
    static int evaluateMobility(const Board& board, Color color);
    static int evaluateTacticalThreats(const Board& board, Color color);
    static int evaluateKingActivity(const Board& board, Color color);
    static int evaluatePawnAdvancement(const Board& board, Color color);
    static int evaluatePassedPawns(const Board& board, Color color);
    
    static int getMaterialCount(const Board& board);
    static int getPieceCount(const Board& board);
    
    static const int PIECE_VALUES[7];
    
    // Piece-square tables for positional evaluation
    static const int PAWN_TABLE[8][8];
    static const int KNIGHT_TABLE[8][8];
    static const int BISHOP_TABLE[8][8];
    static const int ROOK_TABLE[8][8];
    static const int QUEEN_TABLE[8][8];
    static const int KING_MIDGAME_TABLE[8][8];
    static const int KING_ENDGAME_TABLE[8][8];
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
    
    bool isTimeExpired() const;
    
    // Minimax with alpha-beta pruning
    int minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                int alpha, int beta, const CastlingRights& castling, int ply);
    
    // Checkmate detection
    bool isCheckmate(const Board& board, Color color, const CastlingRights& castling);
    bool isStalemate(const Board& board, Color color, const CastlingRights& castling);
    int detectCheckmateInN(const Board& board, Color color, const CastlingRights& castling, int maxDepth);
    
    // Helper functions
    CastlingRights updateCastlingRights(const CastlingRights& rights, const Move& move, 
                                       const Board& board);
    
    // Checkmate score constants
    static constexpr int CHECKMATE_SCORE = 1000000;
    static constexpr int STALEMATE_SCORE = 0;
};

} // namespace Chess

#endif // ENGINE_V2_H
