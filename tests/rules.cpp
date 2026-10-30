// Tests for the parts a human touches: algebraic notation and the game-end
// rules. Run with build\rules.exe (run_tests.bat does it for you).
#include <cstdio>
#include <string>
#include <vector>

#include "board.h"
#include "gamestate.h"
#include "movegen.h"
#include "notation.h"
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

// ---------------------------------------------------------------- SAN ----

static void test_san_round_trip() {
    std::printf("\nSAN round-trip (move -> text -> move)\n");

    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",             // en passant available
        "8/P6k/8/8/8/8/8/K7 w - - 0 1",                  // promotion with check
        "1n5k/P7/8/8/8/8/8/K7 w - - 0 1",                // capture-promotion
        "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",             // mate in one
        "4k3/8/8/8/8/8/8/1N2KN2 w - - 0 1",              // knights needing disambiguation
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",          // castling both ways
    };

    int moves_checked = 0;
    for (const char* fen : fens) {
        Position pos = position_from(fen);
        const std::vector<Move> legal = generate_moves(pos);
        int bad = 0;
        std::string first_bad;
        for (const Move& move : legal) {
            const std::string san = move_to_san(pos, move);
            Move parsed = null_move();
            std::string error;
            if (!parse_user_move(pos, san, parsed, error) || !(parsed == move)) {
                bad++;
                if (first_bad.empty()) {
                    first_bad = san + " -> " + move_to_string(parsed) + " (wanted " +
                                move_to_string(move) + ")" + (error.empty() ? "" : " " + error);
                }
            }
            moves_checked++;
        }
        expect(bad == 0,
               std::string(fen).substr(0, 32) + "...  " + std::to_string(legal.size()) + " moves",
               first_bad);
    }
    std::printf("  %d moves round-tripped\n", moves_checked);
}

static void expect_move(const std::string& fen, const std::string& text,
                        const std::string& uci, const std::string& label) {
    Position pos = position_from(fen);
    Move parsed = null_move();
    std::string error;
    const bool ok = parse_user_move(pos, text, parsed, error);
    expect(ok && move_to_string(parsed) == uci, label,
           ok ? ("got " + move_to_string(parsed)) : error);
}

static void expect_rejected(const std::string& fen, const std::string& text,
                            const std::string& label, const std::string& must_mention = "") {
    Position pos = position_from(fen);
    Move parsed = null_move();
    std::string error;
    const bool rejected = !parse_user_move(pos, text, parsed, error);
    const bool mentions = must_mention.empty() ||
                          error.find(must_mention) != std::string::npos;
    expect(rejected && mentions, label, "accepted " + move_to_string(parsed) + " / " + error);
}

static void test_san_input() {
    std::printf("\nAccepting what a person types\n");
    const std::string start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const std::string kiwipete =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    expect_move(start, "e4", "e2e4", "start: e4");
    expect_move(start, "Nf3", "g1f3", "start: Nf3");
    expect_move(start, "nf3", "g1f3", "start: lowercase nf3");
    expect_move(start, "Nf3+", "g1f3", "start: stray check mark ignored");
    expect_move(start, "e2e4", "e2e4", "start: UCI e2e4");
    expect_move(start, " e4 ", "e2e4", "start: surrounding spaces ignored");

    expect_move(kiwipete, "O-O", "e1g1", "kiwipete: O-O");
    expect_move(kiwipete, "O-O-O", "e1c1", "kiwipete: O-O-O");
    expect_move(kiwipete, "0-0", "e1g1", "kiwipete: 0-0");
    expect_move(kiwipete, "oo", "e1g1", "kiwipete: oo");

    const std::string promotion = "8/P6k/8/8/8/8/8/K7 w - - 0 1";
    expect_move(promotion, "a8=Q", "a7a8q", "promotion: a8=Q");
    expect_move(promotion, "a8q", "a7a8q", "promotion: a8q");
    expect_move(promotion, "a8=Q+", "a7a8q", "promotion: a8=Q+");
    expect_move(promotion, "a8=N", "a7a8n", "promotion: a8=N");
    expect_move(promotion, "a7a8r", "a7a8r", "promotion: UCI a7a8r");

    const std::string capture_promotion = "1n5k/P7/8/8/8/8/8/K7 w - - 0 1";
    expect_move(capture_promotion, "axb8=Q", "a7b8q", "capture-promotion: axb8=Q");

    const std::string en_passant = "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1";
    expect_move(en_passant, "exd6", "e5d6", "en passant: exd6");
    expect_move(en_passant, "exd6 e.p.", "e5d6", "en passant: marker after the move is ignored");

    const std::string knights = "4k3/8/8/8/8/8/8/1N2KN2 w - - 0 1";
    expect_move(knights, "Nbd2", "b1d2", "disambiguation: Nbd2");
    expect_move(knights, "Nfd2", "f1d2", "disambiguation: Nfd2");

    std::printf("\nRejecting what is not a move\n");
    expect_rejected(start, "e5", "start: e5 is not legal");
    expect_rejected(start, "Ke2", "start: Ke2 is blocked", "king");
    expect_rejected(start, "O-O", "start: no castling yet", "castle");
    expect_rejected(start, "xyz", "start: gibberish", "understand");
    expect_rejected(start, "", "start: empty input");
    expect_rejected(start, "e2e5", "start: UCI jump over a piece");
    expect_rejected(promotion, "a8", "promotion: missing piece", "promote");
    expect_rejected(knights, "Nd2", "disambiguation: bare Nd2 is genuinely ambiguous", "ambiguous");
    

    Position pos = position_from(knights);
    Move parsed = null_move();
    std::string error;
    parse_user_move(pos, "Ncd2", parsed, error);
    expect(error.find("ambiguous") != std::string::npos || error.find("knight") != std::string::npos,
           "ambiguous move is explained", error);
}

static void test_san_output() {
    std::printf("\nSAN output\n");

    const std::string mate = "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1";
    Position pos = position_from(mate);
    expect(move_to_san(pos, parse_move("a1a8")) == "Ra8#", "mate is written with '#'",
           move_to_san(pos, parse_move("a1a8")));

    const std::string check = "7k/6P1/8/8/8/8/8/K7 w - - 0 1";
    pos = position_from(check);
    expect(move_to_san(pos, parse_move("g7g8q")) == "g8=Q+", "check is written with '+'",
           move_to_san(pos, parse_move("g7g8q")));

    pos = position_from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    expect(move_to_san(pos, parse_move("e1g1")) == "O-O", "kingside castling",
           move_to_san(pos, parse_move("e1g1")));
    expect(move_to_san(pos, parse_move("e1c1")) == "O-O-O", "queenside castling",
           move_to_san(pos, parse_move("e1c1")));

    pos = position_from("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    expect(move_to_san(pos, parse_move("e5d6")) == "exd6", "en passant capture",
           move_to_san(pos, parse_move("e5d6")));

    pos = position_from("4k3/8/8/8/8/8/8/1N2KN2 w - - 0 1");
    expect(move_to_san(pos, parse_move("b1d2")) == "Nbd2", "disambiguated by file",
           move_to_san(pos, parse_move("b1d2")));
}

// ------------------------------------------------------------ outcomes ----

static void test_outcomes() {
    std::printf("\nGame-end rules\n");

    const std::vector<std::string> no_repetition;

    Position pos = position_from("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    expect(detect_outcome(pos, no_repetition) == Outcome::Checkmate, "checkmate detected");
    expect(describe_outcome(Outcome::Checkmate, pos).find("Black wins") != std::string::npos,
           "checkmate names the winner");

    pos = position_from("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    expect(detect_outcome(pos, no_repetition) == Outcome::Stalemate, "stalemate detected");

    pos = position_from("8/8/8/8/8/8/8/K6k w - - 0 1");
    expect(insufficient_material(pos), "K vs K is insufficient");

    pos = position_from("8/8/8/8/8/8/8/KB5k w - - 0 1");
    expect(insufficient_material(pos), "K+B vs K is insufficient");

    pos = position_from("8/8/8/8/8/8/8/KN5k w - - 0 1");
    expect(insufficient_material(pos), "K+N vs K is insufficient");

    pos = position_from("8/8/8/8/8/8/8/KB3b1k w - - 0 1");
    expect(insufficient_material(pos), "K+B vs K+B, same colour, is insufficient");

    pos = position_from("8/8/8/8/8/8/8/KB4bk w - - 0 1");
    expect(!insufficient_material(pos), "K+B vs K+B, opposite colours, is playable");

    pos = position_from("8/8/8/8/8/8/8/KR5k w - - 0 1");
    expect(!insufficient_material(pos), "K+R vs K is playable");

    pos = position_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 100 60");
    expect(detect_outcome(pos, no_repetition) == Outcome::FiftyMove, "50-move rule detected");

    pos = position_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const std::string key = position_key(pos);
    expect(detect_outcome(pos, {key}) == Outcome::None, "start position is ongoing");
    expect(detect_outcome(pos, {key, key, key}) == Outcome::Threefold,
           "threefold repetition detected");

    Position later = position_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 8 5");
    expect(position_key(later) == key, "repetition key ignores the move clocks");
}

// ----------------------------------------------------------------- main ----

int main() {
    std::printf("rules test\n==========\n");

    test_san_round_trip();
    test_san_input();
    test_san_output();
    test_outcomes();

    if (failures == 0) {
        std::printf("\n%d/%d checks passed\n", checks, checks);
        return 0;
    }
    std::printf("\n%d of %d checks FAILED\n", failures, checks);
    return 1;
}
