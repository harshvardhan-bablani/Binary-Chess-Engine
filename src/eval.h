#pragma once
#include "board.h"

// ============================================================
// EVALUATION
// ============================================================

// Piece values in centipawns
const int PIECE_VALUES[6] = {
    100,   // PAWN
    320,   // KNIGHT
    330,   // BISHOP
    500,   // ROOK
    900,   // QUEEN
    20000  // KING
};

// Piece-square tables (from white's perspective, rank 1 = index 0)
extern const int PST[6][64];

// Evaluate position (positive = white advantage)
int evaluate(const Position& pos);
