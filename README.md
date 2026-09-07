# Chess Engine

A custom chess engine built from scratch in C++. Plays legal chess, evaluates positions, searches for the best move, and connects to online platforms.

---

## What is a Chess Engine?

A chess engine is a program that:
1. Takes a chess position as input
2. Calculates the best move as output

It's the "brain" behind chess GUIs. The engine does the thinking; the GUI displays the board.

```
Input:  "White to move, pieces at these positions"
Output: "e2e4"
```

---

## Core Concepts

### 1. Board Representation (Bitboards)

The chess board has 64 squares. We represent it using a 64-bit integer where each bit represents one square.

```
Bit = 1 → piece is on that square
Bit = 0 → square is empty

Example: White pawns on rank 2
00000000 00000000 00000000 00000000
00000000 00000000 11111111 00000000
```

We use 12 bitboards (6 piece types × 2 colors):
- White: pawns, knights, bishops, rooks, queens, king
- Black: pawns, knights, bishops, rooks, queens, king

**Why bitboards?**
- Single instruction to check if a square is occupied
- Fast parallel operations
- Used by professional engines (Stockfish, etc.)

### 2. Move Generation

For each piece, generate all legal moves:

| Piece | Movement |
|-------|----------|
| Knight | L-shape jumps (2+1 squares) |
| Bishop | Diagonal lines |
| Rook | Straight lines (horizontal/vertical) |
| Queen | Bishop + Rook combined |
| King | One square in any direction |
| Pawn | Forward 1 (or 2 from start), capture diagonally |

Special moves:
- Castling (king-side and queen-side)
- En passant (pawn capture)
- Pawn promotion (pawn reaches last rank)

### 3. Search Algorithm

The engine searches through possible future positions to find the best move.

**Minimax Algorithm:**
- Assume both players play optimally
- Maximize your score, minimize opponent's score

**Alpha-Beta Pruning:**
- Skip branches that can't affect the result
- 2x+ speedup over basic minimax

**Iterative Deepening:**
- Search depth 1, then 2, then 3...
- Stop when time runs out
- Always has a best move ready

**Transposition Table:**
- Cache positions you've already evaluated
- Avoid re-searching the same position

### 4. Evaluation Function

Score a position (positive = white advantage, negative = black advantage).

**Material:**
| Piece | Value (centipawns) |
|-------|-------------------|
| Pawn | 100 |
| Knight | 320 |
| Bishop | 330 |
| Rook | 500 |
| Queen | 900 |
| King | 20000 |

**Piece-Square Tables:**
- Each piece gets bonus/penalty based on square
- Example: Knight on center square = bonus, corner = penalty

**Positional Factors:**
- King safety
- Pawn structure (doubled, isolated, passed pawns)
- Mobility (number of legal moves)
- Center control

### 5. UCI Protocol

Universal Chess Interface - standard way engines talk to GUIs.

```
GUI → Engine:  uci
Engine → GUI:  id name MyEngine
               uciok

GUI → Engine:  position startpos moves e2e4 e7e5
GUI → Engine:  go wtime 60000 btime 60000
Engine → GUI:  info depth 6 score cp 45 pv e2e4
               bestmove e2e4
```

---

## Features

### Core
- Full chess rules (castling, en passant, promotion, draws)
- Alpha-beta search with iterative deepening
- Transposition table for caching
- Piece-square table evaluation
- UCI protocol support

### Player Rating System
- Tracks player moves vs engine best moves
- Classifies errors: blunders, mistakes, inaccuracies
- Calculates accuracy percentage
- Estimates player rating
- Generates detailed game report

### Online Play (Lichess Bot)
- Connects to Lichess as a bot
- Plays against real people online
- Automatic move transmission

---

## Project Structure

```
chess-engine/
├── src/
│   ├── main.cpp          # Entry point + UCI loop
│   ├── types.h           # Common types (Move, Position)
│   ├── board.h/cpp       # Board representation
│   ├── movegen.h/cpp     # Move generation
│   ├── search.h/cpp      # Search algorithm
│   ├── eval.h/cpp        # Evaluation function
│   └── uci.h/cpp         # UCI protocol
├── CMakeLists.txt        # Build system
└── README.md
```

---

## Build & Run

```bash
mkdir build && cd build
cmake ..
make
./chess-engine  # Runs in UCI mode
```

---

## How to Play

### Option 1: Arena GUI (Recommended for testing)
1. Download Arena (playwitharena.de)
2. Engines → Install New Engine → select our executable
3. Choose protocol: UCI
4. Play!

### Option 2: Lichess Bot (Online play)
1. Create Lichess bot account
2. Get API token
3. Run lichess-bot connector
4. People can challenge your bot online

---

## Timeline

| Phase | Task | Duration |
|-------|------|----------|
| 1 | Foundation (bitboards, move generation) | 5 days |
| 2 | Search (alpha-beta, transposition table) | 4 days |
| 3 | Evaluation (material, PST, positional) | 4 days |
| 4 | UCI Protocol | 3 days |
| 5 | Player Rating System | 3 days |
| 6 | Lichess Bot | 3 days |
| 7 | Polish & Testing | 2 days |

---

## References

- [Chess Programming Wiki](https://www.chessprogramming.org/)
- [Sunfish Engine](https://github.com/thomasahle/sunfish) - Simple Python engine (inspiration)
- [Stockfish](https://github.com/official-stockfish/Stockfish) - Professional engine (reference)
- [Lichess Bot](https://github.com/lichess-bot-devs/lichess-bot) - Online play connector
