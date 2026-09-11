#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"

// ============================================================
// UCI PROTOCOL
// ============================================================

void uci_loop() {
    Position pos;
    Searcher searcher;
    std::string line;
    
    while(std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if(command == "uci") {
            std::cout << "id name SimpleEngine" << std::endl;
            std::cout << "id author YourName" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if(command == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if(command == "position") {
            std::string token;
            iss >> token;
            
            if(token == "startpos") {
                init_position(pos);
                
                iss >> token;  // should be "moves"
                while(iss >> token) {
                    Move move = parse_move(token);
                    make_move(pos, move);
                }
            } else if(token == "fen") {
                std::string fen;
                while(iss >> token && token != "moves") {
                    fen += token + " ";
                }
                load_fen(pos, fen);
                
                while(iss >> token) {
                    Move move = parse_move(token);
                    make_move(pos, move);
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
        
        std::string input;
        std::getline(std::cin, input);
        
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
            
            // Check if move is legal
            std::vector<Move> legal_moves = generate_moves(pos);
            bool legal = false;
            for(const Move& m : legal_moves) {
                if(m.from == move.from && m.to == move.to && m.promotion == move.promotion) {
                    legal = true;
                    break;
                }
            }
            
            if(legal) {
                history.push_back(pos);
                if(!make_move(pos, move)) {
                    std::cout << "Illegal move!" << std::endl;
                    pos = history.back();
                    history.pop_back();
                }
            } else {
                std::cout << "Illegal move: " << input << std::endl;
            }
        }
    }
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {
    // Check for UCI mode
    bool uci_mode = false;
    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "uci") {
            uci_mode = true;
            break;
        }
    }
    
    if(uci_mode) {
        uci_loop();
    } else {
        terminal_mode();
    }
    
    return 0;
}
