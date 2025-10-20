import re, time, random, itertools
from pathlib import Path
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
import numpy as np
from typing import Optional

# ---------- paths / constants ----------
RUNS_ROOT = Path("runs")
RUN_DIR = RUNS_ROOT / time.strftime("%Y-%m-%d_%H-%M-%S")
RUN_DIR.mkdir(parents=True, exist_ok=True)

COLOR_ID = {'W': 0, 'O': 1, 'G': 2, 'R': 3, 'B': 4, 'Y': 5}
ID_COLOR = ['W', 'O', 'G', 'R', 'B', 'Y']
CHUNKSIZE = 1024

# ---------- packed color field ----------
class ColorField:
    __slots__ = ('M', '_N', 'buf')
    def __init__(self, M, init_face_color=None):
        self.M = M
        self._N = 24 * M * M
        self.buf = np.zeros(((self._N * 3 + 7) // 8,), dtype=np.uint8)
        if init_face_color is not None:
            for k in range(1, 25):
                v = COLOR_ID[init_face_color[k]]
                self._fill_face(k, v)

    def _lin_index(self, k, a, b):
        return ((k - 1) * self.M + (a - 1)) * self.M + (b - 1)

    def _get_one(self, idx):
        bitpos = idx * 3
        byte = bitpos >> 3
        offset = bitpos & 7
        x = int(self.buf[byte])
        val = (x >> offset) & 0x7
        rem = offset + 3 - 8
        if rem > 0:
            x2 = int(self.buf[byte + 1])
            val |= (x2 & ((1 << rem) - 1)) << (3 - rem)
        return val

    def _set_one(self, idx, val):
        val &= 0x7
        bitpos = idx * 3
        byte = bitpos >> 3
        offset = bitpos & 7
        mask = 0x7 << offset
        b0 = int(self.buf[byte])
        b0 = (b0 & (~mask & 0xFF)) | ((val << offset) & 0xFF)
        self.buf[byte] = b0
        rem = offset + 3 - 8
        if rem > 0:
            mask2 = (1 << rem) - 1
            b1 = int(self.buf[byte + 1])
            b1 = (b1 & (~mask2 & 0xFF)) | ((val >> (3 - rem)) & mask2)
            self.buf[byte + 1] = b1

    def _fill_face(self, k, v):
        M = self.M
        start = (k - 1) * M * M
        for idx in range(start, start + M * M):
            self._set_one(idx, v)

    def __getitem__(self, key):
        k, a, b = key
        idx = self._lin_index(k, a, b)
        return ID_COLOR[self._get_one(idx)]

    def __setitem__(self, key, value):
        k, a, b = key
        idx = self._lin_index(k, a, b)
        self._set_one(idx, COLOR_ID[value])

    def get(self, key, default=None):
        k, a, b = key
        idx = self._lin_index(k, a, b)
        v = self._get_one(idx)
        return ID_COLOR[v] if 0 <= v < 6 else default

    def copy(self):
        other = ColorField(self.M)
        other.buf = self.buf.copy()
        return other

# ---------- 3-cycle planner ----------
"""
def greedy_3cycle_sort(target, initial, max_steps=10000):
    tgt = list(target)
    arr = list(initial)
    correct = [arr[i] == tgt[i] for i in range(24)]
    incorrect = [i for i in range(24) if not correct[i]] # caution! not the inverse of correct

    def gain_of(i, j, k):
        before = (correct[i]) + (correct[j]) + (correct[k])
        after  = (arr[k] == tgt[i]) + (arr[i] == tgt[j]) + (arr[j] == tgt[k])
        return after - before

    def apply_and_update(i, j, k):
        arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]
        for idx in (i, j, k):
            was = correct[idx]
            now = (arr[idx] == tgt[idx])
            correct[idx] = now # updates the correctness-list
            if was and not now:
                if idx not in incorrect: incorrect.append(idx) # updates the incorrectness-list
            elif not was and now:
                if idx in incorrect: incorrect.remove(idx) # updates the incorrectness-list

    def try_two_cycle_fix():
        if len(incorrect) != 2: return None
        p, q = incorrect
        if arr[p] == tgt[q] and arr[q] == tgt[p]: # if this isn't given anyways after arriving at this line, I think there is a problem
            candidates = [r for r in range(24) if r not in (p, q) and tgt[r] == tgt[p]]
            for r in candidates:
                if arr[r] == tgt[r]: return (p, q, r)
            return (p, q, candidates[0]) if candidates else None
        return None

    def canon_cycle(i, j, k):
        if i <= j and i <= k:   return (i, j, k)
        if j <= i and j <= k:   return (j, k, i)
        return (k, i, j)

    step = 0
    moves = []
    wave_id = 1
    current_wave_used = set()

    while incorrect and step < max_steps:
        best_gain = 0
        best_move = None

        inc = incorrect
        if len(inc) >= 3:
            for x, y, z in itertools.combinations(inc, 3):
                for (i, j, k) in ((x, y, z), (x, z, y)):
                    if not (arr[k] == tgt[i] or arr[i] == tgt[j] or arr[j] == tgt[k]): continue # if not at least one is solved
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3: break
                if best_gain == 3: break

        if best_move is None or best_gain <= 0:
            fix = try_two_cycle_fix()
            if fix is not None:
                i, j, k = fix
                if {i, j, k} & current_wave_used:
                    wave_id += 1
                    current_wave_used.clear()
                apply_and_update(i, j, k)
                step += 1
                ci, cj, ck = canon_cycle(i, j, k)
                moves.append((ci, cj, ck, wave_id))
                current_wave_used.update((ci, cj, ck))
                continue
            break

        i, j, k = best_move
        if {i, j, k} & current_wave_used:
            wave_id += 1
            current_wave_used.clear()
        apply_and_update(i, j, k)
        step += 1
        ci, cj, ck = canon_cycle(i, j, k)
        moves.append((ci, cj, ck, wave_id))
        current_wave_used.update((ci, cj, ck))
    return step, moves"""

def greedy_3cycle_sort(target, initial, max_steps=10000):
    """
    Logic-identical to the original:
      - Same candidate enumeration order
      - Same gain calculation
      - Same parity fix behavior
      - Same wave packing & tie-breaking
    Only mechanical speedups (locals, cached lookups, set for membership).
    """
    import itertools

    N = 24
    tgt = list(target)
    arr = list(initial)

    # same construction as before
    correct = [arr[i] == tgt[i] for i in range(N)]
    incorrect = [i for i in range(N) if not correct[i]]  # order matters
    incorrect_set = set(incorrect)                        # membership helper (does NOT replace the list)

    # local bindings to reduce attribute lookups
    rngN = range(N)
    combinations = itertools.combinations

    def gain_of(i, j, k):
        # identical math, just local names
        ci, cj, ck = correct[i], correct[j], correct[k]
        before = (1 if ci else 0) + (1 if cj else 0) + (1 if ck else 0)
        after  = (1 if (arr[k] == tgt[i]) else 0) + (1 if (arr[i] == tgt[j]) else 0) + (1 if (arr[j] == tgt[k]) else 0)
        return after - before

    def apply_and_update(i, j, k):
        # same 3-cycle application
        arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]
        # update correctness + incorrect/incorrect_set exactly like before
        for idx in (i, j, k):
            was = correct[idx]
            now = (arr[idx] == tgt[idx])
            correct[idx] = now
            if was and not now:
                # append only if not already in list
                if idx not in incorrect_set:
                    incorrect.append(idx)   # keep the original list order behavior
                    incorrect_set.add(idx)
            elif (not was) and now:
                # remove only if present (preserve list order of the remainder)
                if idx in incorrect_set:
                    incorrect.remove(idx)   # same O(n) behavior/order as before
                    incorrect_set.remove(idx)

    def try_two_cycle_fix():
        # unchanged logic (uses the same order in 'incorrect')
        if len(incorrect) != 2:
            return None
        p, q = incorrect
        if arr[p] == tgt[q] and arr[q] == tgt[p]:
            # same candidate scan order
            candidates = [r for r in rngN if r not in (p, q) and tgt[r] == tgt[p]]
            for r in candidates:
                if arr[r] == tgt[r]:
                    return (p, q, r)
            return (p, q, candidates[0]) if candidates else None
        return None

    def canon_cycle(i, j, k):
        # identical canonicalization for recording
        if i <= j and i <= k:   return (i, j, k)
        if j <= i and j <= k:   return (j, k, i)
        return (k, i, j)

    step = 0
    moves = []
    wave_id = 1
    current_wave_used = set()

    while incorrect and step < max_steps:
        best_gain = 0
        best_move = None

        # IMPORTANT: keep using the *list* 'incorrect' to preserve iteration order
        inc = incorrect
        if len(inc) >= 3:
            # combinations(inc, 3) yields triples in the same order as your original
            for x, y, z in combinations(inc, 3):
                # same two orientation checks and same short-circuit structure
                # (x,y,z)
                i, j, k = x, y, z
                if (arr[k] == tgt[i]) or (arr[i] == tgt[j]) or (arr[j] == tgt[k]):
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3:
                            break
                # (x,z,y)
                i, j, k = x, z, y
                if (best_gain != 3) and ((arr[k] == tgt[i]) or (arr[i] == tgt[j]) or (arr[j] == tgt[k])):
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3:
                            break
            # preserve the same break behavior after inner loop
            # (the outer loop checks best_gain==3 and breaks once as in original)
            if best_gain == 3:
                pass  # no-op; retains identical flow

        if best_move is None or best_gain <= 0:
            fix = try_two_cycle_fix()
            if fix is not None:
                i, j, k = fix
                if ({i, j, k} & current_wave_used):
                    wave_id += 1
                    current_wave_used.clear()
                apply_and_update(i, j, k)
                step += 1
                ci, cj, ck = canon_cycle(i, j, k)
                moves.append((ci, cj, ck, wave_id))
                current_wave_used.update((ci, cj, ck))
                continue
            break  # identical early stop

        i, j, k = best_move
        if ({i, j, k} & current_wave_used):
            wave_id += 1
            current_wave_used.clear()
        apply_and_update(i, j, k)
        step += 1
        ci, cj, ck = canon_cycle(i, j, k)
        moves.append((ci, cj, ck, wave_id))
        current_wave_used.update((ci, cj, ck))

    return step, moves


# ---------- utils ----------
def count_moves(algorithm: str) -> int:
    total = 0
    for comm in re.findall(r'\[(.*?)\]', algorithm):
        a_part, b_part = comm.split(",", 1)
        total += 2 * (len(a_part.strip().split()) + len(b_part.strip().split()))
    tokens = re.sub(r'\[.*?\]', '', algorithm).split()
    total += len(tokens)
    return total

def save_orbits_to_txt(color_field, M: int, filepath: str) -> None:
    with open(filepath, "w", encoding="utf-8") as f:
        for a in range(1, M + 1):
            for b in range(1, M + 1):
                letters = [color_field[(k, a, b)] for k in range(1, 25)]
                f.write(f"o {a},{b} : {''.join(letters)}\n")

def _compute_orbit_swap(args):
    a, b, initial = args
    target = 'WWWWOOOOGGGGRRRRBBBBYYYY'
    _, moves = greedy_3cycle_sort(target, initial, max_steps=10000)
    waves = defaultdict(list)
    for (i, j, k, w) in moves:
        waves[w].append(f"orbit({a},{b}) - rot({i+1},{j+1},{k+1})") # I prefer 1-based-indexing for the rotations
    return dict(waves)

class SolverV2_5:
    def __init__(self, M=16, seed: Optional[int] = None):
        self.M = M
        self.seed = seed
        self._rng = random.Random(seed) if seed is not None else random.Random()
        colors = {
            **{k: 'W' for k in range(1, 5)},
            **{k: 'O' for k in range(5, 9)},
            **{k: 'G' for k in range(9, 13)},
            **{k: 'R' for k in range(13, 17)},
            **{k: 'B' for k in range(17, 21)},
            **{k: 'Y' for k in range(21, 25)},
        }
        self.subcell_color = ColorField(self.M, init_face_color=colors)
        self.baseline_subcell_color = None
        self.run_dir = RUN_DIR
        self.project_root = Path(__file__).resolve().parent

    def scramble_all_orbits(self):
        base = ['W']*4 + ['O']*4 + ['G']*4 + ['R']*4 + ['B']*4 + ['Y']*4
        for a in range(1, self.M+1):
            for b in range(1, self.M+1):
                lst = base[:]
                self._rng.shuffle(lst)
                for k in range(1, 25):
                    self.subcell_color[(k, a, b)] = lst[k-1]
        self.baseline_subcell_color = self.subcell_color.copy()

    def produce_full_plan(self):
        tiny_args = (
            (a, b, tuple(self.baseline_subcell_color[(k, a, b)] for k in range(1, 25)))
            for a in range(1, self.M+1)
            for b in range(1, self.M+1)
        )

        open_files = {}
        with ProcessPoolExecutor() as exe:
            for wave_dict in exe.map(_compute_orbit_swap, tiny_args, chunksize=CHUNKSIZE):
                for w, lines in wave_dict.items():
                    f = open_files.get(w)
                    if f is None:
                        p = self.run_dir / f"cycles_wave{w}.txt"
                        f = p.open("w", encoding="utf-8")
                        open_files[w] = f
                    f.write("\n".join(lines))
                    f.write("\n")


    def run_rearrange_and_count(self):
        table = str(self.project_root / "table_with_formula.txt")
        exe   = str(self.project_root / "rearrange")
        Mval  = str(self.M)
        waves = []
        for p in sorted(self.run_dir.glob("cycles_wave*.txt")):
            m = re.match(r"cycles_wave(\d+)\.txt$", p.name)
            if m: waves.append((int(m.group(1)), p))
        per_wave_counts = {}
        for w, in_path in waves:
            out_path = self.run_dir / f"wave_{w}_parallel.txt"
            cmd = [exe, "--cycles", str(in_path), "--table", table, "--out", str(out_path), "--M", Mval]
            proc = __import__("subprocess").run(cmd, capture_output=True, text=True)
            moves_this_wave = 0
            if out_path.exists():
                with out_path.open("r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        alg = line.strip()
                        if alg:
                            moves_this_wave += count_moves(alg)
            per_wave_counts[w] = moves_this_wave
            if proc.stdout.strip(): print(proc.stdout.strip())
            if proc.stderr.strip(): print(proc.stderr.strip())
            print(f"[wave {w}] → {out_path.name} ({moves_this_wave} moves)")
        total = sum(per_wave_counts.values())
        print("\n— Per-wave move counts —")
        for w in sorted(per_wave_counts): print(f"  wave_{w}_parallel.txt: {per_wave_counts[w]}")
        print(f"\nTOTAL MOVES: {total}")

    def run_pipeline(self, *, scramble: bool = False):
        t0 = time.time()
        if scramble:
            self.scramble_all_orbits()
            self.baseline_subcell_color = self.subcell_color.copy()
        # ensure baseline exists for produce_full_plan()
        if self.baseline_subcell_color is None:
            self.baseline_subcell_color = self.subcell_color.copy()
        # now operate on the current state (no hidden re-scramble)
        save_orbits_to_txt(self.subcell_color, self.M, str(self.run_dir / "cube_state.txt"))
        self.produce_full_plan()
        self.run_rearrange_and_count()
        print(f"\n✅ solver_v2_5 finished in {time.time() - t0:.2f}s")
        print(f"Outputs in: {self.run_dir}")

# ---------- main ----------
if __name__ == "__main__":
    M = 50  # N = 2M, only odd cubes supported by planner logic
    SolverV2_5(M=M).run_pipeline(scramble=True)