<div align="center">

# ♟️ Binary Chess Engine

### A Simplistic Chess Engine Built from Scratch in C++

[![C++](https://img.shields.io/badge/Language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue.svg?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey?style=for-the-badge)]()
[![UCI](https://img.shields.io/badge/Protocol-UCI-green?style=for-the-badge)](http://wbec-ridderkerk.nl/html/UCIProtocol.html)

---

**A complete chess engine implementing bitboard representation, alpha-beta search with iterative deepening, piece-square table evaluation, and the UCI protocol. Capable of playing legal chess at an intermediate level (~1500 Elo) and integrating with standard chess GUIs.**

</div>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Architecture](#-architecture)
- [Core Algorithms](#-core-algorithms)
- [Technical Implementation](#-technical-implementation)
- [Performance](#-performance)
- [Build & Run](#-build--run)
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
| **Complete Move Generation** | All legal moves including castling, en passant, promotion |
| **Alpha-Beta Pruning** | Search optimization that eliminates unnecessary branches |
| **Iterative Deepening** | Always has a best move ready, even under time pressure |
| **Piece-Square Tables** | Positional evaluation based on piece placement |
| **UCI Protocol** | Compatible with Arena, Cute Chess, and other GUIs |
| **Time Management** | Flexible time control support |

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
| **Knight** | Precomputed attack table | O(1) |
| **Bishop** | Sliding ray attack | O(7) |
| **Rook** | Sliding ray attack | O(7) |
| **Queen** | Bishop + Rook combined | O(14) |
| **King** | Adjacent squares | O(1) |
| **Pawn** | Directional + capture patterns | O(1) |

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
    int directions[] = {9, 11, -9, -11};
    
    for(int d = 0; d < 4; d++) {
        int copy = square;
        while(true) {
            copy += directions[d];
            if(copy < 0 || copy >= 64) break;
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

| Metric | Value |
|--------|-------|
| **Search Speed** | 100k-500k nodes/second |
| **Search Depth** | 6-10 ply (iterative deepening) |
| **Estimated Strength** | ~1500 Elo (intermediate player) |
| **Lines of Code** | ~1500 |
| **Memory Usage** | < 50MB |

### Benchmark Results

```
Position: Starting position
Time: 5 seconds
Depth reached: 7 ply
Nodes searched: ~2.5 million
Best move: Nf3 (Knight to f3)
```

---

## 🚀 Build & Run

### Prerequisites

- C++ compiler (GCC, Clang, or MSVC)
- C++17 or later

### Compilation

```bash
# Linux/macOS
g++ -std=c++17 -O2 -o chess-engine src/main.cpp src/board.cpp src/movegen.cpp src/eval.cpp src/search.cpp

# Windows (MinGW)
g++ -std=c++17 -O2 -o chess-engine.exe src/main.cpp src/board.cpp src/movegen.cpp src/eval.cpp src/search.cpp
```

### Running

```bash
# Terminal mode (play in terminal)
./chess-engine terminal

# UCI mode (for GUIs)
./chess-engine
```

---

## 🎮 How to Play

### Option 1: Arena GUI (Recommended)

1. **Download Arena**: [playwitharena.de](http://www.playwitharena.de)
2. **Open Arena** → Engines → Install New Engine
3. **Select** `chess-engine.exe`
4. **Choose protocol**: UCI
5. **Play!**

### Option 2: Terminal Mode

```bash
./chess-engine terminal

=== CHESS ENGINE ===
Commands:
  <move>     - Make a move (e.g., e2e4)
  think       - Engine thinks and makes a move
  print       - Print the board
  moves       - Show all legal moves
  quit        - Exit
```

### Option 3: Lichess Bot (Online Play)

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
│   └── main.cpp          # UCI protocol + terminal interface
├── CMakeLists.txt        # Build configuration
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
