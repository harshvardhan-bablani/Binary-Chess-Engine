#pragma once
#include "board.h"

#include <string>
#include <vector>

// ============================================================
// GAME STATE
// ============================================================
//
// What the rules say about a position, beyond "which moves are legal":
// is the game over, and if so, why. Kept separate from the play UI so it can
// be tested without a terminal.

enum class Outcome {
    None,
    Checkmate,
    Stalemate,
    FiftyMove,
    Threefold,
    Insufficient
};

// True when neither side has enough material to force mate (K vs K, K+minor vs
// K, K+B vs K+B on the same colour). Deliberately conservative.
bool insufficient_material(const Position& pos);

// Board, side to move, castling and en passant only: the part of a FEN that
// repetition cares about. Equal keys mean the same position was repeated.
std::string position_key(const Position& pos);

// `keys` is the position_key() of every position reached so far, oldest first,
// including the current one.
Outcome detect_outcome(const Position& pos, const std::vector<std::string>& keys);

// A sentence for the player, e.g. "Checkmate. White wins."
std::string describe_outcome(Outcome outcome, const Position& pos);
