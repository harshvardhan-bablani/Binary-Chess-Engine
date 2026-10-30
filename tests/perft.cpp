// ============================================================
// Perft test suite
// ============================================================
//
// Perft counts the leaf nodes of the move tree to a given depth. If the
// engine generates the correct moves, the counts match the published
// reference values exactly. A single wrong or missing move shows up as a
// mismatch, which is why every serious engine uses this as its movegen test.
//
// The six positions below are the standard set from the Chess Programming
// Wiki. The expected numbers were cross-checked against python-chess.
//
// Build and run:
//     g++ -std=c++17 -O2 -Isrc -o build/perft tests/perft.cpp src/board.cpp src/movegen.cpp
//     ./build/perft            (full suite)
//     ./build/perft 3          (cap every position at depth 3, for a quick run)
//
// Exit code is 0 only if every check passes.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "board.h"
#include "movegen.h"

namespace {

struct PerftCase {
    const char* name;
    const char* fen;
    int max_depth;
    long long expected[6];  // expected[d] for d in 1..max_depth
};

const PerftCase kCases[] = {
    {
        "startpos",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        5,
        {0, 20, 400, 8902, 197281, 4865609}
    },
    {
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        4,
        {0, 48, 2039, 97862, 4085603, 0}
    },
    {
        "position3",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        4,
        {0, 14, 191, 2812, 43238, 0}
    },
    {
        "position4",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        4,
        {0, 6, 264, 9467, 422333, 0}
    },
    {
        "position5",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        4,
        {0, 44, 1486, 62379, 2103487, 0}
    },
    {
        "position6",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        4,
        {0, 46, 2079, 89890, 3894594, 0}
    }
};

// Count leaf nodes at `depth`. Position is taken by value so each recursion
// gets its own copy; make_move() mutates its argument.
long long perft(Position pos, int depth) {
    if (depth == 0) return 1;

    const std::vector<Move> moves = generate_moves(pos);
    if (depth == 1) return static_cast<long long>(moves.size());

    long long total = 0;
    for (const Move& move : moves) {
        Position next = pos;
        if (make_move(next, move)) total += perft(next, depth - 1);
    }
    return total;
}

double elapsed_ms(std::chrono::steady_clock::time_point start) {
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

int main(int argc, char** argv) {
    int depth_cap = 99;
    if (argc > 1) {
        depth_cap = std::atoi(argv[1]);
        if (depth_cap < 1) depth_cap = 1;
    }

    int checks = 0;
    int failures = 0;
    const auto suite_start = std::chrono::steady_clock::now();

    std::printf("perft suite\n");
    std::printf("===========\n\n");

    for (const PerftCase& test : kCases) {
        Position pos;
        if (!load_fen(pos, test.fen)) {
            std::printf("[FAIL] %-10s  load_fen rejected the FEN\n", test.name);
            checks++;
            failures++;
            continue;
        }

        // A position must survive a FEN round-trip unchanged. This catches
        // parser and serialiser drift that perft alone would not notice.
        checks++;
        const std::string round_trip = to_fen(pos);
        if (round_trip != test.fen) {
            std::printf("[FAIL] %-10s  FEN round-trip\n", test.name);
            std::printf("         in : %s\n", test.fen);
            std::printf("         out: %s\n", round_trip.c_str());
            failures++;
        }

        std::printf("%s\n", test.name);

        const int max_depth = (test.max_depth < depth_cap) ? test.max_depth : depth_cap;
        for (int depth = 1; depth <= max_depth; depth++) {
            const auto start = std::chrono::steady_clock::now();
            const long long got = perft(pos, depth);
            const double ms = elapsed_ms(start);

            const long long want = test.expected[depth];
            const bool ok = (got == want);

            checks++;
            if (!ok) failures++;

            std::printf("  [%s] depth %d  %10lld  %8.1f ms",
                        ok ? "PASS" : "FAIL", depth, got, ms);
            if (!ok) std::printf("   expected %lld", want);
            std::printf("\n");
        }
        std::printf("\n");
    }

    const double total_ms = elapsed_ms(suite_start);

    if (failures == 0) {
        std::printf("%d/%d checks passed in %.2f s\n", checks, checks, total_ms / 1000.0);
        return 0;
    }

    std::printf("%d of %d checks FAILED in %.2f s\n", failures, checks, total_ms / 1000.0);
    return 1;
}
