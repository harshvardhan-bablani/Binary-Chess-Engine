import subprocess
import sys

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

print()
print("ALL REGRESSION PROBES PASSED" if ok else "SOME REGRESSION PROBES FAILED")
sys.exit(0 if ok else 1)
