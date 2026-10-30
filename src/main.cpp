#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"

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

void uci_loop() {
    Position pos;
    init_position(pos);  // so commands work even before the first `position`

    Searcher searcher;
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
            init_position(pos);
        }
        else if(command == "setoption") {
            // Ignore options for now
        }
        else if(command == "stop") {
            // Stop search - ignore for now
        }
        else if(command == "position") {
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
            // Parse time controls (simplified)
            int wtime = 60000, btime = 60000;
            int winc = 0, binc = 0;
            std::string token;
            
            while(iss >> token) {
                if(token == "wtime") iss >> wtime;
                else if(token == "btime") iss >> btime;
                else if(token == "winc") iss >> winc;
                else if(token == "binc") iss >> binc;
            }
            
            // Calculate time for this move
            int time_left = (pos.side_to_move == WHITE) ? wtime : btime;
            int inc = (pos.side_to_move == WHITE) ? winc : binc;
            int time_for_move = time_left / 30 + inc / 2;
            time_for_move = std::max(time_for_move, 100);
            time_for_move = std::min(time_for_move, time_left / 2);
            
            Move best = searcher.search(pos, time_for_move);
            std::cout << "bestmove " << move_to_string(best) << std::endl;
            std::cout.flush();
        }
        else if(command == "quit") {
            break;
        }
        else if(command == "print") {
            print_board(pos);
            std::cout << "FEN: " << to_fen(pos) << std::endl;
            std::cout << "Eval: " << evaluate(pos) << " cp" << std::endl;
        }
    }
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
    // Check for terminal mode via command line
    bool terminal = false;
    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "terminal" || arg == "--terminal") {
            terminal = true;
            break;
        }
    }
    
    if(terminal) {
        terminal_mode();
    } else {
        // Default: UCI mode (Arena, Cute Chess, etc.)
        uci_loop();
    }
    
    return 0;
}
