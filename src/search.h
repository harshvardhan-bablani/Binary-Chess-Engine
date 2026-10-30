#pragma once
#include "board.h"
#include "movegen.h"
#include "eval.h"
#include <atomic>
#include <chrono>
#include <vector>

// ============================================================
// SEARCHER
// ============================================================

// What one search is allowed to spend. A zero field means "no limit of this
// kind"; `infinite` searches until stop() is called (or the depth cap).
struct SearchLimits {
    int time_ms = 0;
    int depth = 0;
    long long nodes = 0;
    bool infinite = false;
};

class Searcher {
public:
    Searcher();

    // Search for the best move.
    // The time-only overload is what the play mode and terminal mode use.
    Move search(Position& pos, int time_ms);
    Move search(Position& pos, const SearchLimits& limits);

    // Ask a running search to return as soon as possible. Safe to call from
    // another thread; the best move found so far is still returned.
    void stop();
    void clear_stop();

    // Get search statistics
    long long get_nodes_searched() const;
    int get_depth() const;
    int get_score() const;  // centipawns, always from White's point of view

    // Suppress the "info depth ..." lines (used by the interactive play mode,
    // which wants the move but not a protocol stream).
    void set_silent(bool value);

private:
    // Alpha-beta search
    int alphabeta(Position& pos, int depth, int alpha, int beta, bool maximizing, int ply);

    // Quiescence search (captures, plus every legal move when in check)
    int quiescence(Position& pos, int alpha, int beta, bool maximizing, int ply);

    // True when the current search has run out of time, nodes or patience.
    bool limit_reached();

    // Order moves for better pruning
    void order_moves(Position& pos, std::vector<Move>& moves);

    // Move scoring for ordering
    int score_move(const Position& pos, const Move& move);

    // Search state
    long long nodes;
    int search_depth;
    int best_score;
    Move best_move;

    // Limits
    std::chrono::steady_clock::time_point start_time;
    int time_limit_ms;
    int depth_limit;
    long long node_limit;
    bool time_up;

    // Written by the UCI thread, read by the search thread.
    std::atomic<bool> stop_requested;

    // When true, search() stays quiet (no "info depth ..." output).
    bool silent;
};
