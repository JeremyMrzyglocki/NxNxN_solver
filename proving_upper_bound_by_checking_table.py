#!/usr/bin/env python3

# This program checks all 3-cycle algorithms from the table.txt, proving the upper bound. There is no other purpose for
# this code for now.

"""
check_table.py

Purpose
-------
Read a lookup table of center 3-cycle algorithms (like "full_table_except_one_case.txt"),
expand each algorithm (supports commutator notation "[A,B]"),
simulate it as a permutation on 24 positions labeled by the string
"abcdefghijklmnopqrtsuvwx", and write out a per-line report that says:

- did the algorithm move exactly 3 letters (pure 3-cycle), or more?
- which cycles (decomposition) were actually applied?
- did the 3-cycle match the triplet on that line (ignoring orientation)?

Why the 24 letters?
-------------------
We treat the state as 24 "sector indices" (1..24). For realism, we simulate moves
in the same way your solver does for a *single* (a,b) subcell per sector.
Defaults: M=5, a=3, b=4 so that placeholders map like your translator:
"2" → p = M - b + 1 = 2 and "3" → p = M - a + 1 = 3.

Status is one of:
- OK_MATCH:        exactly one 3-cycle and it matches the claimed triplet
- OK_MATCH_INV:    exactly one 3-cycle and it matches the claimed triplet but reversed
- ONLY_3_BUT_DIFF: exactly one 3-cycle, but for different indices than the claim
- NOT_3_CYCLE:     moved != 3 letters (has extra swaps or missing moves)
- MISSING_ALG:     the algorithm field was empty
- PARSE_ERROR:     couldn't parse or expand tokens
"""

import sys
import re
from pathlib import Path

# --------------------------
# Config (can be overridden by CLI)
# --------------------------
DEFAULT_TABLE = "full_table_except_one_case.txt"
DEFAULT_OUT   = "table_check_results.txt"
DEFAULT_M, DEFAULT_A, DEFAULT_B = 5, 3, 4   # see header note

# --------------
# Core helpers
# --------------

# Surface quarter-turn 4-cycles on sector indices (1-based), from your solver
SURFACE_CYCLES = {
    'U': [1, 2, 3, 4],
    'L': [5, 6, 7, 8],
    'F': [9, 10, 11, 12],
    'R': [13, 14, 15, 16],
    'B': [17, 18, 19, 20],
    'D': [21, 22, 23, 24],
}

# Slice moves (two 4-cycles per face) + which coordinate they "fix", from your solver
FACE_SLICE_DEFS = {
    'R': {'c1':[2,20,22,10], 'f1':'b', 'c2':[11,3,17,23], 'f2':'a'},
    'L': {'c1':[1,9,21,19], 'f1':'a', 'c2':[4,12,24,18], 'f2':'b'},
    'F': {'c1':[4,13,22,7], 'f1':'a', 'c2':[3,16,21,6],  'f2':'b'},
    'B': {'c1':[2,5,24,15], 'f1':'a', 'c2':[1,8,23,14],  'f2':'b'},
    'U': {'c1':[9,5,17,13], 'f1':'b', 'c2':[10,6,18,14], 'f2':'a'},
    'D': {'c1':[12,16,20,8],'f1':'a', 'c2':[11,15,19,7], 'f2':'b'},
}

def invert_token(tok: str) -> str:
    """Inverse of a single move token like U, U', U2, 2R, 2R', 3U2."""
    tok = tok.strip()
    if tok.endswith("2"):
        return tok  # 180° is self-inverse
    elif tok.endswith("'"):
        return tok[:-1]  # (X')' = X
    else:
        return tok + "'"

def invert_sequence(seq: str) -> str:
    """Inverse of a space-separated token sequence."""
    toks = [t for t in re.split(r'\s+', seq.strip()) if t]
    inv = [invert_token(t) for t in reversed(toks)]
    return ' '.join(inv)

def expand_commutators(text: str) -> str:
    """
    Expand every [A,B] into 'A B A^-1 B^-1' safely.
    Handles nested pairs by iterating.
    """
    pattern = re.compile(r'\[([^\[\]]+?)\]')
    prev = None
    s = text
    while prev != s:
        prev = s
        def repl(m):
            inside = m.group(1)
            parts = inside.split(',')
            if len(parts) != 2:
                return inside  # malformed, keep as-is
            A, B = parts[0].strip(), parts[1].strip()
            return f"{A} {B} {invert_sequence(A)} {invert_sequence(B)}"
        s = pattern.sub(repl, s)
    return s

def apply_one_token_to_k(token: str, k: int, a_sel: int, b_sel: int, M: int) -> int:
    """
    Apply a single move token to a single sector index k (1..24), for the fixed (a_sel, b_sel).
    Supports quarter-turn, inverse (3x), and double turns.
    """
    # figure out exponent
    count = 1
    if token.endswith("2"):
        token, count = token[:-1], 2
    elif token.endswith("'"):
        token, count = token[:-1], 3

    m = re.match(r'^(\d+)?([ULFRBD])$', token)
    if not m:
        raise ValueError(f"Unrecognized token: {token}")
    prefix, face = m.groups()

    for _ in range(count):
        if prefix:
            p = int(prefix)
            a_t = M + 1 - p
            b_t = a_t
            defs = FACE_SLICE_DEFS[face]
            # c1 path
            if (defs['f1']=='a' and a_sel==a_t) or (defs['f1']=='b' and b_sel==b_t):
                cyc = defs['c1']
                if k in cyc:
                    i = cyc.index(k)
                    k = cyc[(i+1) % 4]
            # c2 path
            if (defs['f2']=='a' and a_sel==a_t) or (defs['f2']=='b' and b_sel==b_t):
                cyc = defs['c2']
                if k in cyc:
                    i = cyc.index(k)
                    k = cyc[(i+1) % 4]
        else:
            cyc = SURFACE_CYCLES[face]
            if k in cyc:
                i = cyc.index(k)
                k = cyc[(i+1) % 4]
    return k

def alg_to_perm(alg: str, a_sel: int, b_sel: int, M: int):
    """
    Convert an algorithm string to a permutation of 24 positions (0-based array),
    by simulating the action on every starting k in 1..24 for the fixed (a_sel,b_sel).
    """
    s = expand_commutators(alg.strip().upper())
    tokens = [t for t in re.split(r'\s+', s) if t]
    mapping = []
    for k0 in range(1, 25):
        k = k0
        for tok in tokens:
            k = apply_one_token_to_k(tok, k, a_sel, b_sel, M)
        mapping.append(k-1)  # 0-based
    return mapping

BASE_STATE = "abcdefghijklmnopqrtsuvwx"

def apply_perm_to_string(s: str, perm):
    arr = list(s)
    out = ['?']*len(arr)
    for i, ch in enumerate(arr):
        out[perm[i]] = ch
    return ''.join(out)

def perm_cycles(perm):
    """Return nontrivial cycles (1-based) of a 0-based permutation list perm."""
    n = len(perm)
    seen = [False]*n
    out = []
    for i in range(n):
        if seen[i] or perm[i] == i:
            continue
        cyc = []
        j = i
        while not seen[j]:
            seen[j] = True
            cyc.append(j+1)
            j = perm[j]
        out.append(tuple(cyc))
    return out

def cycles_to_str(cycles):
    return ' '.join('(' + ' '.join(map(str, c)) + ')' for c in cycles) if cycles else '-'

def status_from(cycles, claim_triplet):
    """
    Decide a status string based on cycles and the claimed triplet (i,j,k).
    """
    moved_count = sum(len(c) for c in cycles)
    three = [c for c in cycles if len(c) == 3]
    def same_set(c, t):
        return set(c) == set(t)
    def same_orientation(c, t):
        # true if c equals t rotated (i->j->k) or exactly t
        t = list(t)
        for r in range(3):
            if tuple(t[r:]+t[:r]) == c:
                return True
        return False
    def inverse_orientation(c, t):
        ti = (t[0], t[2], t[1])
        for r in range(3):
            rr = (ti[(r)%3], ti[(r+1)%3], ti[(r+2)%3])
            if rr == c:
                return True
        return False

    if moved_count == 3 and len(three) == 1:
        c = three[0]
        if same_set(c, claim_triplet):
            if same_orientation(c, claim_triplet):
                return "OK_MATCH"
            elif inverse_orientation(c, claim_triplet):
                return "OK_MATCH_INV"
            else:
                return "OK_MATCH"
        else:
            return "ONLY_3_BUT_DIFF"
    else:
        return "NOT_3_CYCLE"

def parse_table_line(line: str):
    """
    Return (i,j,k, alg) or None if not a data line.
    Expected format: 'i,j,k  algorithm: <alg or empty>'
    """
    m = re.match(r'^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s+algorithm:\s*(.*)\s*$', line)
    if not m:
        return None
    i, j, k = int(m.group(1)), int(m.group(2)), int(m.group(3))
    alg = m.group(4)
    return (i, j, k, alg)

# --------------------------
# Main
# --------------------------

def main():
    args = sys.argv[1:]
    table_path = args[0] if len(args) >= 1 else DEFAULT_TABLE
    out_path   = args[1] if len(args) >= 2 else DEFAULT_OUT

    M = int(args[2]) if len(args) >= 3 else DEFAULT_M
    a_sel = int(args[3]) if len(args) >= 4 else DEFAULT_A
    b_sel = int(args[4]) if len(args) >= 5 else DEFAULT_B

    lines = Path(table_path).read_text(encoding='utf-8', errors='ignore').splitlines()

    with open(out_path, 'w', encoding='utf-8') as out:
        out.write("line_no\tclaimed_triplet\tstatus\tmoved_count\tmoved_positions\tcycles\tresult_state\talgorithm\n")
        for idx, line in enumerate(lines, start=1):
            parsed = parse_table_line(line)
            if not parsed:
                continue
            i, j, k, alg = parsed
            trip = (i, j, k)

            alg_clean = alg.strip()
            if alg_clean == "":
                out.write(f"{idx}\t{i},{j},{k}\tMISSING_ALG\t0\t-\t-\t-\t\n")
                continue

            try:
                perm = alg_to_perm(alg_clean, a_sel, b_sel, M)
                cycles = perm_cycles(perm)
                moved_positions = sorted({x for cyc in cycles for x in cyc})
                status = status_from(cycles, trip)
                result_state = apply_perm_to_string(BASE_STATE, perm)
                out.write(
                    f"{idx}\t{i},{j},{k}\t{status}\t{len(moved_positions)}\t"
                    f"{','.join(map(str, moved_positions)) if moved_positions else '-'}\t"
                    f"{cycles_to_str(cycles)}\t{result_state}\t{alg_clean}\n"
                )
            except Exception as e:
                out.write(f"{idx}\t{i},{j},{k}\tPARSE_ERROR\t0\t-\t-\t-\t{alg_clean}  # {e}\n")

    print(f"Wrote results to {out_path}")

if __name__ == "__main__":
    main()
