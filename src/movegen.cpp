#include "movegen.h"
#include <cmath>

// ============================================================
// ATTACK GENERATION
// ============================================================

// Precomputed knight attacks
uint64_t knight_attacks(int square) {
    uint64_t attacks = 0;
    int offsets[] = {17, 15, 10, 6, -6, -10, -15, -17};
    
    int rank = square / 8;
    int file = square % 8;
    
    for(int i = 0; i < 8; i++) {
        int target = square + offsets[i];
        if(target >= 0 && target < 64) {
            int target_rank = target / 8;
            int target_file = target % 8;
            if((abs(rank - target_rank) == 2 && abs(file - target_file) == 1) ||
               (abs(rank - target_rank) == 1 && abs(file - target_file) == 2)) {
                attacks |= (1ULL << target);
            }
        }
    }
    
    return attacks;
}

// Bishop attacks
uint64_t bishop_attacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int directions[] = {9, 11, -9, -11};
    
    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            
            if(copy < 0 || copy >= 64) break;
            
            int rank_diff = abs((copy / 8) - (square / 8));
            int file_diff = abs((copy % 8) - (square % 8));
            if(rank_diff != file_diff) break;
            
            attacks |= (1ULL << copy);
            
            if(occupied & (1ULL << copy)) break;
        }
    }
    
    return attacks;
}

// Rook attacks
uint64_t rook_attacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int directions[] = {1, -1, 8, -8};
    
    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            
            if(copy < 0 || copy >= 64) break;
            
            if(directions[d] == 1 && (copy % 8) == 0) break;
            if(directions[d] == -1 && (copy % 8) == 7) break;
            
            attacks |= (1ULL << copy);
            
            if(occupied & (1ULL << copy)) break;
        }
    }
    
    return attacks;
}

// Queen attacks
uint64_t queen_attacks(int square, uint64_t occupied) {
    return rook_attacks(square, occupied) | bishop_attacks(square, occupied);
}

// King attacks
uint64_t king_attacks(int square) {
    uint64_t attacks = 0;
    int offsets[] = {1, -1, 8, -8, 9, 11, -9, -11};
    
    int rank = square / 8;
    int file = square % 8;
    
    for(int i = 0; i < 8; i++) {
        int target = square + offsets[i];
        if(target >= 0 && target < 64) {
            int target_rank = target / 8;
            int target_file = target % 8;
            if(abs(rank - target_rank) <= 1 && abs(file - target_file) <= 1) {
                attacks |= (1ULL << target);
            }
        }
    }
    
    return attacks;
}

// White pawn attacks
uint64_t white_pawn_attacks(int square) {
    uint64_t attacks = 0;
    int file = square % 8;
    
    if(file < 7) attacks |= (1ULL << (square + 9));
    if(file > 0) attacks |= (1ULL << (square + 7));
    
    return attacks;
}

// Black pawn attacks
uint64_t black_pawn_attacks(int square) {
    uint64_t attacks = 0;
    int file = square % 8;
    
    if(file < 7) attacks |= (1ULL << (square - 7));
    if(file > 0) attacks |= (1ULL << (square - 9));
    
    return attacks;
}

// ============================================================
// CHECK IF SQUARE IS ATTACKED
// ============================================================
bool is_attacked(const Position& pos, int square, int by_color) {
    // Check pawn attacks
    if(by_color == WHITE) {
        if(square >= 16) {
            int file = square % 8;
            if(file < 7 && (pos.bitboards[WHITE][PAWN] & (1ULL << (square - 7)))) return true;
            if(file > 0 && (pos.bitboards[WHITE][PAWN] & (1ULL << (square - 9)))) return true;
        }
    } else {
        if(square < 48) {
            int file = square % 8;
            if(file < 7 && (pos.bitboards[BLACK][PAWN] & (1ULL << (square + 9)))) return true;
            if(file > 0 && (pos.bitboards[BLACK][PAWN] & (1ULL << (square + 7)))) return true;
        }
    }
    
    // Check knight attacks
    uint64_t knights = pos.bitboards[by_color][KNIGHT];
    if(knights & knight_attacks(square)) return true;
    
    // Check king attacks
    uint64_t kings = pos.bitboards[by_color][KING];
    if(kings & king_attacks(square)) return true;
    
    // Check sliding pieces (bishop, rook, queen)
    uint64_t occupied = pos.all_pieces;
    
    // Bishop/queen attacks (diagonals)
    uint64_t bishops = pos.bitboards[by_color][BISHOP] | pos.bitboards[by_color][QUEEN];
    if(bishops & bishop_attacks(square, occupied)) return true;
    
    // Rook/queen attacks (straight lines)
    uint64_t rooks = pos.bitboards[by_color][ROOK] | pos.bitboards[by_color][QUEEN];
    if(rooks & rook_attacks(square, occupied)) return true;
    
    return false;
}

// Check if current side is in check
bool is_in_check(const Position& pos) {
    int king_square = __builtin_ctzll(pos.bitboards[pos.side_to_move][KING]);
    return is_attacked(pos, king_square, 1 - pos.side_to_move);
}

// ============================================================
// MOVE GENERATION
// ============================================================

// Helper to add a move if it's legal
void add_move(std::vector<Move>& moves, const Position& pos, int from, int to, Piece promotion) {
    Move move;
    move.from = from;
    move.to = to;
    move.promotion = promotion;
    
    // Make the move on a copy to check legality
    Position test_pos = pos;
    if(make_move(test_pos, move)) {
        moves.push_back(move);
    }
}

// Generate all pseudo-legal moves
std::vector<Move> generate_moves(const Position& pos) {
    std::vector<Move> moves;
    int us = pos.side_to_move;
    int them = 1 - us;
    
    uint64_t our_pieces = (us == WHITE) ? pos.white_pieces : pos.black_pieces;
    uint64_t their_pieces = (us == WHITE) ? pos.black_pieces : pos.white_pieces;
    uint64_t occupied = pos.all_pieces;
    uint64_t empty = ~occupied;
    
    // Generate pawn moves
    uint64_t pawns = pos.bitboards[us][PAWN];
    int pawn_push = (us == WHITE) ? 8 : -8;
    int pawn_double = (us == WHITE) ? 16 : -16;
    int start_rank = (us == WHITE) ? 1 : 6;
    int promo_rank = (us == WHITE) ? 6 : 1;
    
    while(pawns) {
        int from = __builtin_ctzll(pawns);
        pawns &= pawns - 1;
        
        int rank = from / 8;
        int file = from % 8;
        
        // Single push
        int to = from + pawn_push;
        if(to >= 0 && to < 64 && (empty & (1ULL << to))) {
            if(rank == promo_rank) {
                add_move(moves, pos, from, to, QUEEN);
                add_move(moves, pos, from, to, ROOK);
                add_move(moves, pos, from, to, BISHOP);
                add_move(moves, pos, from, to, KNIGHT);
            } else {
                add_move(moves, pos, from, to, NONE_PIECE);
            }
            
            // Double push from starting rank
            if(rank == start_rank) {
                int to2 = from + pawn_double;
                if(empty & (1ULL << to2)) {
                    add_move(moves, pos, from, to2, NONE_PIECE);
                }
            }
        }
        
        // Captures
        uint64_t captures = (us == WHITE) ? white_pawn_attacks(from) : black_pawn_attacks(from);
        captures &= their_pieces;
        
        while(captures) {
            int to = __builtin_ctzll(captures);
            captures &= captures - 1;
            
            if(rank == promo_rank) {
                add_move(moves, pos, from, to, QUEEN);
                add_move(moves, pos, from, to, ROOK);
                add_move(moves, pos, from, to, BISHOP);
                add_move(moves, pos, from, to, KNIGHT);
            } else {
                add_move(moves, pos, from, to, NONE_PIECE);
            }
        }
        
        // En passant
        if(pos.en_passant >= 0) {
            int ep_to = pos.en_passant;
            int ep_rank = (us == WHITE) ? 4 : 3;
            
            if(rank == ep_rank) {
                if(file > 0 && from - 1 == ep_to - pawn_push) {
                    add_move(moves, pos, from, ep_to, NONE_PIECE);
                }
                if(file < 7 && from + 1 == ep_to - pawn_push) {
                    add_move(moves, pos, from, ep_to, NONE_PIECE);
                }
            }
        }
    }
    
    // Generate knight moves
    uint64_t knights = pos.bitboards[us][KNIGHT];
    while(knights) {
        int from = __builtin_ctzll(knights);
        knights &= knights - 1;
        
        uint64_t attacks = knight_attacks(from) & ~our_pieces;
        while(attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            add_move(moves, pos, from, to, NONE_PIECE);
        }
    }
    
    // Generate bishop moves
    uint64_t bishops = pos.bitboards[us][BISHOP];
    while(bishops) {
        int from = __builtin_ctzll(bishops);
        bishops &= bishops - 1;
        
        uint64_t attacks = bishop_attacks(from, occupied) & ~our_pieces;
        while(attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            add_move(moves, pos, from, to, NONE_PIECE);
        }
    }
    
    // Generate rook moves
    uint64_t rooks = pos.bitboards[us][ROOK];
    while(rooks) {
        int from = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        
        uint64_t attacks = rook_attacks(from, occupied) & ~our_pieces;
        while(attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            add_move(moves, pos, from, to, NONE_PIECE);
        }
    }
    
    // Generate queen moves
    uint64_t queens = pos.bitboards[us][QUEEN];
    while(queens) {
        int from = __builtin_ctzll(queens);
        queens &= queens - 1;
        
        uint64_t attacks = queen_attacks(from, occupied) & ~our_pieces;
        while(attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            add_move(moves, pos, from, to, NONE_PIECE);
        }
    }
    
    // Generate king moves
    uint64_t kings = pos.bitboards[us][KING];
    int king_square = __builtin_ctzll(kings);
    
    uint64_t attacks = king_attacks(king_square) & ~our_pieces;
    while(attacks) {
        int to = __builtin_ctzll(attacks);
        attacks &= attacks - 1;
        add_move(moves, pos, king_square, to, NONE_PIECE);
    }
    
    // Castling
    if(us == WHITE) {
        if((pos.castling & 0x08) && !(occupied & (1ULL << 5)) && !(occupied & (1ULL << 6))) {
            // Kingside
            if(!is_attacked(pos, 4, BLACK) && !is_attacked(pos, 5, BLACK) && !is_attacked(pos, 6, BLACK)) {
                add_move(moves, pos, 4, 6, NONE_PIECE);
            }
        }
        if((pos.castling & 0x04) && !(occupied & (1ULL << 3)) && !(occupied & (1ULL << 2)) && !(occupied & (1ULL << 1))) {
            // Queenside
            if(!is_attacked(pos, 4, BLACK) && !is_attacked(pos, 3, BLACK) && !is_attacked(pos, 2, BLACK)) {
                add_move(moves, pos, 4, 2, NONE_PIECE);
            }
        }
    } else {
        if((pos.castling & 0x02) && !(occupied & (1ULL << 61)) && !(occupied & (1ULL << 62))) {
            // Kingside
            if(!is_attacked(pos, 60, WHITE) && !is_attacked(pos, 61, WHITE) && !is_attacked(pos, 62, WHITE)) {
                add_move(moves, pos, 60, 62, NONE_PIECE);
            }
        }
        if((pos.castling & 0x01) && !(occupied & (1ULL << 59)) && !(occupied & (1ULL << 58)) && !(occupied & (1ULL << 57))) {
            // Queenside
            if(!is_attacked(pos, 60, WHITE) && !is_attacked(pos, 59, WHITE) && !is_attacked(pos, 58, WHITE)) {
                add_move(moves, pos, 60, 58, NONE_PIECE);
            }
        }
    }
    
    return moves;
}

// Generate capture moves only
std::vector<Move> generate_captures(const Position& pos) {
    std::vector<Move> all_moves = generate_moves(pos);
    std::vector<Move> captures;
    
    uint64_t their_pieces = (pos.side_to_move == WHITE) ? pos.black_pieces : pos.white_pieces;
    
    for(const Move& move : all_moves) {
        if(their_pieces & (1ULL << move.to)) {
            captures.push_back(move);
        }
    }
    
    return captures;
}

// ============================================================
// MAKE MOVE
// ============================================================

bool make_move(Position& pos, Move move) {
    int us = pos.side_to_move;
    int them = 1 - us;
    int from = move.from;
    int to = move.to;
    
    // Find what piece is moving
    Color piece_color;
    Piece piece = get_piece(pos, from, piece_color);
    
    if(piece == NONE_PIECE) return false;
    if(piece_color != us) return false;
    
    // Remove piece from source
    pos.bitboards[us][piece] &= ~(1ULL << from);
    
    // If capture, remove enemy piece
    Color captured_color;
    Piece captured = get_piece(pos, to, captured_color);
    if(captured != NONE_PIECE) {
        pos.bitboards[them][captured] &= ~(1ULL << to);
    }
    
    // Place piece at destination
    if(move.promotion != NONE_PIECE) {
        pos.bitboards[us][move.promotion] |= (1ULL << to);
    } else {
        pos.bitboards[us][piece] |= (1ULL << to);
    }
    
    // Handle castling
    if(piece == KING) {
        if(us == WHITE) {
            pos.castling &= ~0x0C;  // Remove white castling
        } else {
            pos.castling &= ~0x03;  // Remove black castling
        }
        
        // Move rook for castling
        if(abs(from - to) == 2) {
            if(to == 6) {  // Kingside
                pos.bitboards[us][ROOK] &= ~(1ULL << 7);
                pos.bitboards[us][ROOK] |= (1ULL << 5);
            } else if(to == 2) {  // Queenside
                pos.bitboards[us][ROOK] &= ~(1ULL << 0);
                pos.bitboards[us][ROOK] |= (1ULL << 3);
            } else if(to == 62) {
                pos.bitboards[us][ROOK] &= ~(1ULL << 63);
                pos.bitboards[us][ROOK] |= (1ULL << 61);
            } else if(to == 58) {
                pos.bitboards[us][ROOK] &= ~(1ULL << 56);
                pos.bitboards[us][ROOK] |= (1ULL << 59);
            }
        }
    }
    
    // Update castling rights for rook moves
    if(piece == ROOK) {
        if(from == 0) pos.castling &= ~0x04;  // a1
        if(from == 7) pos.castling &= ~0x08;  // h1
        if(from == 56) pos.castling &= ~0x01; // a8
        if(from == 63) pos.castling &= ~0x02; // h8
    }
    
    // Handle en passant capture
    if(piece == PAWN && to == pos.en_passant) {
        int captured_pawn = (us == WHITE) ? to - 8 : to + 8;
        pos.bitboards[them][PAWN] &= ~(1ULL << captured_pawn);
    }
    
    // Update en passant square
    if(piece == PAWN && abs(from - to) == 16) {
        pos.en_passant = (from + to) / 2;
    } else {
        pos.en_passant = -1;
    }
    
    // Update combined bitboards
    pos.white_pieces = 0;
    pos.black_pieces = 0;
    for(int p = 0; p < 6; p++) {
        pos.white_pieces |= pos.bitboards[WHITE][p];
        pos.black_pieces |= pos.bitboards[BLACK][p];
    }
    pos.all_pieces = pos.white_pieces | pos.black_pieces;
    
    // Switch side to move
    pos.side_to_move = them;
    
    // Check if move leaves king in check (illegal)
    int king_square = __builtin_ctzll(pos.bitboards[us][KING]);
    if(is_attacked(pos, king_square, them)) {
        return false;  // Illegal move
    }
    
    return true;
}
