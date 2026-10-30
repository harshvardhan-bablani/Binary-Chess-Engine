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

// Make a move on the position.
// The move must already be legal (see is_legal_move / generate_moves).
bool make_move(Position& pos, Move move);

// True if `move` is one of the legal moves in `pos`.
// Use this to validate moves that arrive as text (UCI, terminal input).
bool is_legal_move(const Position& pos, Move move);
