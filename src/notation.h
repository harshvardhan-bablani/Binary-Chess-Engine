#pragma once
#include "types.h"
#include "board.h"
#include <string>
#include <vector>

// ============================================================
// HUMAN-FRIENDLY NOTATION
// ============================================================
//
// UCI speaks "g1f3". Humans speak "Nf3". These helpers translate between the
// two so the play mode can accept what a person actually types, and can answer
// with something readable.

// Render a legal move in standard algebraic notation, e.g. "Nbd2", "exd5",
// "O-O", "e8=Q+", "Qxf7#".
// `pos` must be the position the move is played in: SAN needs it for capture
// marks, disambiguation and the check/mate suffix.
std::string move_to_san(const Position& pos, Move move);

// Parse a move typed by a human. Accepts
//   - UCI:        "e2e4", "e7e8q"
//   - SAN:        "e4", "Nf3", "exd5", "Nbd2", "O-O", "O-O-O", "e8=Q"
//   - relaxed:    "nf3", "e8q", "0-0", "Ra8+" (case, check marks and '=' are
//                 optional; a missing disambiguation is resolved when only one
//                 piece can legally make the move)
// Returns false and fills `error` with a sentence for the player otherwise.
bool parse_user_move(const Position& pos, const std::string& text, Move& out,
                     std::string& error);

// The legal moves of `pos` in SAN, for "show me my options" and error hints.
std::vector<std::string> legal_moves_san(const Position& pos);
