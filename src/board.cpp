#include "board.h"
#include <iostream>
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
    pos.castling = 0x0F;  // All castling rights (KQkq)
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
void load_fen(Position& pos, const std::string& fen) {
    // Clear position
    for(int c = 0; c < 2; c++) {
        for(int p = 0; p < 6; p++) {
            pos.bitboards[c][p] = 0;
        }
    }
    
    int square = 56;  // Start at a8
    
    // Parse piece placement
    for(char c : fen) {
        if(c == '/') {
            square -= 16;  // Go to next rank
        } else if(c == ' ') {
            break;
        } else if(c >= '1' && c <= '8') {
            square += (c - '0');
        } else {
            Color color = isupper(c) ? WHITE : BLACK;
            Piece piece;
            
            switch(tolower(c)) {
                case 'p': piece = PAWN; break;
                case 'n': piece = KNIGHT; break;
                case 'b': piece = BISHOP; break;
                case 'r': piece = ROOK; break;
                case 'q': piece = QUEEN; break;
                case 'k': piece = KING; break;
                default: piece = NONE_PIECE; break;
            }
            
            if(piece != NONE_PIECE) {
                pos.bitboards[color][piece] |= (1ULL << square);
            }
            square++;
        }
    }
    
    // Update combined bitboards
    pos.white_pieces = 0;
    pos.black_pieces = 0;
    for(int p = 0; p < 6; p++) {
        pos.white_pieces |= pos.bitboards[WHITE][p];
        pos.black_pieces |= pos.bitboards[BLACK][p];
    }
    pos.all_pieces = pos.white_pieces | pos.black_pieces;
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
                    fen += ('0' + empty);
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
                
                if(color == BLACK) c = tolower(c);
                fen += c;
            }
        }
        
        if(empty > 0) {
            fen += ('0' + empty);
        }
        
        if(rank > 0) fen += '/';
    }
    
    // Side to move
    fen += (pos.side_to_move == WHITE) ? " w " : " b ";
    
    // Castling
    std::string castling = "";
    if(pos.castling & 0x08) castling += 'K';
    if(pos.castling & 0x04) castling += 'Q';
    if(pos.castling & 0x02) castling += 'k';
    if(pos.castling & 0x01) castling += 'q';
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
