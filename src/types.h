#pragma once
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

// ============================================================
// HELPER FUNCTIONS
// ============================================================

// Convert square index to algebraic notation (e.g., 4 -> "e1")
inline std::string square_to_string(int square) {
    std::string str;
    str += (square % 8) + 'a';
    str += (square / 8) + '1';
    return str;
}

// Convert algebraic notation to square index (e.g., "e1" -> 4)
inline int string_to_square(const std::string& str) {
    return (str[0] - 'a') + (str[1] - '1') * 8;
}

// Convert move to string (e.g., "e2e4")
inline std::string move_to_string(Move move) {
    std::string str;
    str += square_to_string(move.from);
    str += square_to_string(move.to);
    
    if(move.promotion != NONE_PIECE) {
        switch(move.promotion) {
            case QUEEN: str += 'q'; break;
            case ROOK: str += 'r'; break;
            case BISHOP: str += 'b'; break;
            case KNIGHT: str += 'n'; break;
            default: break;
        }
    }
    
    return str;
}

// Parse move from string (e.g., "e2e4")
inline Move parse_move(const std::string& str) {
    Move move;
    move.from = string_to_square(str.substr(0, 2));
    move.to = string_to_square(str.substr(2, 2));
    move.promotion = NONE_PIECE;
    
    if(str.length() == 5) {
        switch(str[4]) {
            case 'q': move.promotion = QUEEN; break;
            case 'r': move.promotion = ROOK; break;
            case 'b': move.promotion = BISHOP; break;
            case 'n': move.promotion = KNIGHT; break;
            default: break;
        }
    }
    
    return move;
}
