#include "movegen.h"
#include <cstdlib>

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
    int directions[] = {9, 7, -7, -9};
    
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
    int offsets[] = {1, -1, 8, -8, 9, 7, -7, -9};
    
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
    const uint64_t king = pos.bitboards[pos.side_to_move][KING];
    if (king == 0) return false;  // no king on the board: nothing to attack

    const int king_square = __builtin_ctzll(king);
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
            int cap_to = __builtin_ctzll(captures);
            captures &= captures - 1;
            
            if(rank == promo_rank) {
                add_move(moves, pos, from, cap_to, QUEEN);
                add_move(moves, pos, from, cap_to, ROOK);
                add_move(moves, pos, from, cap_to, BISHOP);
                add_move(moves, pos, from, cap_to, KNIGHT);
            } else {
                add_move(moves, pos, from, cap_to, NONE_PIECE);
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
    if(kings != 0) {
        int king_square = __builtin_ctzll(kings);

        uint64_t attacks = king_attacks(king_square) & ~our_pieces;
        while(attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            add_move(moves, pos, king_square, to, NONE_PIECE);
        }
    }
    
    // Castling
    if(us == WHITE) {
        if((pos.castling & CASTLE_WK) && !(occupied & (1ULL << F1)) && !(occupied & (1ULL << G1))) {
            // Kingside
            if(!is_attacked(pos, E1, BLACK) && !is_attacked(pos, F1, BLACK) && !is_attacked(pos, G1, BLACK)) {
                add_move(moves, pos, E1, G1, NONE_PIECE);
            }
        }
        if((pos.castling & CASTLE_WQ) && !(occupied & (1ULL << D1)) && !(occupied & (1ULL << C1)) && !(occupied & (1ULL << B1))) {
            // Queenside
            if(!is_attacked(pos, E1, BLACK) && !is_attacked(pos, D1, BLACK) && !is_attacked(pos, C1, BLACK)) {
                add_move(moves, pos, E1, C1, NONE_PIECE);
            }
        }
    } else {
        if((pos.castling & CASTLE_BK) && !(occupied & (1ULL << F8)) && !(occupied & (1ULL << G8))) {
            // Kingside
            if(!is_attacked(pos, E8, WHITE) && !is_attacked(pos, F8, WHITE) && !is_attacked(pos, G8, WHITE)) {
                add_move(moves, pos, E8, G8, NONE_PIECE);
            }
        }
        if((pos.castling & CASTLE_BQ) && !(occupied & (1ULL << D8)) && !(occupied & (1ULL << C8)) && !(occupied & (1ULL << B8))) {
            // Queenside
            if(!is_attacked(pos, E8, WHITE) && !is_attacked(pos, D8, WHITE) && !is_attacked(pos, C8, WHITE)) {
                add_move(moves, pos, E8, C8, NONE_PIECE);
            }
        }
    }
    
    return moves;
}

// Generate capture moves only.
//
// Quiescence must see every forcing move, so this includes en passant captures
// (whose destination square is empty) and promotions (which are forcing even
// when they do not capture anything).
std::vector<Move> generate_captures(const Position& pos) {
    std::vector<Move> all_moves = generate_moves(pos);
    std::vector<Move> captures;
    
    uint64_t their_pieces = (pos.side_to_move == WHITE) ? pos.black_pieces : pos.white_pieces;
    
    for(const Move& move : all_moves) {
        const bool is_capture = (their_pieces & (1ULL << move.to)) != 0;
        const bool is_promotion = move.promotion != NONE_PIECE;

        bool is_en_passant = false;
        if(move.to == pos.en_passant) {
            Color mover_color;
            is_en_passant = get_piece(pos, move.from, mover_color) == PAWN &&
                            (move.from % 8) != (move.to % 8);
        }

        if(is_capture || is_promotion || is_en_passant) {
            captures.push_back(move);
        }
    }
    
    return captures;
}

// ============================================================
// MAKE MOVE
// ============================================================

namespace {

// Relocate a rook when the king castles.
void relocate_rook(Position& pos, int color, int from, int to) {
    pos.bitboards[color][ROOK] &= ~(1ULL << from);
    pos.bitboards[color][ROOK] |=  (1ULL << to);
}

}  // namespace

// Apply `move` to `pos`.
//
// IMPORTANT: this function assumes `move` has already been validated, i.e. it
// is one of the moves returned by generate_moves(). It checks that a friendly
// piece really is on `from` and that the move does not leave the mover's king
// in check, but it does not re-check geometry, castling rights or the
// promotion rank. Use is_legal_move() for moves that come from outside the
// engine (UCI input, terminal input).
bool make_move(Position& pos, Move move) {
    const int us = pos.side_to_move;
    const int them = 1 - us;
    const int from = move.from;
    const int to = move.to;

    if(from < A1 || from > H8 || to < A1 || to > H8) return false;

    // Find what piece is moving
    Color piece_color;
    const Piece piece = get_piece(pos, from, piece_color);

    if(piece == NONE_PIECE) return false;
    if(piece_color != us) return false;

    // Remove piece from source
    pos.bitboards[us][piece] &= ~(1ULL << from);

    // If capture, remove enemy piece
    Color captured_color;
    const Piece captured = get_piece(pos, to, captured_color);
    if(captured != NONE_PIECE) {
        pos.bitboards[them][captured] &= ~(1ULL << to);

        // Capturing a rook on its home square also removes that castling
        // right. Without this the engine can later "castle" with a rook that
        // no longer exists, conjuring a new one onto the board.
        if(captured == ROOK) {
            if(to == A1)      pos.castling &= ~CASTLE_WQ;
            else if(to == H1) pos.castling &= ~CASTLE_WK;
            else if(to == A8) pos.castling &= ~CASTLE_BQ;
            else if(to == H8) pos.castling &= ~CASTLE_BK;
        }
    }

    // Place piece at destination
    if(move.promotion != NONE_PIECE) {
        pos.bitboards[us][move.promotion] |= (1ULL << to);
    } else {
        pos.bitboards[us][piece] |= (1ULL << to);
    }

    // Handle castling
    if(piece == KING) {
        const int our_rights = (us == WHITE) ? (CASTLE_WK | CASTLE_WQ)
                                             : (CASTLE_BK | CASTLE_BQ);
        pos.castling &= ~our_rights;

        // Move the rook too. A two-square king move can only be castling.
        if(abs(from - to) == 2) {
            if(to == G1)      relocate_rook(pos, us, H1, F1);
            else if(to == C1) relocate_rook(pos, us, A1, D1);
            else if(to == G8) relocate_rook(pos, us, H8, F8);
            else if(to == C8) relocate_rook(pos, us, A8, D8);
        }
    }

    // Update castling rights for rook moves
    if(piece == ROOK) {
        if(from == A1)      pos.castling &= ~CASTLE_WQ;
        else if(from == H1) pos.castling &= ~CASTLE_WK;
        else if(from == A8) pos.castling &= ~CASTLE_BQ;
        else if(from == H8) pos.castling &= ~CASTLE_BK;
    }

    // Handle en passant capture. The target square is empty, so only a
    // diagonal pawn move onto it can be an en passant capture.
    const bool is_en_passant = piece == PAWN &&
                               to == pos.en_passant &&
                               (from % 8) != (to % 8);
    if(is_en_passant) {
        const int captured_pawn = (us == WHITE) ? to - 8 : to + 8;
        pos.bitboards[them][PAWN] &= ~(1ULL << captured_pawn);
    }

    // Halfmove clock: reset on a pawn move or capture, otherwise advance.
    if(piece == PAWN || captured != NONE_PIECE || is_en_passant) {
        pos.halfmove_clock = 0;
    } else {
        pos.halfmove_clock++;
    }
    if(us == BLACK) pos.fullmove_number++;

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
    const uint64_t king = pos.bitboards[us][KING];
    if(king != 0 && is_attacked(pos, __builtin_ctzll(king), them)) {
        return false;  // Illegal move
    }

    return true;
}

// ============================================================
// LEGALITY CHECK FOR EXTERNAL INPUT
// ============================================================

// True if `move` is one of the moves the side to move may legally play.
// This is the gatekeeper for moves that arrive as text (UCI / terminal).
bool is_legal_move(const Position& pos, Move move) {
    if(is_null_move(move)) return false;

    const std::vector<Move> legal = generate_moves(pos);
    for(const Move& candidate : legal) {
        if(candidate == move) return true;
    }
    return false;
}
