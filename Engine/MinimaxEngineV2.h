#ifndef MINIMAX_ENGINE_V2_H
#define MINIMAX_ENGINE_V2_H

#include "Engine.h"
#include <limits>
#include <chrono>

namespace Chess {

// Evaluator for V2 engine with improved evaluation
class EvaluatorV2 {
public:
    static int evaluate(const Board& board, Color aiColor, const CastlingRights& castling, 
                       bool whiteHasCastled, bool blackHasCastled, int moveCount);
    static const int PIECE_VALUES[7];
    static int countMaterial(const Board& board, Color color);
    static int evaluatePieceDevelopment(const Board& board, Color color, int moveCount);
    static int evaluateMobility(const Board& board, Color color, const CastlingRights& castling);
    static bool isPieceOnStartingSquare(PieceType type, Color color, Position pos);
    static int countUndevelopedPieces(const Board& board, Color color);
    static bool isRookDeveloped(const Board& board, Color color);
    static int evaluateEndgame(const Board& board, Color aiColor, const CastlingRights& castling);
    static bool isEndgame(const Board& board);
    static Position findKing(const Board& board, Color color);
};

// AI engine using minimax with alpha-beta pruning
class MinimaxEngineV2 {
public:
    MinimaxEngineV2(int depth = 3);
    
    Move findBestMove(const Board& board, Color color, const CastlingRights& castling,
                     const std::vector<std::string>& positionHistory = {},
                     bool whiteHasCastled = false, bool blackHasCastled = false);
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
    bool whiteHasCastled_;
    bool blackHasCastled_;
    
    bool isTimeExpired() const;
    int minimax(const Board& board, Color currentColor, int depth, bool maximizing,
                int alpha, int beta, const CastlingRights& castling, int moveCount);
    void orderMoves(std::vector<Move>& moves, const Board& board);
    CastlingRights updateCastlingRights(const CastlingRights& rights, const Move& move, 
                                       const Board& board);
    int moveCount_;
};

} // namespace Chess

#endif // MINIMAX_ENGINE_V2_H
