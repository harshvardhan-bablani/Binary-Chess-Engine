#pragma once
#include "types.h"
#include <cstdint>

// ============================================================
// POSITION STRUCTURE
// ============================================================
struct Position {
    // Bitboards for each piece type and color
    uint64_t bitboards[2][6];  // [color][piece_type]
    
    // Combined bitboards
    uint64_t white_pieces;
    uint64_t black_pieces;
    uint64_t all_pieces;
    
    // Game state
    int side_to_move;  // WHITE or BLACK
    int castling;      // 4 bits: KQkq
    int en_passant;    // en passant square (-1 if none)
    int halfmove_clock;
    int fullmove_number;
};

// ============================================================
// BOARD FUNCTIONS
// ============================================================

// Initialize starting position
void init_position(Position& pos);

// Print the board
void print_board(const Position& pos);

// Make a move (modifies position)
bool make_move(Position& pos, Move move);

// Get piece at square
Piece get_piece(const Position& pos, int square, Color& color);

// Convert FEN to position
void load_fen(Position& pos, const std::string& fen);

// Convert position to FEN
std::string to_fen(const Position& pos);
