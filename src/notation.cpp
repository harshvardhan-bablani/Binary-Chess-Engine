#include "notation.h"
#include "movegen.h"

#include <cctype>
#include <string>
#include <vector>

namespace {

char piece_letter(Piece piece) {
    switch (piece) {
        case KNIGHT: return 'N';
        case BISHOP: return 'B';
        case ROOK:   return 'R';
        case QUEEN:  return 'Q';
        case KING:   return 'K';
        default:     return '\0';
    }
}

const char* piece_name(Piece piece) {
    switch (piece) {
        case PAWN:   return "pawn";
        case KNIGHT: return "knight";
        case BISHOP: return "bishop";
        case ROOK:   return "rook";
        case QUEEN:  return "queen";
        case KING:   return "king";
        default:     return "piece";
    }
}

bool is_castle_move(const Position& pos, const Move& move) {
    Color color = WHITE;
    if (get_piece(pos, move.from, color) != KING) return false;
    const int delta = move.to - move.from;
    return delta == 2 || delta == -2;
}

bool is_capture_move(const Position& pos, const Move& move) {
    Color color = WHITE;
    if (get_piece(pos, move.to, color) != NONE_PIECE) return true;

    // En passant: a pawn moves diagonally onto the en passant square, so the
    // destination itself is empty.
    Color mover = WHITE;
    const Piece piece = get_piece(pos, move.from, mover);
    const bool diagonal = (move.from % 8) != (move.to % 8);
    return piece == PAWN && diagonal && move.to == pos.en_passant;
}

// Everything a player may or may not type (case, '+', '#', 'x', '=', '-',
// spaces, "e.p.") is dropped, so "exd5", "exd5+", "E x D5" and "exd5 e.p."
// all reduce to the same key. Castling reduces to "oo" / "ooo".
std::string canonical(const std::string& text) {
    std::string key;
    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) == 0) continue;  // drops '+', '#', '=', '-', '.', spaces
        key += static_cast<char>(std::tolower(u));
    }

    // "e4e.p." keeps the letters of the en-passant marker; drop the marker so
    // it compares equal to "e4".
    if (key.size() >= 4u && key.compare(key.size() - 2, 2, "ep") == 0) {
        key.erase(key.size() - 2);
    }

    for (char& c : key) {
        if (c == '0') c = 'o';
    }
    return key;
}

struct Parsed {
    bool ok = false;
    Piece piece = PAWN;
    int to = NO_SQUARE;
    Piece promotion = NONE_PIECE;
    int from_file = -1;  // -1 means "not specified"
    int from_rank = -1;
};

Parsed parse_structural(const std::string& raw) {
    Parsed parsed;

    std::string text;
    for (char c : raw) {
        if (std::isalnum(static_cast<unsigned char>(c)) != 0) text += c;
    }
    if (text.size() < 2u) return parsed;

    // Promotion: a trailing piece letter, e.g. "e8q" or "e8=Q".
    const char last = text.back();
    const char last_lower = static_cast<char>(std::tolower(static_cast<unsigned char>(last)));
    if (text.size() >= 3u && (last_lower == 'q' || last_lower == 'r' ||
                             last_lower == 'b' || last_lower == 'n')) {
        switch (last_lower) {
            case 'q': parsed.promotion = QUEEN; break;
            case 'r': parsed.promotion = ROOK; break;
            case 'b': parsed.promotion = BISHOP; break;
            default:  parsed.promotion = KNIGHT; break;
        }
        text.pop_back();
    }

    if (text.size() < 2u) return parsed;

    const int to = string_to_square(text.substr(text.size() - 2, 2));
    if (to == NO_SQUARE) return parsed;
    parsed.to = to;
    text.erase(text.size() - 2);

    if (!text.empty()) {
        const char first = text.front();
        switch (first) {
            case 'K': parsed.piece = KING;   text.erase(0, 1); break;
            case 'Q': parsed.piece = QUEEN;  text.erase(0, 1); break;
            case 'R': parsed.piece = ROOK;   text.erase(0, 1); break;
            case 'B': parsed.piece = BISHOP; text.erase(0, 1); break;
            case 'N': parsed.piece = KNIGHT; text.erase(0, 1); break;
            default:  parsed.piece = PAWN;   break;
        }
    }

    for (char c : text) {
        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == 'x') continue;
        if (lower >= 'a' && lower <= 'h') { parsed.from_file = lower - 'a'; continue; }
        if (lower >= '1' && lower <= '8') { parsed.from_rank = lower - '1'; continue; }
        return parsed;  // something we do not understand
    }

    parsed.ok = true;
    return parsed;
}

std::string join_san(const std::vector<Move>& moves, const Position& pos) {
    std::string list;
    for (const Move& move : moves) {
        if (!list.empty()) list += ", ";
        list += move_to_san(pos, move);
    }
    return list;
}

}  // namespace

// ============================================================
// MOVE -> SAN
// ============================================================

std::string move_to_san(const Position& pos, Move move) {
    if (is_null_move(move)) return "--";

    if (is_castle_move(pos, move)) {
        return (move.to > move.from) ? "O-O" : "O-O-O";
    }

    Color color = WHITE;
    const Piece piece = get_piece(pos, move.from, color);
    const bool capture = is_capture_move(pos, move);

    std::string san;
    if (piece == PAWN) {
        if (capture) {
            san += static_cast<char>('a' + (move.from % 8));
            san += 'x';
        }
        san += square_to_string(move.to);
        if (move.promotion != NONE_PIECE) {
            san += '=';
            san += piece_letter(move.promotion);
        }
    } else {
        san += piece_letter(piece);

        // Disambiguate against every other legal move of the same piece type
        // to the same square, as SAN requires: file if it is enough, else rank,
        // else both.
        bool shares_file = false;
        bool shares_rank = false;
        int rivals = 0;
        for (const Move& other : generate_moves(pos)) {
            if (other.to != move.to || other.from == move.from) continue;
            Color other_color = WHITE;
            if (get_piece(pos, other.from, other_color) != piece) continue;
            rivals++;
            if (other.from % 8 == move.from % 8) shares_file = true;
            if (other.from / 8 == move.from / 8) shares_rank = true;
        }
        if (rivals > 0) {
            if (!shares_file) {
                san += static_cast<char>('a' + (move.from % 8));
            } else if (!shares_rank) {
                san += static_cast<char>('1' + (move.from / 8));
            } else {
                san += static_cast<char>('a' + (move.from % 8));
                san += static_cast<char>('1' + (move.from / 8));
            }
        }

        if (capture) san += 'x';
        san += square_to_string(move.to);
    }

    // Check and mate suffix, decided on the position the move leads to.
    Position after = pos;
    if (make_move(after, move)) {
        if (generate_moves(after).empty()) {
            if (is_in_check(after)) san += '#';
        } else if (is_in_check(after)) {
            san += '+';
        }
    }
    return san;
}

// ============================================================
// TEXT -> MOVE
// ============================================================

bool parse_user_move(const Position& pos, const std::string& text, Move& out,
                     std::string& error) {
    const std::vector<Move> legal = generate_moves(pos);
    if (legal.empty()) {
        error = "the game is over: there are no legal moves";
        return false;
    }

    std::string trimmed = text;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())) != 0) {
        trimmed.erase(0, 1);
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())) != 0) {
        trimmed.pop_back();
    }
    if (trimmed.empty()) {
        error = "type a move, for example e4, Nf3 or O-O";
        return false;
    }

    // 1. UCI, the form the engine itself speaks.
    const Move uci = parse_move(trimmed);
    if (!is_null_move(uci) && is_legal_move(pos, uci)) {
        out = uci;
        return true;
    }

    const std::string key = canonical(trimmed);

    // 2. Castling, including "oo", "0-0-0" and "O-O".
    if (key == "oo" || key == "ooo") {
        const bool kingside = (key == "oo");
        for (const Move& move : legal) {
            if (!is_castle_move(pos, move)) continue;
            if (kingside == (move.to > move.from)) {
                out = move;
                return true;
            }
        }
        error = kingside ? "you cannot castle kingside here"
                         : "you cannot castle queenside here";
        return false;
    }

    // 3. Exact SAN match. Comparing against the SAN of each legal move means
    //    the rules (captures, disambiguation, promotion) cannot drift.
    std::vector<Move> matches;
    for (const Move& move : legal) {
        if (canonical(move_to_san(pos, move)) == key) matches.push_back(move);
    }
    if (matches.size() == 1) {
        out = matches[0];
        return true;
    }

    // 4. Structural parse: accepts a missing disambiguation ("Nd2" for "Nbd2")
    //    and explains failures in words rather than "illegal move".
    const Parsed parsed = parse_structural(trimmed);
    if (!parsed.ok) {
        error = "I do not understand \"" + trimmed + "\". Type a move like e4, Nf3, exd5, O-O, or help";
        return false;
    }

    const bool promotion_rank = (parsed.to / 8 == 0 || parsed.to / 8 == 7);
    matches.clear();
    for (const Move& move : legal) {
        Color color = WHITE;
        if (get_piece(pos, move.from, color) != parsed.piece) continue;
        if (move.to != parsed.to) continue;
        if (parsed.promotion != NONE_PIECE) {
            if (move.promotion != parsed.promotion) continue;
        } else if (parsed.piece == PAWN && promotion_rank) {
            // A pawn reaching the last rank must say what it becomes.
            continue;
        }
        if (parsed.from_file >= 0 && (move.from % 8) != parsed.from_file) continue;
        if (parsed.from_rank >= 0 && (move.from / 8) != parsed.from_rank) continue;
        matches.push_back(move);
    }

    if (matches.size() == 1) {
        out = matches[0];
        return true;
    }

    if (matches.size() > 1) {
        error = "ambiguous: " + join_san(matches, pos);
        return false;
    }

    if (parsed.piece == PAWN && promotion_rank) {
        error = "a pawn on the last rank must promote: write e8=Q (or =R, =B, =N)";
        return false;
    }
    if (parsed.piece == PAWN) {
        error = "no pawn can move to " + square_to_string(parsed.to);
        return false;
    }
    error = std::string("no ") + piece_name(parsed.piece) + " can move to " +
            square_to_string(parsed.to);
    return false;
}

// ============================================================
// HELPERS
// ============================================================

std::vector<std::string> legal_moves_san(const Position& pos) {
    std::vector<std::string> list;
    for (const Move& move : generate_moves(pos)) {
        list.push_back(move_to_san(pos, move));
    }
    return list;
}