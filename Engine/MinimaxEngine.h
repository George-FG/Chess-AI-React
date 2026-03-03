#ifndef MINIMAX_ENGINE_H
#define MINIMAX_ENGINE_H

#include "Engine.h"
#include <limits>
#include <chrono>

namespace Chess {

// Evaluates board positions
class Evaluator {
public:
    static int evaluate(const Board& board, Color aiColor, const CastlingRights& castling, int moveCount = 0);
    static const int PIECE_VALUES[7];
    static bool isInOpeningPhase(int moveCount);
    static int evaluatePieceDevelopment(const Board& board, Color color);
};

// AI engine using minimax with alpha-beta pruning
class MinimaxEngine {
public:
    MinimaxEngine(int depth = 3);
    
    Move findBestMove(const Board& board, Color color, const CastlingRights& castling,
                     const std::vector<std::string>& positionHistory = {});
    void setDepth(int depth);
    void setMaxTime(int milliseconds); // 0 = no limit
    
    // Generate a simple position hash for repetition detection
    static std::string getPositionHash(const Board& board);
    
private:
    int depth_;
    int maxTime_; // in milliseconds
    Color rootColor_;
    std::chrono::steady_clock::time_point searchStartTime_;
    bool timeExpired_;
    
    bool isTimeExpired() const;
    int minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                int alpha, int beta, const CastlingRights& castling, int moveCount);
    CastlingRights updateCastlingRights(const CastlingRights& rights, const Move& move, 
                                       const Board& board);
    int moveCount_;
};

} // namespace Chess

#endif // MINIMAX_ENGINE_H
