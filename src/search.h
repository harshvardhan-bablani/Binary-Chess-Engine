#pragma once
#include "board.h"
#include "movegen.h"
#include "eval.h"
#include <vector>
#include <chrono>

// ============================================================
// SEARCHER
// ============================================================

class Searcher {
public:
    Searcher();
    
    // Search for the best move
    Move search(Position& pos, int time_ms);
    
    // Get search statistics
    int get_nodes_searched() const;
    int get_depth() const;
    int get_score() const;

    // Suppress the "info depth ..." lines (used by the interactive play mode,
    // which wants the move but not a protocol stream).
    void set_silent(bool value);
    
private:
    // Alpha-beta search
    int alphabeta(Position& pos, int depth, int alpha, int beta, bool maximizing);
    
    // Quiescence search (search captures only)
    int quiescence(Position& pos, int alpha, int beta, bool maximizing);
    
    // Order moves for better pruning
    void order_moves(Position& pos, std::vector<Move>& moves);
    
    // Move scoring for ordering
    int score_move(const Position& pos, const Move& move);
    
    // Search state
    int nodes;
    int search_depth;
    int best_score;
    Move best_move;
    
    // Time control
    std::chrono::steady_clock::time_point start_time;
    int time_limit_ms;
    bool time_up;

    // When true, search() stays quiet (no "info depth ..." output).
    bool silent;
};
