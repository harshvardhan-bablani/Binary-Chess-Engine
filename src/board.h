#pragma once
#include "types.h"
#include <cstdint>
#include <string>

// ============================================================
// POSITION STRUCTURE
// ============================================================
struct Position {
    // Bitboards for each piece type and color
    uint64_t bitboards[2][6] = {};  // [color][piece_type]

    // Combined bitboards
    uint64_t white_pieces = 0;
    uint64_t black_pieces = 0;
    uint64_t all_pieces = 0;

    // Game state
    int side_to_move = WHITE;  // WHITE or BLACK
    int castling = 0;          // CASTLE_* bits
    int en_passant = -1;       // en passant target square (-1 if none)
    int halfmove_clock = 0;
    int fullmove_number = 1;
};

// ============================================================
// BOARD FUNCTIONS
// ============================================================

// Initialize starting position
void init_position(Position& pos);

// Print the board
void print_board(const Position& pos);

// Get piece at square
Piece get_piece(const Position& pos, int square, Color& color);

// Load a position from a FEN string.
// Returns false (and leaves `pos` untouched) if the FEN is malformed.
bool load_fen(Position& pos, const std::string& fen);

// Convert position to FEN
std::string to_fen(const Position& pos);
