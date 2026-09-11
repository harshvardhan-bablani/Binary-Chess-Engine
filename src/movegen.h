#pragma once
#include "types.h"
#include "board.h"
#include <vector>

// ============================================================
// ATTACK GENERATION
// ============================================================

// Knight attacks for each square
uint64_t knight_attacks(int square);

// Bishop attacks for a square with occupancy
uint64_t bishop_attacks(int square, uint64_t occupied);

// Rook attacks for a square with occupancy
uint64_t rook_attacks(int square, uint64_t occupied);

// Queen attacks (bishop + rook)
uint64_t queen_attacks(int square, uint64_t occupied);

// King attacks for each square
uint64_t king_attacks(int square);

// Pawn attacks
uint64_t white_pawn_attacks(int square);
uint64_t black_pawn_attacks(int square);

// ============================================================
// MOVE GENERATION
// ============================================================

// Generate all pseudo-legal moves for a position
std::vector<Move> generate_moves(const Position& pos);

// Generate only capture moves
std::vector<Move> generate_captures(const Position& pos);

// Check if a square is attacked by a given color
bool is_attacked(const Position& pos, int square, int by_color);

// Check if the current side is in check
bool is_in_check(const Position& pos);

// Make a move on the position
bool make_move(Position& pos, Move move);

// Undo move (requires saved state)
struct MoveState {
    int castling;
    int en_passant;
    int halfmove_clock;
    uint64_t hash;
};

void make_move(Position& pos, Move move, MoveState& state);
void undo_move(Position& pos, Move move, const MoveState& state);
