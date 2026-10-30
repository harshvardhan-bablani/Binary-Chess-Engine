#include "play.h"

#include "board.h"
#include "eval.h"
#include "gamestate.h"
#include "movegen.h"
#include "notation.h"
#include "search.h"
#include "types.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
// PLAY MODE
// ============================================================
//
// A thin, friendly layer over the same board/movegen/search code the UCI mode
// uses. It exists so that a person can sit down and play without knowing what
// UCI is: type "e4", the engine answers with a move in algebraic notation.

namespace {

const char* const kHelp =
    "Commands\n"
    "  <move>        e4, Nf3, exd5, O-O, e8=Q   (e2e4 also works)\n"
    "  moves         list your legal moves\n"
    "  hint          ask the engine what it would play\n"
    "  undo          take back your last move and the engine's reply\n"
    "  level 1|2|3   engine speed: fast, normal, strong\n"
    "  new [black]   start a new game, optionally as Black\n"
    "  board         redraw the board\n"
    "  fen           show the position as a FEN string\n"
    "  eval          show who is better, in pawns\n"
    "  resign        give up this game\n"
    "  quit          leave\n";

const int kLevelMs[3] = {250, 900, 2500};
const char* const kLevelName[3] = {"fast", "normal", "strong"};

std::string lowercase(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

std::string trim(const std::string& text) {
    std::string out = text;
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.front())) != 0) {
        out.erase(0, 1);
    }
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back())) != 0) {
        out.pop_back();
    }
    return out;
}

// Centipawns as a readable score, from the point of view of the side to move.
std::string score_text(int centipawns) {
    if (centipawns > 90000) return "mate";
    if (centipawns < -90000) return "being mated";
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%+.2f", static_cast<double>(centipawns) / 100.0);
    return buffer;
}

void print_move_list(const Position& pos) {
    const std::vector<std::string> moves = legal_moves_san(pos);
    if (moves.empty()) {
        std::cout << "  (no legal moves)\n";
        return;
    }
    std::string line;
    for (const std::string& san : moves) {
        if (!line.empty() && line.size() + san.size() + 1 > 70u) {
            std::cout << "  " << line << "\n";
            line.clear();
        }
        if (!line.empty()) line += ' ';
        line += san;
    }
    if (!line.empty()) std::cout << "  " << line << "\n";
}

}  // namespace

// ============================================================
// THE LOOP
// ============================================================

void play_mode(bool human_is_white) {
    Position pos;
    init_position(pos);

    Searcher searcher;
    searcher.set_silent(true);

    int human_color = human_is_white ? WHITE : BLACK;
    int level = 2;

    std::vector<Position> history;
    std::vector<std::string> keys;
    keys.push_back(position_key(pos));

    bool over = false;
    std::string over_text;
    bool redraw = true;  // print the board only when something changed

    std::cout << "\n  Binary Chess Engine\n"
              << "  -------------------\n"
              << "  You are " << (human_is_white ? "White" : "Black")
              << ". Type a move such as e4, Nf3 or O-O.\n"
              << "  Type 'help' for commands, 'quit' to leave.\n\n";

    while (true) {
        if (!over) {
            const Outcome outcome = detect_outcome(pos, keys);
            if (outcome != Outcome::None) {
                over = true;
                over_text = describe_outcome(outcome, pos);
                redraw = true;
            }
        }

        if (redraw) {
            print_board(pos);
            redraw = false;

            if (over) {
                std::cout << over_text << "\n";
                std::cout << "Type 'new' for another game or 'quit' to leave.\n";
            } else {
                std::cout << (pos.side_to_move == WHITE ? "White" : "Black") << " to move";
                if (pos.side_to_move == human_color) {
                    std::cout << " (you)";
                }
                std::cout << "  -  level " << level << " (" << kLevelName[level - 1] << ")\n";
            }
        }

        // The engine plays its own turn without being asked.
        if (!over && pos.side_to_move != human_color) {
            const auto start = std::chrono::steady_clock::now();
            const Move best = searcher.search(pos, kLevelMs[level - 1]);
            const long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now() - start).count();

            if (is_null_move(best)) {
                std::cout << "The engine has no move.\n";
                continue;  // the outcome check at the top of the loop reports it
            }

            const std::string san = move_to_san(pos, best);
            history.push_back(pos);
            make_move(pos, best);
            keys.push_back(position_key(pos));

            std::cout << "Engine plays " << san << "   (" << elapsed << " ms, depth "
                      << searcher.get_depth() << ")\n\n";
            redraw = true;
            continue;
        }

        std::cout << (over ? "> " : "your move> ");
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n";
            return;
        }
        while (!input.empty() && (input.back() == '\r' || input.back() == '\n')) {
            input.pop_back();
        }
        const std::string line = trim(input);
        if (line.empty()) continue;

        const std::string command = lowercase(line);

        if (command == "quit" || command == "exit") {
            return;
        }
        if (command == "help" || command == "h" || command == "?") {
            std::cout << kHelp << "\n";
            continue;
        }
        if (command == "board" || command == "print") {
            redraw = true;
            continue;
        }
        if (command == "moves" || command == "legal") {
            print_move_list(pos);
            std::cout << "\n";
            continue;
        }
        if (command == "fen") {
            std::cout << to_fen(pos) << "\n\n";
            continue;
        }
        if (command == "eval") {
            int score = evaluate(pos);
            if (pos.side_to_move == BLACK) score = -score;
            std::cout << "Evaluation: " << score_text(score) << " for the side to move\n\n";
            continue;
        }
        if (command == "hint") {
            const Move suggestion = searcher.search(pos, 400);
            if (is_null_move(suggestion)) {
                std::cout << "No move to suggest.\n\n";
            } else {
                std::cout << "Try: " << move_to_san(pos, suggestion) << "\n\n";
            }
            continue;
        }
        if (command == "undo") {
            int taken = 0;
            while (!history.empty() && taken < 2) {
                pos = history.back();
                history.pop_back();
                if (keys.size() > 1u) keys.pop_back();
                ++taken;
                if (pos.side_to_move == human_color) break;
            }
            if (taken == 0) {
                std::cout << "Nothing to undo.\n\n";
            } else {
                over = false;
                over_text.clear();
                redraw = true;
                std::cout << "Move taken back.\n\n";
            }
            continue;
        }
        if (command == "resign") {
            if (!over) {
                over = true;
                over_text = "You resigned. The engine wins.";
            }
            redraw = true;
            std::cout << "\n";
            continue;
        }
        if (command == "new" || command.compare(0, 4, "new ") == 0) {
            const std::string argument = (command.size() > 4) ? command.substr(4) : "";
            if (argument == "black" || argument == "b") {
                human_color = BLACK;
            } else if (argument == "white" || argument == "w" || argument.empty()) {
                human_color = WHITE;
            } else {
                std::cout << "Usage: new [white|black]\n\n";
                continue;
            }
            init_position(pos);
            history.clear();
            keys.assign(1, position_key(pos));
            over = false;
            over_text.clear();
            redraw = true;
            std::cout << "New game. You are "
                      << (human_color == WHITE ? "White" : "Black") << ".\n\n";
            continue;
        }
        if (command == "level" || command.compare(0, 6, "level ") == 0) {
            const std::string argument = (command.size() > 6) ? trim(command.substr(6)) : "";
            if (argument == "1" || argument == "2" || argument == "3") {
                level = argument[0] - '0';
                std::cout << "Engine level " << level << " (" << kLevelName[level - 1] << ")\n\n";
            } else {
                std::cout << "Usage: level 1 (fast), 2 (normal) or 3 (strong)\n\n";
            }
            continue;
        }

        if (over) {
            std::cout << "The game is over. Type 'new' or 'quit'.\n\n";
            continue;
        }

        Move move = null_move();
        std::string error;
        if (!parse_user_move(pos, line, move, error)) {
            std::cout << "  " << error << "\n\n";
            continue;
        }

        const std::string san = move_to_san(pos, move);
        history.push_back(pos);
        make_move(pos, move);
        keys.push_back(position_key(pos));
        redraw = true;
        std::cout << "You played " << san << "\n\n";
    }
}
