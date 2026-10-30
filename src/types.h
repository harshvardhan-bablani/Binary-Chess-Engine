#pragma once
#include <cctype>
#include <cstdint>
#include <string>

// ============================================================
// PIECE TYPES
// ============================================================
enum Piece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    NONE_PIECE = 6
};

// ============================================================
// COLORS
// ============================================================
enum Color {
    WHITE = 0,
    BLACK = 1
};

// ============================================================
// SQUARE NUMBERS
// ============================================================
// a1=0, b1=1, ..., h1=7
// a2=8, b2=9, ..., h8=63
enum Square {
    A1=0, B1, C1, D1, E1, F1, G1, H1,
    A2=8, B2, C2, D2, E2, F2, G2, H2,
    A3=16, B3, C3, D3, E3, F3, G3, H3,
    A4=24, B4, C4, D4, E4, F4, G4, H4,
    A5=32, B5, C5, D5, E5, F5, G5, H5,
    A6=40, B6, C6, D6, E6, F6, G6, H6,
    A7=48, B7, C7, D7, E7, F7, G7, H7,
    A8=56, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = 64
};

// ============================================================
// CASTLING RIGHTS
// ============================================================
// One bit per right, in the same order FEN writes them (KQkq).
enum CastlingRight {
    CASTLE_WK  = 0x08,  // White kingside  (K)
    CASTLE_WQ  = 0x04,  // White queenside (Q)
    CASTLE_BK  = 0x02,  // Black kingside  (k)
    CASTLE_BQ  = 0x01,  // Black queenside (q)
    CASTLE_ALL = 0x0F
};

// ============================================================
// MOVE STRUCT
// ============================================================
struct Move {
    int from;
    int to;
    Piece promotion;

    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotion == other.promotion;
    }
};

// A "null move" means "there is no move" (used for mate/stalemate and as the
// initial value of "best move so far"). It prints as "0000", as UCI expects.
inline Move null_move() {
    return Move{NO_SQUARE, NO_SQUARE, NONE_PIECE};
}

inline bool is_null_move(const Move& move) {
    return move.from == NO_SQUARE && move.to == NO_SQUARE;
}

// ============================================================
// HELPER FUNCTIONS
// ============================================================

// Convert square index to algebraic notation (e.g., 4 -> "e1").
// Returns "--" for anything outside the board instead of wrapping around.
inline std::string square_to_string(int square) {
    if (square < 0 || square >= 64) return "--";

    std::string str;
    str += static_cast<char>('a' + (square % 8));
    str += static_cast<char>('1' + (square / 8));
    return str;
}

// Convert algebraic notation to square index (e.g., "e1" -> 4).
// Returns NO_SQUARE when the text is not a valid square.
inline int string_to_square(const std::string& str) {
    if (str.size() < 2) return NO_SQUARE;

    const char file = str[0];
    const char rank = str[1];
    if (file < 'a' || file > 'h') return NO_SQUARE;
    if (rank < '1' || rank > '8') return NO_SQUARE;

    return (file - 'a') + (rank - '1') * 8;
}

// Convert move to string (e.g., "e2e4"). Null moves print as "0000".
inline std::string move_to_string(const Move& move) {
    if (is_null_move(move)) return "0000";

    std::string str = square_to_string(move.from) + square_to_string(move.to);

    switch(move.promotion) {
        case QUEEN:  str += 'q'; break;
        case ROOK:   str += 'r'; break;
        case BISHOP: str += 'b'; break;
        case KNIGHT: str += 'n'; break;
        default: break;
    }

    return str;
}

// Parse move from string (e.g., "e2e4", "e7e8q").
// Returns a null move if the text is not a well-formed UCI move.
inline Move parse_move(const std::string& str) {
    Move move = null_move();
    if (str.size() < 4) return move;

    const int from = string_to_square(str.substr(0, 2));
    const int to = string_to_square(str.substr(2, 2));
    if (from == NO_SQUARE || to == NO_SQUARE) return move;

    move.from = from;
    move.to = to;

    if (str.size() >= 5) {
        switch (std::tolower(static_cast<unsigned char>(str[4]))) {
            case 'q': move.promotion = QUEEN; break;
            case 'r': move.promotion = ROOK; break;
            case 'b': move.promotion = BISHOP; break;
            case 'n': move.promotion = KNIGHT; break;
            default: break;
        }
    }

    return move;
}
