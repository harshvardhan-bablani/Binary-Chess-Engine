#!/usr/bin/env python3
"""Self-play legality test.

The engine plays itself through the UCI protocol while python-chess acts as
referee. Every `bestmove` the engine returns must be a legal move in the
position it was given; anything else is reported and the test fails.

This catches the class of bug that perft can miss: a legal move list that is
correct, but a search or UCI layer that plays something else (for example
castling with a rook that no longer exists).

Usage:
    python tests/selfplay.py build/chess-engine.exe
    python tests/selfplay.py build/chess-engine.exe 3 40

Arguments:
    engine       path to the engine binary
    games        number of games to play        (default: 2)
    ms           milliseconds per move budget   (default: 40)

Requires: pip install python-chess
Exit code 0 only if every game finished with only legal moves.
"""

import queue
import subprocess
import sys
import threading

try:
    import chess
except ImportError:
    sys.exit("python-chess is required: pip install python-chess")


class Engine:
    def __init__(self, path):
        self.proc = subprocess.Popen(
            [path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self.lines = queue.Queue()
        threading.Thread(target=self._read, daemon=True).start()

        self.send("uci")
        self.wait_for("uciok", timeout=10)

    def _read(self):
        for line in self.proc.stdout:
            self.lines.put(line.strip())

    def send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def wait_for(self, prefix, timeout=60):
        while True:
            try:
                line = self.lines.get(timeout=timeout)
            except queue.Empty:
                raise TimeoutError(f"no '{prefix}' line within {timeout}s")
            if line.startswith(prefix):
                return line

    def best_move(self, moves, ms):
        position = "position startpos"
        if moves:
            position += " moves " + " ".join(moves)
        self.send(position)
        self.send(f"go wtime {ms * 40} btime {ms * 40}")
        line = self.wait_for("bestmove")
        return line.split()[1]

    def close(self):
        try:
            self.send("quit")
        except Exception:
            pass
        try:
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()


def play_game(engine_path, ms, max_plies):
    """Play one game. Returns (result, ply, fen, illegal_move)."""
    engine = Engine(engine_path)
    board = chess.Board()
    moves = []

    try:
        for ply in range(max_plies):
            best = engine.best_move(moves, ms)

            if best in ("0000", "(none)"):
                if board.is_game_over(claim_draw=False):
                    return "gameover:" + board.result(), ply, board.fen(), None
                return "no-move", ply, board.fen(), None

            try:
                move = chess.Move.from_uci(best)
            except ValueError:
                return "unparsable", ply, board.fen(), best

            if move not in board.legal_moves:
                return "illegal", ply, board.fen(), best

            board.push(move)
            moves.append(best)

            if board.is_game_over(claim_draw=False):
                return "gameover:" + board.result(), ply, board.fen(), None

        return "max-plies", max_plies, board.fen(), None
    finally:
        engine.close()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)

    engine_path = sys.argv[1]
    games = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    ms = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    max_plies = 120

    failures = 0
    for number in range(1, games + 1):
        result, ply, fen, bad = play_game(engine_path, ms, max_plies)

        if result == "illegal":
            print(f"game {number}: ILLEGAL MOVE {bad} at ply {ply}")
            print(f"           FEN: {fen}")
            failures += 1
        elif result in ("no-move", "unparsable"):
            print(f"game {number}: {result} at ply {ply}")
            print(f"           FEN: {fen}")
            failures += 1
        else:
            print(f"game {number}: {result} at ply {ply}")

    if failures:
        print(f"\n{failures} of {games} games failed")
        return 1

    print(f"\nall {games} games played legally")
    return 0


if __name__ == "__main__":
    sys.exit(main())
