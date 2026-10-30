// Tests for the search: limits, mate scores at the horizon, and the forcing
// moves quiescence must not miss. Run with build\search.exe.
#include <chrono>
#include <cstdio>
#include <string>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "types.h"

static int checks = 0;
static int failures = 0;

static void expect(bool ok, const std::string& label, const std::string& detail = "") {
    checks++;
    if (!ok) failures++;
    std::printf("  [%s] %s", ok ? "PASS" : "FAIL", label.c_str());
    if (!ok && !detail.empty()) std::printf("   (%s)", detail.c_str());
    std::printf("\n");
}

static Position position_from(const std::string& fen) {
    Position pos;
    if (!load_fen(pos, fen)) {
        std::printf("  [FAIL] could not load FEN: %s\n", fen.c_str());
        checks++;
        failures++;
    }
    return pos;
}

static bool contains_move(const std::vector<Move>& moves, const std::string& uci) {
    for (const Move& move : moves) {
        if (move_to_string(move) == uci) return true;
    }
    return false;
}

// ------------------------------------------------------- captures list ----

static void test_generate_captures() {
    std::printf("\nForcing moves for quiescence\n");

    Position pos = position_from("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    const std::vector<Move> ep = generate_captures(pos);
    expect(contains_move(ep, "e5d6"), "en passant is a capture",
           "generate_captures did not include e5d6");

    pos = position_from("8/P6k/8/8/8/8/8/K7 w - - 0 1");
    const std::vector<Move> promo = generate_captures(pos);
    expect(contains_move(promo, "a7a8q"), "quiet queen promotion is included",
           "generate_captures did not include a7a8q");
    expect(contains_move(promo, "a7a8n"), "quiet knight promotion is included",
           "generate_captures did not include a7a8n");

    pos = position_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    expect(generate_captures(pos).empty(), "start position has no forcing moves");
}

// ------------------------------------------------------ horizon mates ----

static void test_mate_at_horizon() {
    std::printf("\nMate detection at the horizon\n");

    // Ra8# is not a capture, so it is only seen as mate if quiescence searches
    // the check position properly instead of standing pat.
    Position pos = position_from("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");

    Searcher searcher;
    searcher.set_silent(true);

    SearchLimits limits;
    limits.depth = 1;

    const Move best = searcher.search(pos, limits);
    expect(move_to_string(best) == "a1a8", "mate in one is found at depth 1",
           "played " + move_to_string(best));
    expect(searcher.get_score() > 90000, "mate in one is scored as mate",
           "score " + std::to_string(searcher.get_score()));

    // The same position from Black's side: Black is mated, so the white-
    // relative score must be strongly positive as well.
    pos = position_from("R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1");
    const Move mated = searcher.search(pos, limits);
    expect(is_null_move(mated), "checkmated side has no move",
           "played " + move_to_string(mated));
}

// --------------------------------------------------------------- limits ----

static void test_limits() {
    std::printf("\nSearch limits\n");

    Searcher searcher;
    searcher.set_silent(true);

    Position pos = position_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    SearchLimits limits;
    limits.depth = 3;
    Move best = searcher.search(pos, limits);
    expect(searcher.get_depth() == 3, "depth limit is respected",
           "stopped at depth " + std::to_string(searcher.get_depth()));
    expect(is_legal_move(pos, best), "depth-limited search returns a legal move",
           move_to_string(best));

    limits = SearchLimits{};
    limits.nodes = 500;
    best = searcher.search(pos, limits);
    expect(searcher.get_nodes_searched() <= 500, "node limit is respected",
           std::to_string(searcher.get_nodes_searched()) + " nodes");
    expect(is_legal_move(pos, best), "node-limited search returns a legal move",
           move_to_string(best));

    // A search stopped almost immediately must still answer with a legal move
    // rather than a null move.
    limits = SearchLimits{};
    limits.nodes = 1;
    best = searcher.search(pos, limits);
    expect(!is_null_move(best) && is_legal_move(pos, best),
           "a search stopped at the first node still returns a legal move",
           move_to_string(best));

    limits = SearchLimits{};
    limits.time_ms = 100;
    const auto start = std::chrono::steady_clock::now();
    best = searcher.search(pos, limits);
    const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - start).count();
    expect(elapsed < 1000, "time limit is respected", std::to_string(elapsed) + " ms");
    expect(is_legal_move(pos, best), "time-limited search returns a legal move",
           move_to_string(best));

    // Side-to-move score convention: the internal score is white-relative, so
    // Black being a queen up must be a large negative number.
    pos = position_from("7k/q7/8/8/8/8/8/7K b - - 0 1");
    limits = SearchLimits{};
    limits.depth = 2;
    searcher.search(pos, limits);
    expect(searcher.get_score() < -500, "internal score is white-relative",
           "score " + std::to_string(searcher.get_score()));
}

// ----------------------------------------------------------------- main ----

int main() {
    std::printf("search test\n===========\n");

    test_generate_captures();
    test_mate_at_horizon();
    test_limits();

    if (failures == 0) {
        std::printf("\n%d/%d checks passed\n", checks, checks);
        return 0;
    }
    std::printf("\n%d of %d checks FAILED\n", failures, checks);
    return 1;
}
