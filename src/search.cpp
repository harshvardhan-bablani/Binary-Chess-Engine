#include "search.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace {

// Scores beyond this are mate scores. A mate found at ply p is worth
// MATE - p, so shorter mates always beat longer ones.
const int MATE = 100000;
const int MATE_THRESHOLD = 90000;

}  // namespace

// ============================================================
// SEARCHER CONSTRUCTOR
// ============================================================

Searcher::Searcher() : nodes(0), search_depth(0), best_score(0),
                       time_limit_ms(0), depth_limit(0), node_limit(0),
                       time_up(false), stop_requested(false), silent(false) {
    best_move = null_move();
}

// ============================================================
// MOVE ORDERING
// ============================================================

int Searcher::score_move(const Position& pos, const Move& move) {
    int score = 0;
    
    // Prioritize captures (MVV-LVA)
    Color captured_color;
    Piece captured = get_piece(pos, move.to, captured_color);
    if(captured != NONE_PIECE) {
        score += 10000 + PIECE_VALUES[captured] - PIECE_VALUES[get_piece(pos, move.from, captured_color)];
    }
    
    // Prioritize promotions
    if(move.promotion != NONE_PIECE) {
        score += 9000 + PIECE_VALUES[move.promotion];
    }
    
    return score;
}

void Searcher::order_moves(Position& pos, std::vector<Move>& moves) {
    // Score each move
    std::vector<std::pair<int, Move>> scored_moves;
    for(const Move& move : moves) {
        scored_moves.push_back({score_move(pos, move), move});
    }
    
    // Sort by score (highest first)
    std::sort(scored_moves.begin(), scored_moves.end(),
              [](const std::pair<int, Move>& a, const std::pair<int, Move>& b) {
                  return a.first > b.first;
              });
    
    // Extract sorted moves
    moves.clear();
    for(const auto& pair : scored_moves) {
        moves.push_back(pair.second);
    }
}

// ============================================================
// LIMITS
// ============================================================

bool Searcher::limit_reached() {
    if(stop_requested.load(std::memory_order_relaxed)) {
        time_up = true;
        return true;
    }

    if(node_limit > 0 && nodes >= node_limit) {
        time_up = true;
        return true;
    }

    // Reading the clock at every node is wasteful; every 1024 nodes is still
    // a few milliseconds at this engine's speed.
    if(time_limit_ms > 0 && (nodes & 1023) == 0) {
        if(std::chrono::steady_clock::now() - start_time >=
           std::chrono::milliseconds(time_limit_ms)) {
            time_up = true;
            return true;
        }
    }

    return false;
}

void Searcher::stop() {
    stop_requested.store(true, std::memory_order_relaxed);
}

void Searcher::clear_stop() {
    stop_requested.store(false, std::memory_order_relaxed);
}

// ============================================================
// QUIESCENCE SEARCH
// ============================================================
//
// When the side to move is in check there is no "stand pat": the position is
// not quiet, so every legal evasion is searched. Without this the search can
// return a peaceful evaluation for a position that is actually mate.

int Searcher::quiescence(Position& pos, int alpha, int beta, bool maximizing, int ply) {
    nodes++;

    if(limit_reached()) return 0;

    const bool in_check = is_in_check(pos);
    std::vector<Move> moves;

    if(in_check) {
        moves = generate_moves(pos);
        if(moves.empty()) {
            // Checkmate: the side to move has already lost.
            return maximizing ? -(MATE - ply) : (MATE - ply);
        }
    } else {
        const int stand_pat = evaluate(pos);

        if(maximizing) {
            if(stand_pat >= beta) return beta;
            if(stand_pat > alpha) alpha = stand_pat;
        } else {
            if(stand_pat <= alpha) return alpha;
            if(stand_pat < beta) beta = stand_pat;
        }

        moves = generate_captures(pos);
    }

    order_moves(pos, moves);

    for(const Move& move : moves) {
        if(time_up) break;

        Position new_pos = pos;
        if(!make_move(new_pos, move)) continue;

        int score = quiescence(new_pos, alpha, beta, !maximizing, ply + 1);

        if(maximizing) {
            if(score >= beta) return beta;
            if(score > alpha) alpha = score;
        } else {
            if(score <= alpha) return alpha;
            if(score < beta) beta = score;
        }
    }

    return maximizing ? alpha : beta;
}

// ============================================================
// ALPHA-BETA SEARCH
// ============================================================

int Searcher::alphabeta(Position& pos, int depth, int alpha, int beta, bool maximizing, int ply) {
    nodes++;

    if(limit_reached()) return 0;

    // Base case: hand over to quiescence, which also handles checks.
    if(depth == 0) {
        return quiescence(pos, alpha, beta, maximizing, ply);
    }

    // Generate moves
    std::vector<Move> moves = generate_moves(pos);

    // Check for checkmate/stalemate
    if(moves.empty()) {
        if(is_in_check(pos)) {
            return maximizing ? -(MATE - ply) : (MATE - ply);
        }
        return 0;  // stalemate
    }

    // Order moves for better pruning
    order_moves(pos, moves);

    Move local_best = moves[0];

    if(maximizing) {
        int max_score = -MATE;

        for(const Move& move : moves) {
            if(time_up) break;

            Position new_pos = pos;
            if(!make_move(new_pos, move)) continue;

            int score = alphabeta(new_pos, depth - 1, alpha, beta, false, ply + 1);

            if(score > max_score) {
                max_score = score;
                local_best = move;
            }

            alpha = std::max(alpha, score);
            if(beta <= alpha) break;
        }

        // Update best move at root
        if(depth == search_depth) {
            best_score = max_score;
            best_move = local_best;
        }

        return max_score;
    } else {
        int min_score = MATE;

        for(const Move& move : moves) {
            if(time_up) break;

            Position new_pos = pos;
            if(!make_move(new_pos, move)) continue;

            int score = alphabeta(new_pos, depth - 1, alpha, beta, true, ply + 1);

            if(score < min_score) {
                min_score = score;
                local_best = move;
            }

            beta = std::min(beta, score);
            if(beta <= alpha) break;
        }

        // Update best move at root
        if(depth == search_depth) {
            best_score = min_score;
            best_move = local_best;
        }

        return min_score;
    }
}

// ============================================================
// ITERATIVE DEEPENING SEARCH
// ============================================================

Move Searcher::search(Position& pos, int time_ms) {
    SearchLimits limits;
    limits.time_ms = time_ms;
    return search(pos, limits);
}

Move Searcher::search(Position& pos, const SearchLimits& limits) {
    nodes = 0;
    time_up = false;
    time_limit_ms = limits.time_ms;
    depth_limit = limits.depth;
    node_limit = limits.nodes;
    start_time = std::chrono::steady_clock::now();

    best_move = null_move();
    best_score = 0;

    // No legal moves means checkmate or stalemate. Return a null move so the
    // caller can emit `bestmove 0000` instead of spinning to depth 100.
    const std::vector<Move> legal = generate_moves(pos);
    if(legal.empty()) {
        return null_move();
    }

    // Have an answer ready even if the search is stopped at the first node.
    best_move = legal[0];
    Move confirmed_best = legal[0];

    const int max_depth = (depth_limit > 0) ? depth_limit : 100;

    // Iterative deepening
    for(int depth = 1; depth <= max_depth; depth++) {
        search_depth = depth;
        const int score = alphabeta(pos, depth, -MATE, MATE, pos.side_to_move == WHITE, 0);

        // A partial iteration is not trustworthy: keep the previous depth's
        // move. Depth 1 is the exception, because it is better than nothing.
        if(time_up && depth > 1) break;

        confirmed_best = best_move;
        best_score = score;

        if(!silent) {
            // UCI scores are from the engine's point of view, i.e. the side to
            // move, while the search itself scores from White's point of view.
            const int display_score = (pos.side_to_move == WHITE) ? score : -score;

            std::cout << "info depth " << depth << " score ";
            if(std::abs(display_score) > MATE_THRESHOLD) {
                const int plies = MATE - std::abs(display_score);
                const int moves = (plies + 1) / 2;
                std::cout << "mate " << (display_score > 0 ? moves : -moves);
            } else {
                std::cout << "cp " << display_score;
            }
            std::cout << " nodes " << nodes
                      << " pv " << move_to_string(confirmed_best) << std::endl;
            std::cout.flush();
        }

        if(time_up) break;

        // If we found a forced mate, stop searching.
        if(std::abs(score) > MATE_THRESHOLD) break;
    }

    return confirmed_best;
}

// ============================================================
// GETTERS
// ============================================================

long long Searcher::get_nodes_searched() const {
    return nodes;
}

int Searcher::get_depth() const {
    return search_depth;
}

int Searcher::get_score() const {
    return best_score;
}

// Play mode wants the answer, not a stream of "info depth ..." lines.
void Searcher::set_silent(bool value) {
    silent = value;
}
