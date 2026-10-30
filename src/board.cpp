#include "board.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

// ============================================================
// Initialize starting position
// ============================================================
void init_position(Position& pos) {
    // Clear all bitboards
    for(int c = 0; c < 2; c++) {
        for(int p = 0; p < 6; p++) {
            pos.bitboards[c][p] = 0;
        }
    }
    
    // White pawns (rank 2: squares 8-15)
    pos.bitboards[WHITE][PAWN] = 0x000000000000FF00ULL;
    
    // White knights
    pos.bitboards[WHITE][KNIGHT] = 0x0000000000000042ULL;  // b1, g1
    
    // White bishops
    pos.bitboards[WHITE][BISHOP] = 0x0000000000000024ULL;  // c1, f1
    
    // White rooks
    pos.bitboards[WHITE][ROOK] = 0x0000000000000081ULL;    // a1, h1
    
    // White queen
    pos.bitboards[WHITE][QUEEN] = 0x0000000000000008ULL;   // d1
    
    // White king
    pos.bitboards[WHITE][KING] = 0x0000000000000010ULL;    // e1
    
    // Black pawns (rank 7: squares 48-55)
    pos.bitboards[BLACK][PAWN] = 0x00FF000000000000ULL;
    
    // Black knights
    pos.bitboards[BLACK][KNIGHT] = 0x4200000000000000ULL;  // b8, g8
    
    // Black bishops
    pos.bitboards[BLACK][BISHOP] = 0x2400000000000000ULL;  // c8, f8
    
    // Black rooks
    pos.bitboards[BLACK][ROOK] = 0x8100000000000000ULL;    // a8, h8
    
    // Black queen
    pos.bitboards[BLACK][QUEEN] = 0x0800000000000000ULL;   // d8
    
    // Black king
    pos.bitboards[BLACK][KING] = 0x1000000000000000ULL;    // e8
    
    // Combined bitboards
    pos.white_pieces = 0;
    pos.black_pieces = 0;
    for(int p = 0; p < 6; p++) {
        pos.white_pieces |= pos.bitboards[WHITE][p];
        pos.black_pieces |= pos.bitboards[BLACK][p];
    }
    pos.all_pieces = pos.white_pieces | pos.black_pieces;
    
    // Game state
    pos.side_to_move = WHITE;
    pos.castling = CASTLE_ALL;  // All castling rights (KQkq)
    pos.en_passant = -1;
    pos.halfmove_clock = 0;
    pos.fullmove_number = 1;
}

// ============================================================
// Print the board
// ============================================================
void print_board(const Position& pos) {
    std::cout << "\n  a b c d e f g h\n";
    std::cout << "  +----------------+\n";
    
    for(int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " |";
        for(int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char piece = '.';
            
            for(int c = 0; c < 2; c++) {
                for(int p = 0; p < 6; p++) {
                    if(pos.bitboards[c][p] & (1ULL << square)) {
                        if(p == PAWN) piece = (c == WHITE) ? 'P' : 'p';
                        else if(p == KNIGHT) piece = (c == WHITE) ? 'N' : 'n';
                        else if(p == BISHOP) piece = (c == WHITE) ? 'B' : 'b';
                        else if(p == ROOK) piece = (c == WHITE) ? 'R' : 'r';
                        else if(p == QUEEN) piece = (c == WHITE) ? 'Q' : 'q';
                        else if(p == KING) piece = (c == WHITE) ? 'K' : 'k';
                    }
                }
            }
            
            std::cout << " " << piece;
        }
        std::cout << " |\n";
    }
    
    std::cout << "  +----------------+\n";
    std::cout << "  a b c d e f g h\n\n";
}

// ============================================================
// Get piece at square
// ============================================================
Piece get_piece(const Position& pos, int square, Color& color) {
    uint64_t mask = 1ULL << square;
    
    for(int p = 0; p < 6; p++) {
        if(pos.bitboards[WHITE][p] & mask) {
            color = WHITE;
            return static_cast<Piece>(p);
        }
        if(pos.bitboards[BLACK][p] & mask) {
            color = BLACK;
            return static_cast<Piece>(p);
        }
    }
    
    color = WHITE;
    return NONE_PIECE;
}

// ============================================================
// Load FEN string
// ============================================================
namespace {

// Map a FEN piece character to a Piece (NONE_PIECE if it is not one).
Piece piece_from_char(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'p': return PAWN;
        case 'n': return KNIGHT;
        case 'b': return BISHOP;
        case 'r': return ROOK;
        case 'q': return QUEEN;
        case 'k': return KING;
        default:  return NONE_PIECE;
    }
}

// A castling right is only meaningful if the rook it refers to is still home.
void drop_unbacked_castling_rights(Position& pos) {
    const uint64_t white_rooks = pos.bitboards[WHITE][ROOK];
    const uint64_t black_rooks = pos.bitboards[BLACK][ROOK];

    if (!(white_rooks & (1ULL << H1))) pos.castling &= ~CASTLE_WK;
    if (!(white_rooks & (1ULL << A1))) pos.castling &= ~CASTLE_WQ;
    if (!(black_rooks & (1ULL << H8))) pos.castling &= ~CASTLE_BK;
    if (!(black_rooks & (1ULL << A8))) pos.castling &= ~CASTLE_BQ;
}

}  // namespace

bool load_fen(Position& pos, const std::string& fen) {
    // Parse into a temporary so a bad FEN cannot corrupt the live position.
    Position parsed;

    std::istringstream iss(fen);
    std::string placement;
    std::string side;
    std::string castling;
    std::string en_passant;

    if (!(iss >> placement >> side >> castling >> en_passant)) return false;

    // The two clock fields are optional (some tools omit them).
    iss >> parsed.halfmove_clock >> parsed.fullmove_number;
    if (parsed.halfmove_clock < 0) parsed.halfmove_clock = 0;
    if (parsed.fullmove_number < 1) parsed.fullmove_number = 1;

    // ---- Piece placement: exactly 8 ranks of exactly 8 squares ----
    int square = A8;
    int files_in_rank = 0;
    int rank_breaks = 0;

    for (char c : placement) {
        if (c == '/') {
            if (files_in_rank != 8) return false;
            square -= 16;
            files_in_rank = 0;
            rank_breaks++;
        } else if (c >= '1' && c <= '8') {
            const int empty = c - '0';
            if (files_in_rank + empty > 8) return false;
            files_in_rank += empty;
            square += empty;
        } else {
            const Piece piece = piece_from_char(c);
            if (piece == NONE_PIECE) return false;
            if (files_in_rank >= 8 || square < A1 || square > H8) return false;

            const Color color = std::isupper(static_cast<unsigned char>(c)) ? WHITE : BLACK;
            parsed.bitboards[color][piece] |= (1ULL << square);
            files_in_rank++;
            square++;
        }
    }

    if (files_in_rank != 8) return false;   // last rank incomplete
    if (rank_breaks != 7) return false;     // need exactly 8 ranks

    // ---- Side to move ----
    if (side == "w") parsed.side_to_move = WHITE;
    else if (side == "b") parsed.side_to_move = BLACK;
    else return false;

    // ---- Castling rights ----
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': parsed.castling |= CASTLE_WK; break;
                case 'Q': parsed.castling |= CASTLE_WQ; break;
                case 'k': parsed.castling |= CASTLE_BK; break;
                case 'q': parsed.castling |= CASTLE_BQ; break;
                default:  return false;
            }
        }
    }

    // ---- En passant target ----
    if (en_passant != "-") {
        const int ep = string_to_square(en_passant);
        if (ep == NO_SQUARE) return false;
        parsed.en_passant = ep;
    }

    // ---- A position is only playable with exactly one king per side ----
    if (__builtin_popcountll(parsed.bitboards[WHITE][KING]) != 1) return false;
    if (__builtin_popcountll(parsed.bitboards[BLACK][KING]) != 1) return false;

    drop_unbacked_castling_rights(parsed);

    // ---- Combined bitboards ----
    for (int p = 0; p < 6; p++) {
        parsed.white_pieces |= parsed.bitboards[WHITE][p];
        parsed.black_pieces |= parsed.bitboards[BLACK][p];
    }
    parsed.all_pieces = parsed.white_pieces | parsed.black_pieces;

    pos = parsed;
    return true;
}

// ============================================================
// Convert to FEN
// ============================================================
std::string to_fen(const Position& pos) {
    std::string fen;
    
    for(int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for(int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            Color color;
            Piece piece = get_piece(pos, square, color);
            
            if(piece == NONE_PIECE) {
                empty++;
            } else {
                if(empty > 0) {
                    fen += static_cast<char>('0' + empty);
                    empty = 0;
                }
                
                char c;
                switch(piece) {
                    case PAWN: c = 'P'; break;
                    case KNIGHT: c = 'N'; break;
                    case BISHOP: c = 'B'; break;
                    case ROOK: c = 'R'; break;
                    case QUEEN: c = 'Q'; break;
                    case KING: c = 'K'; break;
                    default: c = '?'; break;
                }
                
                if(color == BLACK) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                fen += c;
            }
        }
        
        if(empty > 0) {
            fen += static_cast<char>('0' + empty);
        }
        
        if(rank > 0) fen += '/';
    }
    
    // Side to move
    fen += (pos.side_to_move == WHITE) ? " w " : " b ";
    
    // Castling
    std::string castling = "";
    if(pos.castling & CASTLE_WK) castling += 'K';
    if(pos.castling & CASTLE_WQ) castling += 'Q';
    if(pos.castling & CASTLE_BK) castling += 'k';
    if(pos.castling & CASTLE_BQ) castling += 'q';
    if(castling.empty()) castling = "-";
    fen += castling + " ";
    
    // En passant
    if(pos.en_passant >= 0) {
        fen += square_to_string(pos.en_passant);
    } else {
        fen += "-";
    }
    
    // Halfmove clock and fullmove number
    fen += " " + std::to_string(pos.halfmove_clock);
    fen += " " + std::to_string(pos.fullmove_number);
    
    return fen;
}
