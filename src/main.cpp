#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <thread>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"
#include "play.h"

// ============================================================
// UCI PROTOCOL
// ============================================================

// Apply one UCI move token to `pos`.
//
// Moves arriving from a GUI are untrusted text: they can be mistyped, refer to
// a different position, or be outright illegal. make_move() assumes a legal
// move, so we check against the legal move list first. This is what stops
// input like `e1g1` at the start of the game from corrupting the board.
bool apply_uci_move(Position& pos, const std::string& token) {
    const Move move = parse_move(token);

    if(!is_legal_move(pos, move)) {
        std::cout << "info string illegal move: " << token << std::endl;
        std::cout.flush();
        return false;
    }

    return make_move(pos, move);
}

// Translate a UCI `go` line into search limits.
//
// Supported: wtime, btime, winc, binc, movestogo, movetime, depth, nodes and
// infinite. A GUI that sends only `depth` or `nodes` gets exactly that many,
// with no clock; everything else falls back to a share of the remaining time.
SearchLimits parse_go(std::istringstream& iss, const Position& pos) {
    SearchLimits limits;

    int wtime = 60000, btime = 60000;
    int winc = 0, binc = 0;
    int movestogo = 0;
    int movetime = 0;
    bool has_clock = false;

    std::string token;
    while(iss >> token) {
        if(token == "wtime") { iss >> wtime; has_clock = true; }
        else if(token == "btime") { iss >> btime; has_clock = true; }
        else if(token == "winc") iss >> winc;
        else if(token == "binc") iss >> binc;
        else if(token == "movestogo") iss >> movestogo;
        else if(token == "movetime") iss >> movetime;
        else if(token == "depth") iss >> limits.depth;
        else if(token == "nodes") iss >> limits.nodes;
        else if(token == "infinite") limits.infinite = true;
        // `ponder`, `searchmoves` and unknown tokens are ignored.
    }

    if(movetime > 0) {
        limits.time_ms = movetime;
    } else if(limits.infinite) {
        limits.time_ms = 0;  // until stop
    } else if(!has_clock && (limits.depth > 0 || limits.nodes > 0)) {
        limits.time_ms = 0;
    } else {
        const int time_left = std::max(0, (pos.side_to_move == WHITE) ? wtime : btime);
        const int inc = std::max(0, (pos.side_to_move == WHITE) ? winc : binc);
        const int moves_to_go = (movestogo > 0) ? movestogo : 30;

        int budget = time_left / moves_to_go + inc / 2;
        budget = std::max(budget, 1);
        budget = std::min(budget, std::max(1, time_left / 2));
        limits.time_ms = budget;
    }

    return limits;
}

void uci_loop() {
    Position pos;
    init_position(pos);  // so commands work even before the first `position`

    Searcher searcher;
    std::thread worker;

    // Stop a running search and wait for it. Called before changing the
    // position, before starting another search, and on quit/EOF.
    auto finish_search = [&]() {
        searcher.stop();
        if(worker.joinable()) worker.join();
    };

    auto start_search = [&](const SearchLimits& limits) {
        finish_search();
        searcher.clear_stop();

        // The worker gets its own copy of the position, so the UCI thread can
        // keep reading commands while the search runs.
        Position search_pos = pos;
        worker = std::thread([&searcher, search_pos, limits]() mutable {
            const Move best = searcher.search(search_pos, limits);
            std::cout << "bestmove " << move_to_string(best) << std::endl;
            std::cout.flush();
        });
    };

    std::string line;
    
    while(std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if(command == "uci") {
            std::cout << "id name SimpleEngine" << std::endl;
            std::cout << "id author Harshvardhan Bablani" << std::endl;
            std::cout << "uciok" << std::endl;
            std::cout.flush();
        }
        else if(command == "isready") {
            std::cout << "readyok" << std::endl;
            std::cout.flush();
        }
        else if(command == "ucinewgame") {
            finish_search();
            init_position(pos);
        }
        else if(command == "setoption") {
            // Ignore options for now
        }
        else if(command == "stop") {
            finish_search();
        }
        else if(command == "position") {
            finish_search();
            std::string token;
            iss >> token;
            
            if(token == "startpos") {
                init_position(pos);
                
                // Check if there are more tokens (moves)
                std::string next;
                if(iss >> next) {
                    if(next == "moves") {
                        while(iss >> token) {
                            apply_uci_move(pos, token);
                        }
                    }
                }
            } else if(token == "fen") {
                std::string fen;
                while(iss >> token && token != "moves") {
                    fen += token + " ";
                }

                if(!load_fen(pos, fen)) {
                    std::cout << "info string invalid FEN, position unchanged" << std::endl;
                    std::cout.flush();
                }

                while(iss >> token) {
                    apply_uci_move(pos, token);
                }
            }
        }
        else if(command == "go") {
            start_search(parse_go(iss, pos));
        }
        else if(command == "quit") {
            finish_search();
            break;
        }
        else if(command == "print") {
            print_board(pos);
            std::cout << "FEN: " << to_fen(pos) << std::endl;
            std::cout << "Eval: " << evaluate(pos) << " cp" << std::endl;
        }
    }

    finish_search();
}

// ============================================================
// TERMINAL MODE
// ============================================================

void terminal_mode() {
    Position pos;
    Searcher searcher;
    
    init_position(pos);
    
    std::cout << "\n=== CHESS ENGINE ===" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  <move>     - Make a move (e.g., e2e4)" << std::endl;
    std::cout << "  think       - Engine thinks and makes a move" << std::endl;
    std::cout << "  print       - Print the board" << std::endl;
    std::cout << "  fen         - Show FEN" << std::endl;
    std::cout << "  eval        - Show evaluation" << std::endl;
    std::cout << "  moves       - Show all legal moves" << std::endl;
    std::cout << "  undo        - Undo last move" << std::endl;
    std::cout << "  quit        - Exit" << std::endl;
    
    std::vector<Position> history;
    history.push_back(pos);
    
    while(true) {
        print_board(pos);
        std::cout << (pos.side_to_move == WHITE ? "White" : "Black") << " to move" << std::endl;
        std::cout << "> ";
        std::cout.flush();

        std::string input;
        if(!std::getline(std::cin, input)) {
            // stdin was closed (e.g. piped input ran out). Leave cleanly
            // instead of spinning forever on a failed read.
            std::cout << std::endl;
            break;
        }

        // Tolerate CRLF line endings. When input is piped from a Windows
        // shell the line still ends in '\r', so "quit\r" would never match.
        while(!input.empty() && (input.back() == '\r' || input.back() == '\n')) {
            input.pop_back();
        }

        if(input.empty()) {
            continue;
        }

        if(input == "quit") {
            break;
        }
        else if(input == "print") {
            continue;  // Board will be printed at loop start
        }
        else if(input == "fen") {
            std::cout << to_fen(pos) << std::endl;
        }
        else if(input == "eval") {
            std::cout << "Evaluation: " << evaluate(pos) << " cp" << std::endl;
        }
        else if(input == "moves") {
            std::vector<Move> moves = generate_moves(pos);
            std::cout << "Legal moves: ";
            for(const Move& move : moves) {
                std::cout << move_to_string(move) << " ";
            }
            std::cout << std::endl;
        }
        else if(input == "think") {
            std::cout << "Thinking..." << std::endl;
            Move best = searcher.search(pos, 3000);

            if(is_null_move(best)) {
                std::cout << "No legal moves available." << std::endl;
                continue;
            }

            std::cout << "Engine plays: " << move_to_string(best) << std::endl;
            
            history.push_back(pos);
            make_move(pos, best);
        }
        else if(input == "undo") {
            if(history.size() > 1) {
                pos = history.back();
                history.pop_back();
            } else {
                std::cout << "No moves to undo" << std::endl;
            }
        }
        else {
            // Try to parse as a move
            Move move = parse_move(input);
            
            if(!is_legal_move(pos, move)) {
                std::cout << "Illegal move: " << input << std::endl;
                continue;
            }

            history.push_back(pos);
            if(!make_move(pos, move)) {
                // Should be unreachable: is_legal_move already approved it.
                history.pop_back();
                std::cout << "Illegal move: " << input << std::endl;
            }
        }
    }
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {
    // Command-line modes:
    //   chess-engine             UCI (what a GUI or Lichess bot speaks)
    //   chess-engine play        play a game in the terminal, as White
    //   chess-engine play black  play as Black
    //   chess-engine terminal    scriptable REPL (UCI-style moves, no auto-reply)
    bool terminal = false;
    bool play = false;
    bool play_as_white = true;

    for(int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if(arg == "terminal" || arg == "--terminal") {
            terminal = true;
        } else if(arg == "play" || arg == "--play") {
            play = true;
        } else if(arg == "black" || arg == "--black") {
            play_as_white = false;
        } else if(arg == "white" || arg == "--white") {
            play_as_white = true;
        }
    }

    if(play) {
        play_mode(play_as_white);
    } else if(terminal) {
        terminal_mode();
    } else {
        // Default: UCI mode (Arena, Cute Chess, etc.)
        uci_loop();
    }

    return 0;
}
