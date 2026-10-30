import queue
import subprocess
import sys
import threading
import time

EXE = sys.argv[1]


def uci(cmds, timeout=10):
    p = subprocess.run([EXE], input="\n".join(cmds) + "\n",
                       capture_output=True, text=True, timeout=timeout)
    return p.returncode, [l for l in (p.stdout or "").splitlines() if l.strip()], (p.stderr or "").strip()


def fen_of(cmds):
    rc, out, err = uci(cmds)
    fen = [l[5:] for l in out if l.startswith("FEN:")]
    return rc, (fen[0] if fen else None), err


def check(name, condition, detail=""):
    status = "PASS" if condition else "FAIL"
    print(f"  [{status}] {name}")
    if detail:
        print(f"         {detail}")
    return condition


ok = True

print("1. bishop / king diagonals (perft already covers, spot-check here)")
rc, out, _ = uci(["position startpos moves b2b3", "quit"])
rc, fen, _ = fen_of(["position startpos moves b2b3", "print", "quit"])
ok &= check("bishop can reach b2 and a3", fen is not None and fen.startswith("rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR"), fen)

print("2. illegal castling through pieces is rejected")
rc, fen, _ = fen_of(["position startpos moves e1g1", "print", "quit"])
ok &= check("e1g1 at start does not corrupt the board",
            fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fen)

print("3. castling without rights is rejected")
rc, fen, _ = fen_of(["position fen 4k3/8/8/8/8/8/8/4K2R w - - 0 1 moves e1g1", "print", "quit"])
ok &= check("no rights -> no castle", fen == "4k3/8/8/8/8/8/8/4K2R w - - 0 1", fen)

print("4. castling while in check is rejected")
rc, fen, _ = fen_of(["position fen 4k3/8/8/8/8/8/4r3/4K2R w - - 0 1 moves e1g1", "print", "quit"])
ok &= check("in check -> no castle", fen == "4k3/8/8/8/8/8/4r3/4K2R w - - 0 1", fen)

print("5. illegal promotion is rejected")
rc, fen, _ = fen_of(["position startpos moves e2e4 a7a6 e4e5q", "print", "quit"])
ok &= check("e4e5q leaves the pawn on e4",
            fen == "rnbqkbnr/1ppppppp/p7/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", fen)

print("6. bishop cannot teleport through its own pawn")
rc, fen, _ = fen_of(["position startpos moves g1f3 g8f6 f1c4", "print", "quit"])
ok &= check("f1c4 rejected", fen is not None and "/2B5/" not in fen, fen)

print("7. FEN with garbage / wrong rank count is rejected, position preserved")
rc, fen, _ = fen_of(["position fen zzzz/zzzz/zzzz/zzzz/zzzz/zzzz/zzzz/zzzz w - - 0 1", "print", "quit"])
ok &= check("junk FEN rejected", fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fen)
rc, fen, _ = fen_of(["position fen 8/8/8 w - - 0 1", "print", "quit"])
ok &= check("3-rank FEN rejected", fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fen)
rc, fen, _ = fen_of(["position fen 8/8/8/8/8/8/8/8/8/8 w - - 0 1", "print", "quit"])
ok &= check("10-rank FEN rejected", fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fen)

print("8. FEN parsing now reads all six fields")
rc, fen, _ = fen_of(["position fen 4k3/8/8/8/8/8/8/R3K2R b KQ - 7 42", "print", "quit"])
ok &= check("side/castling/ep/clocks round-trip",
            fen == "4k3/8/8/8/8/8/8/R3K2R b KQ - 7 42", fen)

print("9. clocks advance correctly")
rc, fen, _ = fen_of(["position startpos moves g1f3", "print", "quit"])
ok &= check("after g1f3: halfmove 1 fullmove 1",
            fen == "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1", fen)
rc, fen, _ = fen_of(["position startpos moves g1f3 g8f6", "print", "quit"])
ok &= check("after g8f6: halfmove 2 fullmove 2",
            fen == "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 2 2", fen)
rc, fen, _ = fen_of(["position startpos moves e2e4", "print", "quit"])
ok &= check("after e2e4: halfmove reset to 0",
            fen == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", fen)

print("10. `go` before `position` no longer crashes")
rc, out, err = uci(["go wtime 1000 btime 1000", "quit"])
ok &= check("go on a fresh process exits 0 with a bestmove",
            rc == 0 and any(l.startswith("bestmove ") for l in out),
            f"exit={rc} err={err[:80]}")

print("11. checkmate / stalemate emit bestmove 0000")
# Fool's mate: white is checkmated, no legal moves.
rc, out, err = uci(["position startpos moves f2f3 e7e5 g2g4 d8h4", "go wtime 1000 btime 1000", "quit"])
best = [l for l in out if l.startswith("bestmove")]
ok &= check("checkmated side returns bestmove 0000",
            rc == 0 and best and best[0] == "bestmove 0000", best[0] if best else "none")

print("12. terminal mode: EOF, CRLF and short input")
for name, data in [("stdin closed immediately", ""),
                   ("EOF after one move", "e2e4\n"),
                   ("CRLF quit", "quit\r\n"),
                   ("single character 'x'", "x\nquit\n"),
                   ("two characters 'e2'", "e2\nquit\n")]:
    try:
        p = subprocess.run([EXE, "terminal"], input=data, capture_output=True,
                           text=True, timeout=8)
        good = p.returncode == 0
        detail = f"exit={p.returncode}"
        if p.stderr.strip():
            detail += " err=" + p.stderr.strip().splitlines()[-1][:70]
    except subprocess.TimeoutExpired:
        good = False
        detail = "TIMEOUT / HANG"
    ok &= check(f"terminal: {name}", good, detail)

print("13. castling rights are revoked when a rook is captured")
# White rook a1 takes the black rook a8: black loses the queenside right, and
# white loses its own queenside right because the rook left a1.
rc, fen, _ = fen_of(["position fen r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 moves a1a8", "print", "quit"])
ok &= check("rights after a1xa8 are Kk",
            fen == "R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1", fen)

# The exact self-play position that used to produce an illegal e8g8.
rc, fen, _ = fen_of(["position fen r1b1k2N/pp1nb1pp/8/2pNp3/8/8/PPP1PPPP/R1B1KB1R b KQq - 0 10 moves e8g8", "print", "quit"])
ok &= check("black cannot castle with no rook on h8",
            fen == "r1b1k2N/pp1nb1pp/8/2pNp3/8/8/PPP1PPPP/R1B1KB1R b KQq - 0 10", fen)

print("14. UCI clock, depth, stop and score reporting")


class LiveEngine:
    """One long-lived engine process, so a search can actually run.

    The one-shot `uci()` helper sends `quit` straight after `go`, and the
    engine stops the search when it sees `quit`. Anything that measures a
    clock, a depth or a stop has to keep the process open and wait for the
    `bestmove` line instead.
    """

    def __init__(self):
        self.p = subprocess.Popen([EXE], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  text=True, bufsize=1)
        self.q = queue.Queue()
        threading.Thread(target=self._read, daemon=True).start()
        self.send("uci")
        self.wait("uciok")

    def _read(self):
        for line in self.p.stdout:
            self.q.put(line.strip())

    def send(self, cmd):
        self.p.stdin.write(cmd + "\n")
        self.p.stdin.flush()

    def wait(self, prefix, timeout=15):
        while True:
            line = self.q.get(timeout=timeout)
            if line.startswith(prefix):
                return line

    def search(self, cmd, timeout=15):
        """Send `cmd` and collect output until bestmove. Returns (lines, seconds)."""
        start = time.time()
        self.send(cmd)
        lines = []
        while True:
            line = self.q.get(timeout=timeout)
            lines.append(line)
            if line.startswith("bestmove"):
                return lines, time.time() - start

    def close(self):
        try:
            self.send("quit")
            self.p.wait(timeout=5)
        except Exception:
            self.p.kill()


def info_lines(lines):
    return [l for l in lines if l.startswith("info depth")]


def reported_depth(lines):
    depths = []
    for line in info_lines(lines):
        parts = line.split()
        if len(parts) > 2 and parts[1] == "depth":
            try:
                depths.append(int(parts[2]))
            except ValueError:
                pass
    return max(depths) if depths else 0


def last_score(lines):
    """The (kind, value) of the last `score` field, e.g. ('cp', 40)."""
    score = None
    for line in info_lines(lines):
        parts = line.split()
        if "score" in parts:
            i = parts.index("score")
            if i + 2 < len(parts):
                try:
                    score = (parts[i + 1], int(parts[i + 2]))
                except ValueError:
                    pass
    return score


engine = LiveEngine()
try:
    engine.send("position startpos")
    lines, elapsed = engine.search("go movetime 400")
    ok &= check("movetime 400 finishes in about 0.4 s", elapsed < 1.5, f"{elapsed:.2f}s")
    ok &= check("movetime search reaches at least depth 3", reported_depth(lines) >= 3,
                f"depth {reported_depth(lines)}")

    engine.send("position startpos")
    lines, elapsed = engine.search("go depth 4")
    ok &= check("go depth 4 stops at depth 4", reported_depth(lines) == 4,
                f"depth {reported_depth(lines)}")

    engine.send("position startpos")
    engine.send("go infinite")
    time.sleep(0.4)
    lines, elapsed = engine.search("stop")
    ok &= check("go infinite + stop answers bestmove", bool(lines) and
                lines[-1].startswith("bestmove"), lines[-1] if lines else "none")
    ok &= check("stop is honoured quickly", elapsed < 1.5, f"{elapsed:.2f}s")

    engine.send("position startpos")
    lines, elapsed = engine.search("go nodes 2000")
    ok &= check("go nodes 2000 returns a legal bestmove",
                bool(lines) and lines[-1].startswith("bestmove") and
                lines[-1] != "bestmove 0000", lines[-1] if lines else "none")
    ok &= check("go nodes 2000 is quick", elapsed < 1.5, f"{elapsed:.2f}s")

    # Black to move, a queen up. UCI scores are from the side to move, so the
    # engine must report a positive score here.
    engine.send("position fen 7k/q7/8/8/8/8/8/7K b - - 0 1")
    lines, _ = engine.search("go depth 3")
    score = last_score(lines)
    ok &= check("score is side-to-move relative (Black +queen is positive)",
                score is not None and score[0] == "cp" and score[1] > 0, str(score))

    # White mates in one with Ra8#.
    engine.send("position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1")
    lines, _ = engine.search("go depth 2")
    score = last_score(lines)
    ok &= check("mate in one is reported as 'score mate 1'",
                score is not None and score[0] == "mate" and score[1] == 1, str(score))
    ok &= check("mate in one plays a1a8",
                bool(lines) and lines[-1].split()[-1] == "a1a8",
                lines[-1] if lines else "none")

    engine.send("position startpos moves f2f3 e7e5 g2g4 d8h4")
    lines, _ = engine.search("go movetime 200")
    ok &= check("checkmated position returns bestmove 0000",
                bool(lines) and lines[-1] == "bestmove 0000",
                lines[-1] if lines else "none")
finally:
    engine.close()

print()
print("ALL REGRESSION PROBES PASSED" if ok else "SOME REGRESSION PROBES FAILED")
sys.exit(0 if ok else 1)
