<div align="center">

# ♟️ Binary Chess Engine

### A Simplistic Chess Engine Built from Scratch in C++

[![C++](https://img.shields.io/badge/Language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue.svg?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey?style=for-the-badge)]()
[![UCI](https://img.shields.io/badge/Protocol-UCI-green?style=for-the-badge)](http://wbec-ridderkerk.nl/html/UCIProtocol.html)

---

**A from-scratch chess engine implementing bitboard representation, alpha-beta search with iterative deepening, piece-square table evaluation, and the UCI protocol. It generates provably correct moves (verified with perft) and plugs into standard chess GUIs.**

> **Play it in one command:** `play.bat` on Windows (type `e4`, the engine answers).
> **Test it in one command:** `run_tests.bat` on Windows, or `make test` on Linux/macOS.

</div>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Architecture](#-architecture)
- [Core Algorithms](#-core-algorithms)
- [Technical Implementation](#-technical-implementation)
- [Performance](#-performance)
- [Build & Run](#-build--run)
- [Testing](#-testing)
- [How to Play](#-how-to-play)
- [Project Structure](#-project-structure)
- [Key Concepts](#-key-concepts)
- [Future Enhancements](#-future-enhancements)
- [References](#-references)

---

## 🔍 Overview

This chess engine was built as a deep dive into **game theory, search algorithms, and low-level optimization**. Every component — from board representation to move generation to the search algorithm — was implemented from first principles.

### Key Features

| Feature | Description |
|---------|-------------|
| **Bitboard Representation** | 64-bit integers for O(1) square queries |
| **Correct Move Generation** | All legal moves including castling, en passant and promotion, verified by perft |
| **Alpha-Beta Pruning** | Search optimization that eliminates unnecessary branches |
| **Iterative Deepening** | Always has a best move ready, even under time pressure |
| **Piece-Square Tables** | Positional evaluation based on piece placement |
| **UCI Protocol** | Compatible with Arena, Cute Chess, and other GUIs |
| **Play Mode** | `play.bat`: type `e4`, `Nf3` or `O-O` and the engine replies |
| **UCI Time Control** | `wtime` / `btime` / `winc` / `binc` / `movestogo`, plus `movetime`, `depth`, `nodes`, `infinite` and `stop`; scores are from the side to move |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      CHESS ENGINE                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │              │    │              │    │              │  │
│  │   Board      │◄───│   Move       │◄───│   Search     │  │
│  │   Represent. │    │   Generation │    │   Algorithm  │  │
│  │              │    │              │    │              │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│         │                   │                   │           │
│         ▼                   ▼                   ▼           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │              │    │              │    │              │  │
│  │   Bitboard   │    │   Attack     │    │   Alpha-Beta │  │
│  │   Operations │    │   Tables     │    │   + pruning  │  │
│  │              │    │              │    │              │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │              │    │              │    │              │  │
│  │   Eval       │    │   UCI        │    │   Terminal   │  │
│  │   Function   │    │   Protocol   │    │   Interface  │  │
│  │              │    │              │    │              │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

```
Position Input → Move Generation → Search (Alpha-Beta) → Evaluation → Best Move
       │                │                  │                  │            │
       ▼                ▼                  ▼                  ▼            ▼
   Bitboard       Legal moves       Pruned tree        Scored pos    UCI output
```

---

## 🧠 Core Algorithms

### 1. Bitboard Representation

The chess board is represented using **12 bitboards** (6 piece types × 2 colors), where each bit in a 64-bit integer corresponds to a square:

```
Bitboard Layout:

63 62 61 60 59 58 57 56     a8 b8 c8 d8 e8 f8 g8 h8
55 54 53 52 51 50 49 48     a7 b7 c7 d7 e7 f7 g7 h7
47 46 45 44 43 42 41 40     a6 b6 c6 d6 e6 f6 g6 h6
39 38 37 36 35 34 33 32     a5 b5 c5 d5 e5 f5 g5 h5
31 30 29 28 27 26 25 24     a4 b4 c4 d4 e4 f4 g4 h4
23 22 21 20 19 18 17 16     a3 b3 c3 d3 e3 f3 g3 h3
15 14 13 12 11 10  9  8     a2 b2 c2 d2 e2 f2 g2 h2
 7  6  5  4  3  2  1  0     a1 b1 c1 d1 e1 f1 g1 h1
```

**Operations:**
```cpp
// Set bit (place piece)
bitboard |= (1ULL << square);

// Clear bit (remove piece)
bitboard &= ~(1ULL << square);

// Check if bit is set
bool occupied = bitboard & (1ULL << square);

// Count pieces (population count)
int count = __builtin_popcountll(bitboard);

// Find least significant bit
int square = __builtin_ctzll(bitboard);
```

### 2. Move Generation

Each piece type has specialized move generation:

| Piece | Method | Complexity |
|-------|--------|------------|
| **Knight** | Eight fixed offsets, filtered by rank/file delta | O(1) |
| **Bishop** | Sliding ray attack | O(7) |
| **Rook** | Sliding ray attack | O(7) |
| **Queen** | Bishop + Rook combined | O(14) |
| **King** | Adjacent squares | O(1) |
| **Pawn** | Directional + capture patterns | O(1) |

The attack functions are plain functions computed on demand. They are not
precomputed tables yet — that is the first item in
[Future Enhancements](#-future-enhancements).

**Pseudo-legal → Legal:**
```
Generate pseudo-legal moves
  → Make each move on a copy
    → Check if king is in check
      → If not, move is legal
```

### 3. Alpha-Beta Pruning

```
Without pruning:     With pruning:

        A                     A
       /|\                   /|\
      B C D                 B C D
     /|\/|\                /|  |\
    E F G H I             E F  G H

~700M positions        ~50M positions
(depth 6, 30 moves)   (same depth, same moves)
```

**How it works:**
- **Alpha**: Best score MAX player can guarantee (worst case)
- **Beta**: Best score MIN player can guarantee (worst case)
- **Cutoff**: If beta ≤ alpha, prune remaining moves

### 4. Iterative Deepening

```
Depth 1: Search 1 move ahead → 0.001s
Depth 2: Search 2 moves ahead → 0.01s
Depth 3: Search 3 moves ahead → 0.1s
Depth 4: Search 4 moves ahead → 1.0s
Depth 5: Search 5 moves ahead → 10.0s
        ↓
    Time limit reached
        ↓
    Return best move from last completed depth
```

**Benefits:**
- Always has a move ready
- Each depth improves the previous best
- Can stop at any time

### 5. Evaluation Function

```
Position Score = Material + Piece-Square Tables

Material:
  Pawn = 100 centipawns
  Knight = 320
  Bishop = 330
  Rook = 500
  Queen = 900
  King = 20000

Piece-Square Tables:
  +50 for center pawns
  -50 for corner knights
  +20 for active bishops
  ... etc.
```

---

## 💻 Technical Implementation

### Bitboard Attack Generation

```cpp
// Knight attacks using L-shaped offsets
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
```

### Sliding Piece Attacks

```cpp
// Bishop attacks (diagonal sliding)
uint64_t bishop_attacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int directions[] = {9, 7, -7, -9};   // the four diagonals

    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            if(copy < 0 || copy >= 64) break;
            // A diagonal step changes rank and file by the same amount; this
            // guard is what stops a ray wrapping around the board edge.
            if(abs((copy/8) - (square/8)) != abs((copy%8) - (square%8))) break;
            attacks |= (1ULL << copy);
            if(occupied & (1ULL << copy)) break;  // Blocked by piece
        }
    }

    return attacks;
}
```

### Alpha-Beta Search

```cpp
int alphabeta(Position& pos, int depth, int alpha, int beta, bool maximizing) {
    if(depth == 0) return evaluate(pos);
    
    std::vector<Move> moves = generate_moves(pos);
    order_moves(pos, moves);  // Improve pruning
    
    if(maximizing) {
        int max_score = -INF;
        for(const Move& move : moves) {
            Position new_pos = pos;
            make_move(new_pos, move);
            int score = alphabeta(new_pos, depth - 1, alpha, beta, false);
            max_score = std::max(max_score, score);
            alpha = std::max(alpha, score);
            if(beta <= alpha) break;  // Beta cutoff
        }
        return max_score;
    } else {
        // Similar for minimizing player...
    }
}
```

---

## 📊 Performance

| Metric | Measured value |
|--------|----------------|
| **Move generation** | ~9.5M perft nodes/second (perft(5) from the start: 4,865,609 nodes in 0.51 s) |
| **Search Speed** | ~265k nodes/second |
| **Search Depth** | 6 ply in ~3 s; deeper given more time |
| **Lines of Code** | ~1,800 in `src/` (plus tests and tooling) |
| **Memory Usage** | Well under 50 MB (no transposition table yet) |

These are measured on a single desktop core, not estimates. Search speed is
deliberately modest: every candidate move copies the whole position, which is
the first thing on the [improvement list](#-future-enhancements).

### Benchmark Results

```
Position: Starting position
Time: 3 seconds
Depth reached: 6 ply
Nodes searched: ~812,000
Best move: Nf3 (Knight to f3)
```

---

## 🚀 Build & Run

### Prerequisites

- A C++17 compiler (GCC or Clang; the code uses `__builtin_ctzll`, so MSVC will not compile it as-is)
- `make` (optional — the batch scripts and the one-line command below work without it)

### Build

**Windows**

```bat
build.bat
```

**Linux / macOS / MSYS2**

```bash
make            # produces build/chess-engine, build/perft, build/rules and build/searchtest
```

**Without make, any platform**

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o chess-engine \
    src/main.cpp src/board.cpp src/movegen.cpp src/eval.cpp src/search.cpp \
    src/notation.cpp src/gamestate.cpp src/play.cpp
```

`CMakeLists.txt` is also provided if you prefer CMake.

### Running

```bash
./build/chess-engine play          # play a game, you are White
./build/chess-engine play black    # play a game as Black
./build/chess-engine terminal      # scriptable REPL (UCI moves, no auto-reply)
./build/chess-engine               # UCI mode, for chess GUIs
```

---

## ✅ Testing

The engine is validated with **perft**, the standard move-generation test: it
counts the nodes in the move tree at a given depth and compares them with the
published reference values. If a single move is wrong, the counts diverge.

### Quick start

**Windows**

```bat
build.bat          :: 1. compile into build\chess-engine.exe
run_tests.bat      :: 2. build and run the perft + rules + search suites
```

**Linux / macOS / MSYS2**

```bash
make test          # builds everything, then runs all three suites
```

That is all. The suite runs six standard positions to depth 4–5 (31 checks, about
2 seconds), then the notation/rules tests (60 checks), then the search tests
(15 checks), and prints `PASS` or `FAIL` for each, exiting non-zero on failure.

```
startpos
  [PASS] depth 1          20       0.0 ms
  ...
  [PASS] depth 5     4865609     509.6 ms
kiwipete
  [PASS] depth 4     4085603     418.9 ms
...
31/31 checks passed in 1.60 s

rules test
==========
  [PASS] rnbqkbnr/pppppppp/8/8/8/8/PPPPPP...  20 moves
  ...
60/60 checks passed

search test
===========
  [PASS] en passant is a capture
  [PASS] mate in one is found at depth 1
  ...
15/15 checks passed
```

Pass a depth to cap the run when you want a quick check:

```bash
./build/perft 3        # or: run_tests.bat 3
```

### Extra tests

| Test | Command | What it proves |
|------|---------|----------------|
| Perft suite | `make test` | Move generation matches the reference counts for all six standard positions |
| Notation & rules | `make test` | Every legal move survives SAN round-trip; castling, promotion, en passant, mate/stalemate and the draw rules are parsed and reported correctly |
| Search | `make test` | Quiescence finds en passant and quiet promotions, sees mates at the horizon, and honours depth, node and time limits |
| UCI regression | `python tests/regression.py ./build/chess-engine` | Illegal castling, illegal promotion, malformed FEN and terminal edge cases are rejected; `movetime`/`depth`/`nodes`/`infinite`+`stop` are honoured and scores are side-to-move relative |
| Self-play | `pip install python-chess` then `python tests/selfplay.py ./build/chess-engine` | The engine only ever plays legal moves, with `python-chess` as referee |

CI runs the build, all three test suites, the UCI regression probes, a UCI smoke
test, a play-mode smoke test and a self-play game on every push and pull request
(`.github/workflows/ci.yml`).

---

## 🎮 How to Play

### Option 1: Play in the terminal (Recommended)

Double-click **`play.bat`**, or run:

```bat
play.bat            :: you play White
play.bat black      :: you play Black
```

You type moves the way they are written in books — `e4`, `Nf3`, `exd5`, `O-O`,
`e8=Q` — and the engine answers immediately. Coordinates (`e2e4`) work too.

```
  a b c d e f g h
  +----------------+
8 | r n b q k b n r |
7 | p p p p p p p p |
6 | . . . . . . . . |
5 | . . . . . . . . |
4 | . . . . . . . . |
3 | . . . . . . . . |
2 | P P P P P P P P |
1 | R N B Q K B N R |
  +----------------+
  a b c d e f g h

White to move (you)  -  level 2 (normal)
your move> e4
You played e4
...
Black to move  -  level 2 (normal)
Engine plays Nc6   (901 ms, depth 6)
```

Type `help` at any point:

| Command | What it does |
|---------|--------------|
| `moves` | list your legal moves in algebraic notation |
| `hint` | the engine suggests a move without playing it |
| `undo` | take back your move and the engine's reply |
| `level 1` / `2` / `3` | fast (0.25 s), normal (0.9 s), strong (2.5 s) per move |
| `new` / `new black` | start again, as White or Black |
| `eval`, `fen`, `board` | who is better, the FEN, redraw |
| `resign`, `quit` | leave the game |

Checkmate, stalemate, the 50-move rule, threefold repetition and insufficient
material are all detected and reported.

### Option 2: Arena GUI

1. **Download Arena**: [playwitharena.de](http://www.playwitharena.de)
2. **Open Arena** → Engines → Install New Engine
3. **Select** `chess-engine.exe`
4. **Choose protocol**: UCI
5. **Play!**

### Option 3: Scriptable terminal mode

For piping moves in from a script (no automatic replies):

```bash
./build/chess-engine terminal

=== CHESS ENGINE ===
Commands:
  <move>     - Make a move (e.g., e2e4)
  think       - Engine thinks and makes a move
  print       - Print the board
  moves       - Show all legal moves
  quit        - Exit
```

### Option 4: Lichess Bot (Online Play)

Play against anyone online at https://lichess.org/@/aniemite

**Setup (one-time):**

```bash
# 1. Clone lichess-bot
git clone https://github.com/lichess-bot-devs/lichess-bot
cd lichess-bot
pip install -r requirements.txt

# 2. Create config.yml with your token
# 3. Run
python lichess-bot.py
```

**Detailed Steps:**

1. Create a bot account at https://lichess.org/signup
2. Generate API token at https://lichess.org/account/oauth/token/create
   - Select: `bot:play`, `challenge:read`, `challenge:write`
3. Create `config.yml`:
   ```yaml
   token: "YOUR_LICHESS_TOKEN"
   url: "https://lichess.org/"

   engine:
     dir: "path/to/chess-engine/"
     name: "chess-engine.exe"
     protocol: "uci"
     ponder: false

   challenge:
     concurrency: 1
     sort_by: "best"
     accept_bot: true
     accept_human: true
     variants:
       - standard
     time_modes:
       - real
       - correspondence
       - unlimited
   ```
4. Run: `python lichess-bot.py`

**Bot Profile:** https://lichess.org/@/aniemite

---

## 📁 Project Structure

```
chess-engine/
├── src/
│   ├── types.h           # Core types (Piece, Color, Square, Move)
│   ├── board.h/cpp       # Board representation + FEN parsing
│   ├── movegen.h/cpp     # Move generation + legality checks
│   ├── eval.h/cpp        # Evaluation function + PSTs
│   ├── search.h/cpp      # Alpha-beta + iterative deepening
│   ├── notation.h/cpp    # SAN output + forgiving move parsing
│   ├── gamestate.h/cpp   # Draw rules + game-end detection
│   ├── play.h/cpp        # Interactive play mode
│   └── main.cpp          # Mode dispatch: UCI, play, terminal
├── tests/
│   ├── perft.cpp         # Perft suite (six standard positions)
│   ├── rules.cpp         # Notation + game-end rule checks
│   ├── search.cpp        # Search limits, horizon and quiescence checks
│   ├── regression.py     # UCI/illegal-input regression checks
│   └── selfplay.py       # Self-play legality test (python-chess referee)
├── .github/workflows/
│   └── ci.yml            # Build + all suites + UCI/play smoke + self-play on every push
├── Makefile              # make / make test
├── build.bat             # one-command Windows build
├── play.bat              # build (if needed) and play a game
├── run_tests.bat         # one-command Windows test
├── CMakeLists.txt        # Alternative CMake build
├── README.md             # This file
└── LICENSE               # GNU GPL v3
```

---

## 📚 Key Concepts

### Chess Programming Fundamentals

| Concept | Description |
|---------|-------------|
| **Bitboard** | 64-bit integer representing piece positions |
| **Mailbox** | Alternative: 64-element array |
| **Pseudo-legal** | Moves that might leave king in check |
| **Legal** | Moves that don't leave king in check |
| **Quiescence** | Searching capture sequences to avoid horizon effect |
| **Transposition** | Same position reached by different move orders |
| **Killer Move** | Heuristic: prioritize moves that caused cutoffs |
| **MVV-LVA** | Most Valuable Victim - Least Valuable Attacker |

### Algorithms Used

| Algorithm | Purpose |
|-----------|---------|
| **Minimax** | Game tree search |
| **Alpha-Beta** | Prune ineffective branches |
| **Iterative Deepening** | Time management |
| **Quiescence Search** | Avoid horizon effect |
| **Move Ordering** | Improve pruning efficiency |

---

## 🔮 Future Enhancements

- [ ] **Transposition Table** - Cache positions for faster search
- [ ] **NNUE Evaluation** - Neural network for stronger evaluation
- [ ] **Opening Book** - Database of known opening moves
- [ ] **Endgame Tablebases** - Perfect endgame play
- [ ] **Player Rating System** - Analyze and rate player performance
- [ ] **Multi-threading** - Parallel search for faster analysis
- [ ] **Pondering** - Think during opponent's time

---

## 📖 References

- [Chess Programming Wiki](https://www.chessprogramming.org/) - Comprehensive chess programming resource
- [Sunfish](https://github.com/thomasahle/sunfish) - Minimal Python engine (inspiration)
- [Stockfish](https://github.com/official-stockfish/Stockfish) - World's strongest open-source engine
- [Lichess Bot](https://github.com/lichess-bot-devs/lichess-bot) - Online play integration
- [UCI Protocol](http://wbec-ridderkerk.nl/html/UCIProtocol.html) - Universal Chess Interface specification

---

## 📄 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

---

<div align="center">

**Built with ❤️ and C++**

*"The chessboard is the world, the pieces are the phenomena of the universe."*

</div>
