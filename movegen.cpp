#include <iostream>
#include <cstdint>
#include <cmath>

// ============================================================
// LESSON 1-8: Bitboard Basics
// ============================================================
// A bitboard is a 64-bit number where each bit represents a square
// Square numbers:
// 56 57 58 59 60 61 62 63  (rank 8)
// 48 49 50 51 52 53 54 55  (rank 7)
// 40 41 42 43 44 45 46 47  (rank 6)
// 32 33 34 35 36 37 38 39  (rank 5)
// 24 25 26 27 28 29 30 31  (rank 4)
// 16 17 18 19 20 21 22 23  (rank 3)
//  8  9 10 11 12 13 14 15  (rank 2)
//  0  1  2  3  4  5  6  7  (rank 1)

// ============================================================
// LESSON 9-10: Board Representation
// ============================================================
// We use 12 bitboards (6 piece types x 2 colors)
struct Position {
    uint64_t white_pawns;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_rooks;
    uint64_t white_queens;
    uint64_t white_king;
    
    uint64_t black_pawns;
    uint64_t black_knights;
    uint64_t black_bishops;
    uint64_t black_rooks;
    uint64_t black_queens;
    uint64_t black_king;
    
    uint64_t all_pieces;
    uint64_t white_pieces;
    uint64_t black_pieces;
    
    int side_to_move;  // 0 = white, 1 = black
};

// Initialize starting position
void init_position(Position& pos) {
    // White pieces
    pos.white_pawns   = 0x000000000000FF00;  // rank 2
    pos.white_knights = 0x0000000000000042;  // b1, g1
    pos.white_bishops = 0x0000000000000024;  // c1, f1
    pos.white_rooks   = 0x0000000000000081;  // a1, h1
    pos.white_queens  = 0x0000000000000008;  // d1
    pos.white_king    = 0x0000000000000010;  // e1
    
    // Black pieces
    pos.black_pawns   = 0x00FF000000000000;  // rank 7
    pos.black_knights = 0x4200000000000000;  // b8, g8
    pos.black_bishops = 0x2400000000000000;  // c8, f8
    pos.black_rooks   = 0x8100000000000000;  // a8, h8
    pos.black_queens  = 0x0800000000000000;  // d8
    pos.black_king    = 0x1000000000000000;  // e8
    
    // Combined
    pos.white_pieces = pos.white_pawns | pos.white_knights | pos.white_bishops |
                       pos.white_rooks | pos.white_queens | pos.white_king;
    pos.black_pieces = pos.black_pawns | pos.black_knights | pos.black_bishops |
                       pos.black_rooks | pos.black_queens | pos.black_king;
    pos.all_pieces = pos.white_pieces | pos.black_pieces;
    
    pos.side_to_move = 0;  // white starts
}

// ============================================================
// LESSON 12: Knight Attacks
// ============================================================
// Knight moves in L-shape: 2+1 squares
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
            if(abs(rank - target_rank) == 2 && abs(file - target_file) == 1 ||
               abs(rank - target_rank) == 1 && abs(file - target_file) == 2) {
                attacks |= (1ULL << target);
            }
        }
    }
    
    return attacks;
}

// ============================================================
// LESSON 14: Bishop Attacks
// ============================================================
// Bishop moves diagonally, slides until blocked
uint64_t bishop_attacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int directions[] = {9, 11, -9, -11};
    
    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            
            // Check if off board
            if(copy < 0 || copy >= 64) break;
            
            // Check for wrapping (diagonal must stay on same diagonal)
            int rank_diff = abs((copy / 8) - (square / 8));
            int file_diff = abs((copy % 8) - (square % 8));
            if(rank_diff != file_diff) break;
            
            // Add this square
            attacks |= (1ULL << copy);
            
            // If occupied, stop
            if(occupied & (1ULL << copy)) break;
        }
    }
    
    return attacks;
}

// ============================================================
// LESSON 15: Rook Attacks
// ============================================================
// Rook moves straight, slides until blocked
uint64_t rook_attacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int directions[] = {1, -1, 8, -8};
    
    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            
            // Check if off board
            if(copy < 0 || copy >= 64) break;
            
            // Check for horizontal wrapping
            if(directions[d] == 1 && (copy % 8) == 0) break;
            if(directions[d] == -1 && (copy % 8) == 7) break;
            
            // Add this square
            attacks |= (1ULL << copy);
            
            // If occupied, stop
            if(occupied & (1ULL << copy)) break;
        }
    }
    
    return attacks;
}

// ============================================================
// LESSON 16: Queen Attacks
// ============================================================
// Queen = Rook + Bishop
uint64_t queen_attacks(int square, uint64_t occupied) {
    return rook_attacks(square, occupied) | bishop_attacks(square, occupied);
}

// ============================================================
// LESSON 16: King Attacks
// ============================================================
// King moves one square in any direction
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

// ============================================================
// LESSON 17: Pawn Attacks
// ============================================================

// White pawn forward moves
uint64_t white_pawn_forward(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    
    int one_forward = square + 8;
    if(one_forward < 64 && !(occupied & (1ULL << one_forward))) {
        attacks |= (1ULL << one_forward);
        
        // Two squares forward from starting rank (rank 2)
        if(square >= 8 && square <= 15) {
            int two_forward = square + 16;
            if(!(occupied & (1ULL << two_forward))) {
                attacks |= (1ULL << two_forward);
            }
        }
    }
    
    return attacks;
}

// White pawn captures
uint64_t white_pawn_captures(int square, uint64_t enemy_pieces) {
    uint64_t attacks = 0;
    int file = square % 8;
    
    // Capture right (+9)
    if(file < 7) {
        int target = square + 9;
        if(target < 64 && (enemy_pieces & (1ULL << target))) {
            attacks |= (1ULL << target);
        }
    }
    
    // Capture left (+7)
    if(file > 0) {
        int target = square + 7;
        if(target < 64 && (enemy_pieces & (1ULL << target))) {
            attacks |= (1ULL << target);
        }
    }
    
    return attacks;
}

// White pawn all moves
uint64_t white_pawn_attacks(int square, uint64_t occupied, uint64_t enemy_pieces) {
    return white_pawn_forward(square, occupied) | white_pawn_captures(square, enemy_pieces);
}

// Black pawn forward moves
uint64_t black_pawn_forward(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    
    int one_forward = square - 8;
    if(one_forward >= 0 && !(occupied & (1ULL << one_forward))) {
        attacks |= (1ULL << one_forward);
        
        // Two squares forward from starting rank (rank 7)
        if(square >= 48 && square <= 55) {
            int two_forward = square - 16;
            if(!(occupied & (1ULL << two_forward))) {
                attacks |= (1ULL << two_forward);
            }
        }
    }
    
    return attacks;
}

// Black pawn captures
uint64_t black_pawn_captures(int square, uint64_t enemy_pieces) {
    uint64_t attacks = 0;
    int file = square % 8;
    
    // Capture right (-7)
    if(file < 7) {
        int target = square - 7;
        if(target >= 0 && (enemy_pieces & (1ULL << target))) {
            attacks |= (1ULL << target);
        }
    }
    
    // Capture left (-9)
    if(file > 0) {
        int target = square - 9;
        if(target >= 0 && (enemy_pieces & (1ULL << target))) {
            attacks |= (1ULL << target);
        }
    }
    
    return attacks;
}

// Black pawn all moves
uint64_t black_pawn_attacks(int square, uint64_t occupied, uint64_t enemy_pieces) {
    return black_pawn_forward(square, occupied) | black_pawn_captures(square, enemy_pieces);
}

// ============================================================
// Helper: Print bitboard as chess squares
// ============================================================
void print_bitboard(uint64_t bb, const char* name) {
    std::cout << name << ": ";
    for(int i = 0; i < 64; i++) {
        if(bb & (1ULL << i)) {
            char file = 'a' + (i % 8);
            int rank = (i / 8) + 1;
            std::cout << file << rank << " ";
        }
    }
    std::cout << std::endl;
}

// ============================================================
// TEST ALL FUNCTIONS
// ============================================================
int main() {
    Position pos;
    init_position(pos);
    
    std::cout << "=== MOVE GENERATION TEST ===" << std::endl << std::endl;
    
    // Test knight on e4
    print_bitboard(knight_attacks(28), "Knight on e4");
    
    // Test bishop on e4 (empty board)
    print_bitboard(bishop_attacks(28, 0), "Bishop on e4 (empty)");
    
    // Test rook on e4 (empty board)
    print_bitboard(rook_attacks(28, 0), "Rook on e4 (empty)");
    
    // Test queen on e4 (empty board)
    print_bitboard(queen_attacks(28, 0), "Queen on e4 (empty)");
    
    // Test king on e4
    print_bitboard(king_attacks(28), "King on e4");
    
    // Test white pawn on e2
    print_bitboard(white_pawn_attacks(12, pos.all_pieces, pos.black_pieces), 
                   "White pawn on e2");
    
    // Test white pawn on e5 with enemy on d6 and f6
    uint64_t test_occupied = (1ULL << 28) | (1ULL << 43) | (1ULL << 45);
    uint64_t test_enemy = (1ULL << 43) | (1ULL << 45);
    print_bitboard(white_pawn_attacks(28, test_occupied, test_enemy), 
                   "White pawn on e5 (enemies on d6, f6)");
    
    // Test black pawn on e7
    print_bitboard(black_pawn_attacks(52, pos.all_pieces, pos.white_pieces), 
                   "Black pawn on e7");
    
    // Test with starting position
    std::cout << std::endl << "=== STARTING POSITION ===" << std::endl;
    print_bitboard(pos.all_pieces, "All pieces");
    print_bitboard(pos.white_pieces, "White pieces");
    print_bitboard(pos.black_pieces, "Black pieces");
    
    return 0;
}
