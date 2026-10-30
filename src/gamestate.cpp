#include "gamestate.h"

#include "movegen.h"
#include "types.h"

#include <algorithm>

namespace {

int piece_count(const Position& pos, int color, Piece piece) {
    return __builtin_popcountll(pos.bitboards[color][piece]);
}

// Colour of a square: a1 is dark, so (file + rank) even means dark.
int square_color(int square) {
    return (square % 8 + square / 8) % 2;
}

}  // namespace

bool insufficient_material(const Position& pos) {
    for (int color = WHITE; color <= BLACK; ++color) {
        if (piece_count(pos, color, PAWN) != 0) return false;
        if (piece_count(pos, color, ROOK) != 0) return false;
        if (piece_count(pos, color, QUEEN) != 0) return false;
    }

    const int white_knights = piece_count(pos, WHITE, KNIGHT);
    const int white_bishops = piece_count(pos, WHITE, BISHOP);
    const int black_knights = piece_count(pos, BLACK, KNIGHT);
    const int black_bishops = piece_count(pos, BLACK, BISHOP);

    const int minors = white_knights + white_bishops + black_knights + black_bishops;
    if (minors <= 1) return true;  // K vs K, K+N vs K, K+B vs K

    if (white_knights == 0 && black_knights == 0 &&
        white_bishops == 1 && black_bishops == 1) {
        return square_color(__builtin_ctzll(pos.bitboards[WHITE][BISHOP])) ==
               square_color(__builtin_ctzll(pos.bitboards[BLACK][BISHOP]));
    }

    return false;
}

std::string position_key(const Position& pos) {
    const std::string fen = to_fen(pos);

    // Drop the halfmove clock and fullmove number: repetition ignores them.
    const std::size_t last = fen.find_last_of(' ');
    if (last == std::string::npos) return fen;
    const std::size_t before_last = fen.find_last_of(' ', last - 1);
    return (before_last == std::string::npos) ? fen : fen.substr(0, before_last);
}

Outcome detect_outcome(const Position& pos, const std::vector<std::string>& keys) {
    if (generate_moves(pos).empty()) {
        return is_in_check(pos) ? Outcome::Checkmate : Outcome::Stalemate;
    }
    if (pos.halfmove_clock >= 100) return Outcome::FiftyMove;
    if (!keys.empty() && std::count(keys.begin(), keys.end(), keys.back()) >= 3) {
        return Outcome::Threefold;
    }
    if (insufficient_material(pos)) return Outcome::Insufficient;
    return Outcome::None;
}

std::string describe_outcome(Outcome outcome, const Position& pos) {
    switch (outcome) {
        case Outcome::Checkmate: {
            const char* winner = (pos.side_to_move == WHITE) ? "Black" : "White";
            return std::string("Checkmate. ") + winner + " wins.";
        }
        case Outcome::Stalemate:    return "Stalemate. It is a draw.";
        case Outcome::FiftyMove:    return "Draw by the 50-move rule.";
        case Outcome::Threefold:    return "Draw by threefold repetition.";
        case Outcome::Insufficient: return "Draw: neither side can force mate.";
        default:                    return "";
    }
}
