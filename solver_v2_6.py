import re, time, random, itertools
from pathlib import Path
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
import numpy as np
from typing import Optional
import itertools
from time import perf_counter
import os
from typing import Tuple
import subprocess, shutil
from concurrent.futures import ThreadPoolExecutor, as_completed
from collections import defaultdict
from typing import Dict, List, Tuple


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

def greedy_3cycle_sort(target, initial, max_steps=10000):

    N = 24
    tgt = list(target)
    arr = list(initial)

    correct = [arr[i] == tgt[i] for i in range(N)]
    incorrect = [i for i in range(N) if not correct[i]]  
    incorrect_set = set(incorrect)                 

    rngN = range(N)
    combinations = itertools.combinations

    def gain_of(i, j, k):
        # identical math, just local names
        ci, cj, ck = correct[i], correct[j], correct[k]
        before = (1 if ci else 0) + (1 if cj else 0) + (1 if ck else 0)
        after  = (1 if (arr[k] == tgt[i]) else 0) + (1 if (arr[i] == tgt[j]) else 0) + (1 if (arr[j] == tgt[k]) else 0)
        return after - before

    def apply_and_update(i, j, k):
        arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]
        for idx in (i, j, k):
            was = correct[idx]
            now = (arr[idx] == tgt[idx])
            correct[idx] = now
            if was and not now:
                if idx not in incorrect_set:
                    incorrect.append(idx)  
                    incorrect_set.add(idx)
            elif (not was) and now:
                if idx in incorrect_set:
                    incorrect.remove(idx)  
                    incorrect_set.remove(idx)

    def try_two_cycle_fix():
        if len(incorrect) != 2:
            return None
        p, q = incorrect
        if arr[p] == tgt[q] and arr[q] == tgt[p]:
            candidates = [r for r in rngN if r not in (p, q) and tgt[r] == tgt[p]]
            for r in candidates:
                if arr[r] == tgt[r]:
                    return (p, q, r)
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
            for x, y, z in combinations(inc, 3):
                i, j, k = x, y, z
                if (arr[k] == tgt[i]) or (arr[i] == tgt[j]) or (arr[j] == tgt[k]):
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3:
                            break
                i, j, k = x, z, y
                if (best_gain != 3) and ((arr[k] == tgt[i]) or (arr[i] == tgt[j]) or (arr[j] == tgt[k])):
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3:
                            break
            if best_gain == 3:
                pass

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
            break 

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

class SolverV2_6:
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
        self.sw = Stopwatch()


    def scramble_all_orbits(self):
        self.sw.start("scramble")
        base = ['W']*4 + ['O']*4 + ['G']*4 + ['R']*4 + ['B']*4 + ['Y']*4
        for a in range(1, self.M+1):
            for b in range(1, self.M+1):
                lst = base[:]
                self._rng.shuffle(lst)
                for k in range(1, 25):
                    self.subcell_color[(k, a, b)] = lst[k-1]
        self.baseline_subcell_color = self.subcell_color.copy()
        self.sw.stop("scramble")


    def produce_full_plan(self, *, mode: str = "sorting_network"):
        self.sw.start("plan.total")
        
        if mode == "row_wise":
            tiny_args = (
                (a, b, tuple(self.baseline_subcell_color[(k, a, b)] for k in range(1, 25)))
                for a in range(1, self.M+1)
                for b in range(1, self.M+1)
            )

            open_files = {}
            with ProcessPoolExecutor() as exe:
                t_map0 = perf_counter()
                for wave_dict in exe.map(_compute_orbit_swap, tiny_args, chunksize=CHUNKSIZE):
                    # time spent waiting on/doing compute since last iteration
                    t_map1 = perf_counter()
                    self.sw.add("plan.compute", t_map1 - t_map0)

                    # write the current chunk
                    t_write0 = perf_counter()
    
                    for w, lines in wave_dict.items():
                        f = open_files.get(w)
                        if f is None:
                            p = self.run_dir / f"cycles_wave{w}.txt"
                            f = p.open("w", encoding="utf-8")
                            open_files[w] = f
                        f.write("\n".join(lines))
                        f.write("\n")
                        
                    t_write1 = perf_counter()
                    self.sw.add("plan.write", t_write1 - t_write0)

                    # next compute window starts now
                    t_map0 = perf_counter()

            # close any open files
            for f in open_files.values():
                try:
                    f.close()
                except:
                    pass

        elif mode in ("sorting_network", "sorting_network_2D"):
            state_path = self.run_dir / "cube_state.txt"
            exe = str(self.project_root / "compute_face_swap")
            cmd = [exe, "--M", str(self.M), "--state", str(state_path), "--outdir", str(self.run_dir)]
            t0 = perf_counter()
            proc = __import__("subprocess").run(cmd, capture_output=True, text=True)
            t1 = perf_counter()
            self.sw.add("plan.compute", t1 - t0)
        else:
            raise ValueError(f"Unknown mode: {mode}")

        self.sw.stop("plan.total")
        
    def run_sorting_network_2D_stage(self, *, sector_count: int = 5) -> None:
        """
        After cycles are generated (cycles_wave*.txt), do the 2D bucketing,
        run the orbit_pipeline binary, and stitch a single solution.txt.
        """
        # 1) group 3-cycles into orbit_points/*.txt
        extract_orbits_for_all_cycles(waves_dir=str(self.run_dir))

        # 2) paths
        run_dir   = self.run_dir
        orbit_dir = os.path.join(run_dir, "orbit_points")
        inter_dir = os.path.join(run_dir, "intermediary")
        sol_dir   = os.path.join(run_dir, "solutions")
        os.makedirs(inter_dir, exist_ok=True)
        os.makedirs(sol_dir, exist_ok=True)

        table_path = os.path.join(self.project_root, "table_with_formula_v2.txt")
        exe        = os.path.join(self.project_root, "orbit_pipeline")

        # 3) run orbit_pipeline
        cmd = [exe, orbit_dir, str(self.M), str(sector_count), inter_dir, table_path, sol_dir]
        print("Running:", " ".join(cmd))
        subprocess.run(cmd, check=True)
        print("Solution files written to:", sol_dir)

        # 4) unify into a single run_dir/solution.txt
        write_big_solution_file(sol_dir=sol_dir, run_dir=str(run_dir), out_name="solution.txt")




    def run_rearrange_and_count(self):
        per_wave_times = {}
        self.sw.start("expand_count.total")
        table = str(self.project_root / "table_with_formula_v2.txt")
        exe   = str(self.project_root / "rearrange")
        Mval  = str(self.M)

        waves = []
        for p in sorted(self.run_dir.glob("cycles_wave*.txt")):
            m = re.match(r"cycles_wave(\d+)\.txt$", p.name)
            if m:
                waves.append((int(m.group(1)), p))

        per_wave_counts = {}
        for w, in_path in waves:
            out_path = self.run_dir / f"wave_{w}_parallel.txt"
            cmd = [exe, "--cycles", str(in_path), "--table", table, "--out", str(out_path), "--M", Mval]

            t0 = perf_counter()
            proc = __import__("subprocess").run(cmd, capture_output=True, text=True)
            t1 = perf_counter()

            moves_this_wave = 0
            t2 = perf_counter()
            if out_path.exists():
                with out_path.open("r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        alg = line.strip()
                        if alg:
                            moves_this_wave += count_moves(alg)
            t3 = perf_counter()

            per_wave_counts[w] = moves_this_wave
            per_wave_times[w] = {
                "expand_sec": t1 - t0,
                "count_sec":  t3 - t2,
                "total_sec":  (t1 - t0) + (t3 - t2),
            }

            if proc.stdout.strip():
                print(proc.stdout.strip())
            if proc.stderr.strip():
                print(proc.stderr.strip())
            print(
                f"[wave {w}] → {out_path.name} ({moves_this_wave} moves) "
                f"[expand {per_wave_times[w]['expand_sec']:.3f}s | "
                f"count {per_wave_times[w]['count_sec']:.3f}s]"
            )

        self.sw.stop("expand_count.total")

        total = sum(per_wave_counts.values())
        print("\n— Per-wave move counts —")
        for w in sorted(per_wave_counts):
            print(f"  wave_{w}_parallel.txt: {per_wave_counts[w]}")
        print(f"\nTOTAL MOVES: {total}")

        print("\n— Per-wave move counts & times —")
        for w in sorted(per_wave_times):
            t = per_wave_times[w]
            print(
                f"  wave_{w}_parallel.txt: {per_wave_counts[w]} moves "
                f"(expand {t['expand_sec']:.3f}s, count {t['count_sec']:.3f}s, total {t['total_sec']:.3f}s)"
            )
            
            
                
        # print(f"\nTOTAL MOVES: {total}")
        # print("\n— Per-wave move counts & times —")
        # for w in sorted(per_wave_times):
        #     t = per_wave_times[w]
        #     print(
        #         f"  wave_{w}_parallel.txt: {per_wave_counts[w]} moves "
        #         f"(expand {t['expand_sec']:.3f}s, count {t['count_sec']:.3f}s, total {t['total_sec']:.3f}s)"
        #     )
        # # ➜ add this return so the caller can log totals
        # return total, per_wave_counts, per_wave_times

            
            
        

    def run_pipeline(self, *, scramble: bool = False, mode: str, sector_count: int = 5):
        t_pipeline0 = perf_counter()
        if scramble:
            self.scramble_all_orbits()
            self.baseline_subcell_color = self.subcell_color.copy()
        if self.baseline_subcell_color is None:
            self.baseline_subcell_color = self.subcell_color.copy()

        self.sw.start("save_state")
        save_orbits_to_txt(self.subcell_color, self.M, str(self.run_dir / "cube_state.txt"))
        self.sw.stop("save_state")

        # 1) generate
        self.produce_full_plan(mode=mode)

        # 2) expand/count (same for all modes that produce cycles_wave*.txt)
        self.run_rearrange_and_count()

        # 3) optional 2D stage (only for the new mode)
        if mode == "sorting_network_2D":
            self.run_sorting_network_2D_stage(sector_count=sector_count)

        total_time = perf_counter() - t_pipeline0
        print(f"\n✅ solver_v2_6 finished in {total_time:.2f}s")
        print(f"Outputs in: {self.run_dir}")

        print("\n— Timing summary (seconds) —")
        print(f"  scramble            : {self.sw.get('scramble'):.3f}")
        print(f"  save_state          : {self.sw.get('save_state'):.3f}")
        print(f"  plan.total          : {self.sw.get('plan.total'):.3f}")
        print(f"    plan.compute      : {self.sw.get('plan.compute'):.3f}")
        print(f"    plan.write        : {self.sw.get('plan.write'):.3f}")
        print(f"  expand_count.total  : {self.sw.get('expand_count.total'):.3f}")
        print(f"  pipeline.total      : {total_time:.3f}")

    
    
    # def run_pipeline(self, *, scramble: bool = False, mode: str):
    #     t_pipeline0 = perf_counter()
    #     if scramble:
    #         self.scramble_all_orbits()
    #         self.baseline_subcell_color = self.subcell_color.copy()
    #     if self.baseline_subcell_color is None:
    #         self.baseline_subcell_color = self.subcell_color.copy()

    #     self.sw.start("save_state")
    #     save_orbits_to_txt(self.subcell_color, self.M, str(self.run_dir / "cube_state.txt"))
    #     self.sw.stop("save_state")

    #     self.produce_full_plan(mode=mode)

    #     # ⬇️ capture totals from rearrange step
    #     total_moves, per_wave_counts, per_wave_times = self.run_rearrange_and_count()

    #     total_time = perf_counter() - t_pipeline0
    #     print(f"\n✅ solver_v2_6 finished in {total_time:.2f}s")
    #     print(f"Outputs in: {self.run_dir}")

    #     print("\n— Timing summary (seconds) —")
    #     print(f"  scramble            : {self.sw.get('scramble'):.3f}")
    #     print(f"  save_state          : {self.sw.get('save_state'):.3f}")
    #     print(f"  plan.total          : {self.sw.get('plan.total'):.3f}")
    #     print(f"    plan.compute      : {self.sw.get('plan.compute'):.3f}")
    #     print(f"    plan.write        : {self.sw.get('plan.write'):.3f}")
    #     print(f"  expand_count.total  : {self.sw.get('expand_count.total'):.3f}")
    #     print(f"  pipeline.total      : {total_time:.3f}")

    #     # ➜ return benchmark-friendly summary
    #     return {
    #         "M": self.M,
    #         "moves": int(total_moves),
    #         "time_sec": float(total_time),
    #         "run_dir": str(self.run_dir),
    #     }


def benchmark_sweep():
    #Ms = [10, 20, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000]
    Ms = [700, 800, 900, 1000, 2000, 3000]

    log_path = RUN_DIR / "log.txt"
    with log_path.open("w", encoding="utf-8") as log:
        log.write("M,moves,time_sec,run_dir\n")
        for M in Ms:
            print(f"\n=== Benchmark M={M} ===")
            solver = SolverV2_6(M=M)
            # put each run in its own folder under the same timestamp root
            perM_dir = RUN_DIR / f"M_{M}"
            perM_dir.mkdir(parents=True, exist_ok=True)
            solver.run_dir = perM_dir

            # choose the mode you actually use; 'row_wise' is typical for generating cycles
            summary = solver.run_pipeline(scramble=True, mode="sorting_network")
            log.write(f"{summary['M']},{summary['moves']},{summary['time_sec']:.6f},{summary['run_dir']}\n")
            log.flush()
    print(f"\n📄 Benchmark log written to: {log_path}")


# ---------- timing helper ----------
class Stopwatch:
    def __init__(self): self.t = {}
    def start(self, k): self.t[k] = self.t.get(k, 0.0) - perf_counter()
    def stop(self, k):  self.t[k] = self.t.get(k, 0.0) + perf_counter()
    def add(self, k, dt): self.t[k] = self.t.get(k, 0.0) + dt
    def get(self, k): return self.t.get(k, 0.0)
    


def _parse_cycle_line_fast(line: str) -> Tuple[int,int,int,int,int] | None:
    """
    Fast (regex-free) parser for lines like:
      'orbit(1,4) - rot(1,3,27)'
    Returns (o1, o2, x, y, z) or None if the line isn't well-formed.
    """
    # quick filters to avoid extra work
    if "orbit(" not in line or ") - rot(" not in line:
        return None
    try:
        # orbit(1,4) - rot(1,3,27)
        # grab inside of orbit(...)
        i1 = line.index("orbit(") + 6
        i2 = line.index(")", i1)
        o1_s, o2_s = line[i1:i2].split(",", 1)

        # grab inside of rot(...)
        j1 = line.index("rot(", i2) + 4
        j2 = line.index(")", j1)
        x_s, y_s, z_s = line[j1:j2].split(",", 2)

        return (int(o1_s), int(o2_s), int(x_s), int(y_s), int(z_s))
    except Exception:
        return None


def _bucket_file(path: str) -> Dict[Tuple[int,int,int], List[Tuple[int,int]]]:
    """
    Parse a single cycles_wave*.txt file and return a local bucket:
      {(x,y,z): [(o1,o2), ...], ...}
    """
    local = defaultdict(list)
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                parsed = _parse_cycle_line_fast(line)
                if not parsed:
                    continue
                o1, o2, x, y, z = parsed
                local[(x, y, z)].append((o1, o2))
    except OSError:
        pass
    return local

def _parse_cycle_line_fast(line: str) -> Tuple[int,int,int,int,int] | None:
    if "orbit(" not in line or ") - rot(" not in line:
        return None
    try:
        i1 = line.index("orbit(") + 6
        i2 = line.index(")", i1)
        o1_s, o2_s = line[i1:i2].split(",", 1)

        j1 = line.index("rot(", i2) + 4
        j2 = line.index(")", j1)
        x_s, y_s, z_s = line[j1:j2].split(",", 2)

        return (int(o1_s), int(o2_s), int(x_s), int(y_s), int(z_s))
    except Exception:
        return None

def _bucket_file(path: str) -> Dict[Tuple[int,int,int], List[Tuple[int,int]]]:
    local = defaultdict(list)
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                parsed = _parse_cycle_line_fast(line)
                if parsed:
                    o1, o2, x, y, z = parsed
                    local[(x, y, z)].append((o1, o2))
    except OSError:
        pass
    return local

def extract_orbits_for_all_cycles(
    waves_dir: str,
    out_subdir: str = "orbit_points",
    max_workers: int | None = None
) -> list[str]:
    try:
        names = sorted(
            fn for fn in os.listdir(waves_dir)
            if fn.startswith("cycles_wave") and fn.endswith(".txt")
        )
    except FileNotFoundError:
        print(f"⚠️ No such directory: {waves_dir}")
        return []

    if not names:
        print(f"⚠️ No cycles_wave*.txt found in: {waves_dir}")
        return []

    paths = [os.path.join(waves_dir, fn) for fn in names]
    global_bucket: Dict[Tuple[int,int,int], List[Tuple[int,int]]] = defaultdict(list)

    with ThreadPoolExecutor(max_workers=max_workers) as pool:
        futures = {pool.submit(_bucket_file, p): p for p in paths}
        for fut in as_completed(futures):
            local = fut.result()
            for key, pairs in local.items():
                global_bucket[key].extend(pairs)

    out_dir = os.path.join(waves_dir, out_subdir)
    os.makedirs(out_dir, exist_ok=True)

    written: list[str] = []
    for (x, y, z) in sorted(global_bucket.keys()):
        pairs = global_bucket[(x, y, z)]
        pairs.sort(key=lambda t: (t[0], t[1]))
        out_path = os.path.join(out_dir, f"{x}_{y}_{z}.txt")
        with open(out_path, "w", encoding="utf-8") as out:
            for o1, o2 in pairs:
                out.write(f"{o1},{o2}\n")
        written.append(out_path)

    print(f"✅ Bucketed {sum(len(v) for v in global_bucket.values())} orbit pairs "
          f"across {len(global_bucket)} unique 3-cycles from {len(paths)} files.")
    print(f"✅ Wrote {len(written)} files to: {out_dir}")
    return written

def write_big_solution_file(sol_dir: str, run_dir: str, out_name: str = "solution.txt") -> str:
    try:
        parts = sorted(p for p in os.listdir(sol_dir) if p.endswith(".txt"))
    except FileNotFoundError:
        print(f"⚠️ Solutions directory not found: {sol_dir}")
        return ""

    if not parts:
        print(f"⚠️ No solution txts found in: {sol_dir}")
        return ""

    out_path = os.path.join(run_dir, out_name)
    with open(out_path, "w", encoding="utf-8") as out:
        for name in parts:
            p = os.path.join(sol_dir, name)
            try:
                with open(p, "r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            out.write(line + "\n")
            except OSError:
                continue

    print(f"🧩 Big solution written: {out_path}")
    return out_path

if __name__ == "__main__":
    M = 10
    solver = SolverV2_6(M=M)
    # Choose one of: "row_wise", "sorting_network", "sorting_network_2D"
    solver.run_pipeline(scramble=True, mode="row_wise")
                        #, sector_count=5)




# if __name__ == "__main__":
#     benchmark_sweep()