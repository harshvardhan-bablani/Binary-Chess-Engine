#include "search.h"
#include <algorithm>
#include <iostream>

// ============================================================
// SEARCHER CONSTRUCTOR
// ============================================================

Searcher::Searcher() : nodes(0), search_depth(0), best_score(0), 
                       time_limit_ms(5000), time_up(false) {
    best_move = {0, 0, NONE_PIECE};
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
// QUIESCENCE SEARCH
// ============================================================

int Searcher::quiescence(Position& pos, int alpha, int beta, bool maximizing) {
    nodes++;
    
    // Check time
    if(nodes % 4096 == 0) {
        if(std::chrono::steady_clock::now() - start_time >= 
           std::chrono::milliseconds(time_limit_ms)) {
            time_up = true;
            return 0;
        }
    }
    
    int stand_pat = evaluate(pos);
    
    if(maximizing) {
        if(stand_pat >= beta) return beta;
        if(stand_pat > alpha) alpha = stand_pat;
    } else {
        if(stand_pat <= alpha) return alpha;
        if(stand_pat < beta) beta = stand_pat;
    }
    
    // Generate capture moves
    std::vector<Move> captures = generate_captures(pos);
    order_moves(pos, captures);
    
    for(const Move& move : captures) {
        if(time_up) break;
        
        Position new_pos = pos;
        if(!make_move(new_pos, move)) continue;
        
        int score = quiescence(new_pos, alpha, beta, !maximizing);
        
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

int Searcher::alphabeta(Position& pos, int depth, int alpha, int beta, bool maximizing) {
    nodes++;
    
    // Check time
    if(nodes % 4096 == 0) {
        if(std::chrono::steady_clock::now() - start_time >= 
           std::chrono::milliseconds(time_limit_ms)) {
            time_up = true;
            return 0;
        }
    }
    
    // Base case: depth 0 or time up
    if(depth == 0 || time_up) {
        return quiescence(pos, alpha, beta, maximizing);
    }
    
    // Generate moves
    std::vector<Move> moves = generate_moves(pos);
    
    // Check for checkmate/stalemate
    if(moves.empty()) {
        if(is_in_check(pos)) {
            // Checkmate
            return maximizing ? -100000 + (100 - depth) : 100000 - (100 - depth);
        } else {
            // Stalemate
            return 0;
        }
    }
    
    // Order moves for better pruning
    order_moves(pos, moves);
    
    Move local_best = moves[0];
    
    if(maximizing) {
        int max_score = -100000;
        
        for(const Move& move : moves) {
            if(time_up) break;
            
            Position new_pos = pos;
            if(!make_move(new_pos, move)) continue;
            
            int score = alphabeta(new_pos, depth - 1, alpha, beta, false);
            
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
        int min_score = 100000;
        
        for(const Move& move : moves) {
            if(time_up) break;
            
            Position new_pos = pos;
            if(!make_move(new_pos, move)) continue;
            
            int score = alphabeta(new_pos, depth - 1, alpha, beta, true);
            
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
    nodes = 0;
    time_up = false;
    time_limit_ms = time_ms;
    start_time = std::chrono::steady_clock::now();
    
    best_move = {0, 0, NONE_PIECE};
    best_score = 0;
    
    // Iterative deepening
    for(int depth = 1; depth <= 100; depth++) {
        search_depth = depth;
        int score = alphabeta(pos, depth, -100000, 100000, pos.side_to_move == WHITE);
        
        if(time_up && depth > 1) break;
        
        // Output info
        std::cout << "info depth " << depth 
                  << " score cp " << score 
                  << " nodes " << nodes
                  << " pv " << move_to_string(best_move)
                  << std::endl;
        
        // If we found a forced mate, stop searching
        if(abs(score) > 90000) break;
    }
    
    return best_move;
}

// ============================================================
// GETTERS
// ============================================================

int Searcher::get_nodes_searched() const {
    return nodes;
}

int Searcher::get_depth() const {
    return search_depth;
}

int Searcher::get_score() const {
    return best_score;
}
