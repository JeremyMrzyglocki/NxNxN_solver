import io
import re
import sys
import glob
import time
import random
import itertools
import numpy as np
import sys as _sys
from PyQt5 import QtWidgets
from vispy import scene, app
from PyQt5.QtCore import QTimer
from collections import defaultdict
from PyQt5.QtWidgets import QMessageBox
from concurrent.futures import ProcessPoolExecutor

swap_plan_path = "cycles_wave*.txt" # This txt will contain the translation of the cube into cycles needed to solve it (in this case the non-diag centers)
table_path = "table.txt" # This is the table of cycles -> commutators
output_path = "solution.txt" # output file


# read the first cycles_waveX.txt files. They will have a format like: "orbit(3,2) - rot(1,3,14)"
# and the table.txt file will have a line with either that exact triplet or its inversion.
# E.g. you will find the line "1,3,14  algorithm: [2B' 3R2 2B,R']"
# Now we only take that algorithm after the colon and substitute the prefixes with the following formula:
# in that algorithm substitute the PREFIX "2" with (M - a + 1) and the "3" with (M - b + 1) where a and b are the first two numbers in the orbit
# and where M is the radius of the cube (not diameter).

# Recognize a move token like U, U', U2, 20R, 20R', 20R2, etc.
MOVE_RE = re.compile(r"\b\d*[RLFBUD](?:2|')?(?!\w)")
COMM_RE = re.compile(r"\[.*?\]")  # first commutator on a line

def apply_3cycle(arr, i, j, k):
    arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]

def count_correct(arr, target):
    return sum(1 for x, y in zip(arr, target) if x == y)

def greedy_3cycle_sort(target, initial, max_steps=10000, *, verbose=False):
    """
    Greedy 3-cycle sort with pruning, 2-cycle parity fix, and wavepacking.
    Returns:
        steps (int)
        moves (list[tuple[int,int,int,int]]) -> (i,j,k,wave), 0-based indices
    """

    tgt = list(target)
    arr = list(initial)
    n = len(arr)

    correct = [arr[i] == tgt[i] for i in range(n)]
    incorrect = [i for i in range(n) if not correct[i]]

    def gain_of(i, j, k):
        before = (correct[i]) + (correct[j]) + (correct[k])
        after  = (arr[k] == tgt[i]) + (arr[i] == tgt[j]) + (arr[j] == tgt[k])
        return after - before

    def apply_and_update(i, j, k):
        arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]
        for idx in (i, j, k):
            was = correct[idx]
            now = (arr[idx] == tgt[idx])
            correct[idx] = now
            if was and not now:
                if idx not in incorrect:
                    incorrect.append(idx)
            elif not was and now:
                if idx in incorrect:
                    incorrect.remove(idx)

    def try_two_cycle_fix():
        if len(incorrect) != 2:
            return None
        p, q = incorrect
        if arr[p] == tgt[q] and arr[q] == tgt[p]:
            candidates = [r for r in range(n) if r not in (p, q) and tgt[r] == tgt[p]]
            if not candidates:
                return None
            for r in candidates:
                if arr[r] == tgt[r]:
                    return (p, q, r)
            return (p, q, candidates[0])
        return None

    def canon_cycle(i, j, k):
        if i <= j and i <= k:   return (i, j, k)
        if j <= i and j <= k:   return (j, k, i)
        return (k, i, j)

    step = 0
    moves = []

    # --- wavepacking state ---
    wave_id = 1
    current_wave_used = set()   # indices already used in this wave

    while incorrect and step < max_steps:
        best_gain = 0
        best_move = None

        inc = incorrect
        if len(inc) >= 3:
            for x, y, z in itertools.combinations(inc, 3):
                for (i, j, k) in ((x, y, z), (x, z, y)):
                    if not (arr[k] == tgt[i] or arr[i] == tgt[j] or arr[j] == tgt[k]):
                        continue
                    g = gain_of(i, j, k)
                    if g > best_gain:
                        best_gain, best_move = g, (i, j, k)
                        if best_gain == 3:
                            break
                if best_gain == 3:
                    break

        if best_move is None or best_gain <= 0:
            fix = try_two_cycle_fix()
            if fix is not None:
                i, j, k = fix
                # wave decision based on index overlap
                if {i, j, k} & current_wave_used:
                    wave_id += 1
                    current_wave_used.clear()

                apply_and_update(i, j, k)
                step += 1
                ci, cj, ck = canon_cycle(i, j, k)
                moves.append((ci, cj, ck, wave_id))
                current_wave_used.update((ci, cj, ck))

                if verbose:
                    print(f"rot({ci},{cj},{ck}) -> {''.join(arr)}  (parity) wave {wave_id}")
                continue

            # Optional small fallback: try 2 incorrect + 1 correct (sampled)
            if len(inc) >= 2 and any(correct):
                correct_idxs = [r for r in range(n) if correct[r]]
                helpers = correct_idxs if len(correct_idxs) <= 16 else random.sample(correct_idxs, 16)
                for x, y in itertools.combinations(inc, 2):
                    for z in helpers:
                        for (i, j, k) in ((x, y, z), (x, z, y), (z, x, y)):
                            if not (arr[k] == tgt[i] or arr[i] == tgt[j] or arr[j] == tgt[k]):
                                continue
                            g = gain_of(i, j, k)
                            if g > best_gain:
                                best_gain, best_move = g, (i, j, k)
                    if best_move:
                        break

        if best_move is None or best_gain <= 0:
            if verbose:
                print(f"[stop] no improving triple; step={step}, correct={sum(correct)}")
            break

        i, j, k = best_move

        # wave decision based on index overlap
        if {i, j, k} & current_wave_used:
            wave_id += 1
            current_wave_used.clear()

        apply_and_update(i, j, k)
        step += 1
        ci, cj, ck = canon_cycle(i, j, k)
        moves.append((ci, cj, ck, wave_id))
        current_wave_used.update((ci, cj, ck))

        if verbose:
            print(f"rot({ci},{cj},{ck}) -> {''.join(arr)} (gain {best_gain}) wave {wave_id}")

    return step, moves


def invert_move(m: str) -> str: # inverts a move
    if m.endswith("'"): return m[:-1]
    if m.endswith("2"): return m
    return m + "'"

def expand_commutator(expr: str) -> str: # expr is like "[A_moves , B_moves]". Returns the 4‑part expansion: A B A^-1 B^-1
    inner = expr.strip()[1:-1]        
    try:
        a_part, b_part = inner.split(",", 1)
    except ValueError:
        return expr
    A = a_part.strip().split()
    B = b_part.strip().split()
    A_inv = [invert_move(m) for m in reversed(A)]
    B_inv = [invert_move(m) for m in reversed(B)]
    return " ".join(A + B + A_inv + B_inv)

def create_state_matrix(M): # This holds all of the information
    region_size = 2 * M
    rows, cols = 3 * region_size, 4 * region_size
    face_map = {
        (0, 1): ('W', 1), (1, 0): ('O', 5), (1, 1): ('G', 9),
        (1, 2): ('R', 13), (1, 3): ('B', 17), (2, 1): ('Y', 21),
    }
    quad_offset = {(0, 0): 0, (0, 1): 1, (1, 1): 2, (1, 0): 3}

    def ab(qr, qc, ip, jp):
        if (qr, qc) == (0, 0): return M - jp, M - ip
        if (qr, qc) == (0, 1): return M - ip, jp + 1
        if (qr, qc) == (1, 1): return jp + 1, ip + 1
        return ip + 1, M - jp

    matrix = []
    for i in range(rows):
        rr, ri = divmod(i, region_size)
        row = []
        for j in range(cols):
            rc, ci = divmod(j, region_size)
            key = (rr, rc)
            if key in face_map:
                _, base = face_map[key]
                br, bc = ri // M, ci // M
                k = base + quad_offset[(br, bc)]
                a, b = ab(br, bc, ri % M, ci % M)
                row.append((k, a, b))
            else:
                row.append(None)
        matrix.append(row)
    return matrix

def read_swap_plan(swap_plan_path):
    with open(swap_plan_path, 'r') as file:
        swap_plan = file.readlines()
    return [line.strip() for line in swap_plan if line.strip()]

def read_table(table_path):
    with open(table_path, 'r') as file:
        table = file.readlines()
    return [line.strip() for line in table if line.strip()]

def write_output(algorithms, output_path):
    with open(output_path, 'w') as file:
        for algo in algorithms:
            file.write(algo + '\n')
            
def swap_commutator_contents(alg: str) -> str: # Finds every '[A, B]' in the string (where A and B can be any sequence not containing ']'), and replaces it with '[B, A]', preserving all spacing/punctuation outside.
    def _swap(m: re.Match) -> str:
        inner = m.group(1)
        parts = inner.split(',', 1)
        if len(parts) != 2:
            return m.group(0)
        A, B = parts[0].strip(), parts[1].strip()
        return f"[{B}, {A}]"
    # match '[' then anything up to a comma+something up to the closing ']'
    return re.sub(r'\[([^\]]+?,[^\]]+?)\]', _swap, alg) # Write merged file
    
def count_moves(algorithm: str) -> int:
    """
    Count moves in an algorithm string.
    - Normal tokens (U, U', U2, 20R, ...) count as 1 each.
    - Inside [A,B], expand to A B A^-1 B^-1 => 2x total tokens inside.
    """
    total = 0

    # First handle commutators explicitly
    for comm in re.findall(r'\[(.*?)\]', algorithm):
        try:
            a_part, b_part = comm.split(",", 1)
        except ValueError:
            continue
        # Count tokens in A and B
        a_tokens = a_part.strip().split()
        b_tokens = b_part.strip().split()
        total += 2 * (len(a_tokens) + len(b_tokens))  # double
    # Remove commutators from string before counting remaining tokens
    algorithm_wo_comm = re.sub(r'\[.*?\]', '', algorithm)
    tokens = algorithm_wo_comm.split()
    total += len(tokens)
    return total


def translator():
    """
    Translate cycles_wave*.txt into algorithms using table.txt.
    - Writes per-wave outputs solution_wave{w}.txt
    - Writes merged output solution.txt
    - Prints total move count at the end
    """
    start = time.time()
    import glob, os
    from collections import defaultdict

    wave_files = sorted(
        glob.glob("cycles_wave*.txt"),
        key=lambda p: int(re.search(r"cycles_wave(\d+)\.txt", os.path.basename(p)).group(1))
    )
    if not wave_files:
        raise FileNotFoundError("No cycles_wave*.txt files found. Generate waves first.")

    table = read_table(table_path)
    per_wave_algorithms = defaultdict(list)
    merged_algorithms = []
    move_count_total = 0

    def wave_id_for(path: str) -> int:
        m = re.search(r"cycles_wave(\d+)\.txt", os.path.basename(path))
        return int(m.group(1))

    for path in wave_files:
        wave_id = wave_id_for(path)
        swap_plan = read_swap_plan(path)
        processed_algorithms = []

        for line in swap_plan:
            match = re.match(r'orbit\((\d+),(\d+)\) - rot\((\d+),(\d+),(\d+)\)', line)
            if not match:
                continue

            a, b, c, d, e = map(int, match.groups())
            if a == b or a == M or b == M:
                continue

            triplet = f"{c},{d},{e}"
            inverted_triplet = f"{c},{e},{d}"

            mapping = {'2': str(M - b + 1), '3': str(M - a + 1)}
            pattern = r'\b([23])(?=[A-Za-z])'

            for table_line in table:
                if triplet in table_line or inverted_triplet in table_line:
                    if ':' in table_line:
                        algorithm_part = table_line.split(':', 1)[1].strip()
                        algorithm_part = re.sub(pattern, lambda m: mapping[m.group(1)], algorithm_part)
                        if triplet in table_line:
                            algorithm_part = swap_commutator_contents(algorithm_part)
                        processed_algorithms.append(algorithm_part)
                        move_count_total += count_moves(algorithm_part)  # count moves here
                    break

        per_wave_algorithms[wave_id].extend(processed_algorithms)
        merged_algorithms.extend(processed_algorithms)

    # Write per-wave
    for w in sorted(per_wave_algorithms):
        out_w = f"solution_wave{w}.txt"
        write_output(per_wave_algorithms[w], out_w)

    # Write merged
    write_output(merged_algorithms, output_path)

    end = time.time()
    total = sum(len(v) for v in per_wave_algorithms.values())
    print(f"\n✅ Done. Saved {total} algorithm(s) across {len(per_wave_algorithms)} wave file(s).")
    print(f"Total moves (counting commutators doubled): {move_count_total}")
    print(f"translator() took {end - start:.2f} seconds")




def _compute_orbit_swap(args):
    a, b, subcell_color, M = args
    target = 'WWWWOOOOGGGGRRRRBBBBYYYY'
    initial = [subcell_color[(k, a, b)] for k in range(1, 25)]

    _, moves = greedy_3cycle_sort(target, initial, verbose=False)

    waves = defaultdict(list)
    for (i, j, k, w) in moves:
        waves[w].append(f"orbit({a},{b}) - rot({i+1},{j+1},{k+1})")
    return dict(waves)
    
    


# -----------------------------------------------------------------------------
# Main application
# -----------------------------------------------------------------------------

class CubeGridApp(QtWidgets.QMainWindow):
    def __init__(self, M=50, cell_size=1, gui=True):
        super().__init__()
        self.M = M
        self.cell_size = cell_size
        self.layout = create_state_matrix(M)

        colors = {
            **{k: 'W' for k in range(1, 5)},
            **{k: 'O' for k in range(5, 9)},
            **{k: 'G' for k in range(9, 13)},
            **{k: 'R' for k in range(13, 17)},
            **{k: 'B' for k in range(17, 21)},
            **{k: 'Y' for k in range(21, 25)},
        }
        self.initial_subcell_color = {
            (k, a, b): colors[k]
            for row in self.layout for cell in row if cell
            for k, a, b in [cell]
        }
        self.subcell_color = self.initial_subcell_color.copy()

        self.surface_cycles = {
            'U': [1, 2, 3, 4],
            'L': [5, 6, 7, 8],
            'F': [9, 10, 11, 12],
            'R': [13, 14, 15, 16],
            'B': [17, 18, 19, 20],
            'D': [21, 22, 23, 24]
        }

        if gui:
            self._build_ui()
            self._create_vispy_canvas()
            self.update_image()
            self.setWindowTitle("NxNxN Solver (n^2-type-pieces only)")
            self.showMaximized()
            
            
            

    def _build_ui(self):
        # Central widget and layout
        central = QtWidgets.QWidget()
        vbox = QtWidgets.QVBoxLayout(central)
        self.setCentralWidget(central)

        # Move & Scramble controls
        mf = QtWidgets.QHBoxLayout()
        self.move_entry = QtWidgets.QLineEdit()
        self.move_entry.setPlaceholderText("Moves (e.g. 1R U2 ...)")
        self.move_entry.textChanged.connect(lambda: self.on_scramble(live=True))
        mf.addWidget(self.move_entry)

        scramble_btn = QtWidgets.QPushButton("[1] Scramble")
        scramble_btn.clicked.connect(self.scramble_all_orbits)
        scramble_btn.setMinimumHeight(80)
        mf.addWidget(scramble_btn)

        plan_btn = QtWidgets.QPushButton("[2] Generate full\n swap-plan (cycles_waves.txt-files)")
        plan_btn.clicked.connect(self.produce_full_plan)
        plan_btn.setMinimumHeight(80)
        mf.addWidget(plan_btn)
        
        trans_btn = QtWidgets.QPushButton("[3] Translate full\n swap list (cycles) into moves")
        trans_btn.clicked.connect(self.translate_full_swap)
        trans_btn.setMinimumHeight(80)
        mf.addWidget(trans_btn)

        rearr_btn = QtWidgets.QPushButton("[4] Rearrange commutators\n by setup moves")
        #rearr_btn.clicked.connect(self.rearrange_commutators_by_setup_moves)
        rearr_btn.setMinimumHeight(80)
        mf.addWidget(rearr_btn)
        
        apply_btn = QtWidgets.QPushButton("[5] Apply found\n Solution on Cube")
        apply_btn.clicked.connect(self.apply_solution)
        apply_btn.setMinimumHeight(80)
        mf.addWidget(apply_btn)
        
        self.hide_edges_cb = QtWidgets.QCheckBox("Hide edges & diagonal centers")
        self.hide_edges_cb.toggled.connect(self.update_image)
        mf.addWidget(self.hide_edges_cb)
        

        vbox.addLayout(mf)

        # Orbit reader & translator
        of = QtWidgets.QHBoxLayout()
        of.addWidget(QtWidgets.QLabel("a:"))
        self.orbit_a = QtWidgets.QLineEdit(); self.orbit_a.setFixedWidth(40)
        of.addWidget(self.orbit_a)
        of.addWidget(QtWidgets.QLabel("b:"))
        self.orbit_b = QtWidgets.QLineEdit(); self.orbit_b.setFixedWidth(40)
        of.addWidget(self.orbit_b)
        read_btn = QtWidgets.QPushButton("Read Orbit")
        read_btn.clicked.connect(self.read_orbit); of.addWidget(read_btn)
        
        vbox.addLayout(of)

        # Solve output
        self.solve_output = QtWidgets.QTextEdit()
        self.solve_output.setFixedHeight(80)
        vbox.addWidget(self.solve_output)

        # Placeholder for VisPy canvas
        self.canvas_container = QtWidgets.QWidget()
        self.canvas_layout = QtWidgets.QHBoxLayout(self.canvas_container)
        vbox.addWidget(self.canvas_container)


    def _create_vispy_canvas(self):
        # Create VisPy SceneCanvas and embed in Qt
        self.canvas = scene.SceneCanvas(keys='interactive', bgcolor='black', parent=self.canvas_container)
        self.canvas.create_native()
        self.canvas.native.setParent(self.canvas_container)
        self.canvas_layout.addWidget(self.canvas.native)

        # Setup view and image
        self.view = self.canvas.central_widget.add_view()
        self.view.camera = scene.PanZoomCamera(aspect=1)
        rows, cols = len(self.layout), len(self.layout[0])
        height, width = rows * self.cell_size, cols * self.cell_size
        # dummy initial image
        data = np.zeros((height, width, 3), dtype=np.uint8)
        self.image = scene.visuals.Image(data, parent=self.view.scene, interpolation='nearest')

    def update_image(self):
        # Build numpy image from subcell_color
        rows, cols = len(self.layout), len(self.layout[0])
        h, w = rows * self.cell_size, cols * self.cell_size
        buf = np.zeros((h,w,3),dtype=np.uint8)
        # RGB map
        cmap = {'W': (255,255,255), 'O': (255,165,0), 'G': (0,128,0),
                'R': (255,0,0), 'B': (0,0,255), 'Y': (255,255,0)}
        for i, row in enumerate(self.layout):
            for j, cell in enumerate(row):
                if not cell:
                    continue
                k,a,b = cell
                clr = self.subcell_color[(k,a,b)]
                rgb = cmap[clr]
                y0, y1 = i*self.cell_size, (i+1)*self.cell_size
                x0, x1 = j*self.cell_size, (j+1)*self.cell_size
                buf[y0:y1, x0:x1, :] = rgb
        buf = buf[:, ::-1, :]
        self.image.set_data(buf)
        # Reset camera scale on first draw
        if hasattr(self, '_first_draw') is False:
            self.view.camera.set_range(x=(w, 0), y=(h, 0))
            self._first_draw = True
        self.canvas.update()
        

    def produce_full_plan(self):
        t0 = time.time()

        args = [
            (a, b, self.initial_subcell_color, self.M)
            for a in range(1, self.M+1)
            for b in range(1, self.M+1)
        ]

        from collections import defaultdict
        wave_lines = defaultdict(list)  # wave -> list[str]

        with ProcessPoolExecutor() as exe:
            for wave_dict in exe.map(_compute_orbit_swap, args):
                for w, lines in wave_dict.items():
                    wave_lines[w].extend(lines)

        # write one file per wave (sorted by wave id)
        written = []
        for w in sorted(wave_lines):
            fname = f"cycles_wave{w}.txt"
            with open(fname, "w") as f:
                f.write("\n".join(wave_lines[w]))
                f.write("\n")
            written.append(fname)

        dt = time.time() - t0
        self.solve_output.setPlainText(
            f"Saved {len(written)} wave file(s): {', '.join(written)} in {dt:.2f}s"
        )


    def translate_full_swap(self):
        t0 = time.time()
        translator()
        dt = time.time() - t0
        self.solve_output.setPlainText(f"Translated full swap-list in {dt:.2f} s")

    def scramble_all_orbits(self):
        start = time.time()
        base = ['W']*4 + ['O']*4 + ['G']*4 + ['R']*4 + ['B']*4 + ['Y']*4
        for a in range(1, self.M+1):
            for b in range(1, self.M+1):
                lst = base[:]
                random.shuffle(lst)
                for k in range(1,25):
                    self.subcell_color[(k,a,b)] = lst[k-1]
        self.initial_subcell_color = self.subcell_color.copy()
        self.update_image()
        end = time.time()
        print(f"scramble_all_orbits() took {end - start:.2f} seconds")
        self.solve_output.setPlainText(f"Scramble done in {end - start:.2f} s")

    def read_orbit(self):
        t0 = time.time()
        try:
            a, b = int(self.orbit_a.text()), int(self.orbit_b.text())
        except ValueError:
            QMessageBox.warning(self, "Input error", "Please enter valid integers for a and b.")
            return
        arr = [self.subcell_color.get((k, a, b), '?') for k in range(1, 25)]
        self.orbit_output.setText(''.join(arr))
        dt = time.time() - t0
        self.solve_output.setPlainText(f"Read orbit (a={a}, b={b}) in {dt*1000:.1f} ms")

    def on_scramble(self, live=False):
        text = self.move_entry.text().upper()
        text = re.sub(
            r"\[([^\]]+)\]",
            lambda m: expand_commutator(m.group(0)),
            text
        )
        tokens = text.split() # tokenize input string
        moves = []
        for mv in tokens:
            mv = mv.strip()
            if not mv:
                continue
            cnt = 1
            if mv.endswith("2"):
                cnt, mv = 2, mv[:-1]
            elif mv.endswith("'"):
                cnt, mv = 3, mv[:-1]

            if not mv:
                continue

            face, prefix = mv[-1], mv[:-1]
            if face not in "RLFBUD":
                continue

            if prefix == "":
                p, spec = 1, True    # outer slice → rotate face
            elif prefix.isdigit():
                p, spec = int(prefix), False
            else:
                continue

            moves.append((face, p, cnt, spec))

        # apply on top of the scrambled/base state
        self.subcell_color = self.initial_subcell_color.copy()
        for face, p, cnt, spec in moves:
            fn = getattr(self, f"apply_p{face}")
            for _ in range(cnt):
                fn(p)
            if spec:
                self._rotate_surface(face, cnt)

        self.update_image()

        if not live:
            self.initial_subcell_color = self.subcell_color.copy()
    def apply_solution(self):
        """ Read solution.txt, split into 100-line chunks, and animate them
        one chunk at a time, timing the entire animation. """
        try:
            with open("solution.txt", "r") as f:
                lines = [l.strip() for l in f if l.strip()]
                if not lines:
                    QMessageBox.information(self, "No moves", "solution.txt is empty.")
                    return
        except IOError as e:
            QMessageBox.critical(self, "File Error", f"Could not read solution.txt:\n{e}")
            return

        # break into 1000-line chunks
        chunk_size = 1000
        self._chunks = [" ".join(lines[i:i+chunk_size])
                        for i in range(0, len(lines), chunk_size)]
        self._chunk_idx = 0

        # record when we started
        self._animation_start = time.time()
        if hasattr(self, "_timer"):
            self._timer.stop()
        else:
            self._timer = QTimer(self)
            self._timer.timeout.connect(self._apply_next_chunk)

        self._timer.start(20)  # 20 ms per frame
        
    def _apply_next_chunk(self):
        if self._chunk_idx >= len(self._chunks):
            self._timer.stop()
            total = time.time() - self._animation_start
            self.solve_output.append(f"✅ Animation complete in {total:.2f} s")
            return

        seq = self._chunks[self._chunk_idx]
        self.move_entry.setText(seq)
        self.on_scramble(live=False)

        self.solve_output.setPlainText(
            f"Chunk {self._chunk_idx+1}/{len(self._chunks)} — "
            f"{len(seq.split())} moves"
        )
        self._chunk_idx += 1


    def _rotate_surface(self, face, count):
        cycle = self.surface_cycles[face]
        for _ in range(count):
            old = self.subcell_color.copy()
            for k in cycle:
                prev = cycle[(cycle.index(k)-1) % len(cycle)]
                for a in range(1, self.M+1):
                    for b in range(1, self.M+1):
                        key = (prev,a,b)
                        if key in old:
                            self.subcell_color[(k,a,b)] = old[key]

    def apply_move(self, p, cycle1, cycle2, fixed1='b', fixed2='a'):
        b_t = self.M + 1 - p
        a_t = b_t
        old = self.subcell_color.copy()
        for i,k in enumerate(cycle1):
            prev = cycle1[i-1]
            if fixed1=='b':
                for a in range(1, self.M+1):
                    key=(prev,a,b_t)
                    if key in old:
                        self.subcell_color[(k,a,b_t)] = old[key]
            else:
                for b in range(1, self.M+1):
                    key=(prev,a_t,b)
                    if key in old:
                        self.subcell_color[(k,a_t,b)] = old[key]
        for i,k in enumerate(cycle2):
            prev = cycle2[i-1]
            if fixed2=='b':
                for a in range(1, self.M+1):
                    key=(prev,a,b_t)
                    if key in old:
                        self.subcell_color[(k,a,b_t)] = old[key]
            else:
                for b in range(1, self.M+1):
                    key=(prev,a_t,b)
                    if key in old:
                        self.subcell_color[(k,a_t,b)] = old[key]

    def apply_pR(self, p): self.apply_move(p, [2,20,22,10], [11,3,17,23], 'b','a')
    def apply_pL(self, p): self.apply_move(p, [1,9,21,19], [4,12,24,18], 'a','b')
    def apply_pF(self, p): self.apply_move(p, [4,13,22,7], [3,16,21,6], 'a','b')
    def apply_pB(self, p): self.apply_move(p, [2,5,24,15], [1,8,23,14], 'a','b')
    def apply_pU(self, p): self.apply_move(p, [9,5,17,13], [10,6,18,14], 'b','a')
    def apply_pD(self, p): self.apply_move(p, [12,16,20,8], [11,15,19,7], 'a','b')
    
    
    
def process_algorithms(input_path, m, output_path):
    app.use_app('pyqt5')
    qapp = QtWidgets.QApplication(sys.argv)
    with open(input_path, 'r') as infile, open(output_path, 'w') as outfile:
        for line in infile:
            algorithm = line.strip() 
            if not algorithm:
                continue

            cube = CubeGridApp(M=m, cell_size=1)             # Create fresh cube instance for each algorithm
            cube.move_entry.setText(algorithm)             # Apply the algorithm
            cube.on_scramble(live=False)
            cube.produce_full_plan()            # Run full plan (writes to 'cycles.txt')
            with open("cycles.txt", "r") as plan_file:             # Read the generated file
                output = plan_file.read()
            outfile.write(f"algorithm: {algorithm}\n\noutput:\n{output.strip()}\n\n")      # Format and write results
    qapp.quit()

if __name__ == '__main__':
    M = 50 # half of N where N is the size of the cube (N=2M). Only odd cubes supported for now
    app.use_app('pyqt5')
    qapp = QtWidgets.QApplication(sys.argv)
    win = CubeGridApp(M, cell_size=1)
    sys.exit(qapp.exec_())
    
    