import io
import re
import sys
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

# File paths
swap_plan_path = "cycles.txt" # This txt will contain the translation of the cube into cycles needed to solve it (in this case the non-diag centers)
table_path = "table.txt" # This is the table of cycles -> commutators
output_path = "solution.txt" # output file


# read the first cycles.txt file. It will have a format like: "orbit(3,2) - rot(1,3,14)"
# and the table.txt file will have a line with either that exact triplet or its inversion.
# E.g. you will find the line "1,3,14  algorithm: [2B' 3R2 2B,R']"
# Now we only take that algorithm after the colon and substitute the prefixes with the following formula:
# in that algorithm substitute the PREFIX "2" with (M - a + 1) and the "3" with (M - b + 1) where a and b are the first two numbers in the orbit
# and where M is the radius of the cube (not diameter).

# Recognize a move token like U, U', U2, 20R, 20R', 20R2, etc.
MOVE_RE = re.compile(r"\b\d*[RLFBUD](?:2|')?\b")
COMM_RE = re.compile(r"\[.*?\]")  # first commutator on a line

def apply_3cycle(arr, i, j, k):
    arr[i], arr[j], arr[k] = arr[k], arr[i], arr[j]

def count_correct(arr, target):
    return sum(1 for x, y in zip(arr, target) if x == y)

def greedy_3cycle_sort(target, initial, max_steps=10000): # This algorithm is one potential spot of optimization. It averages about 8 cycles per scrambled orbit.
    arr = initial.copy()
    n = len(arr)
    triples = []
    for i, j, k in itertools.combinations(range(n), 3):
        triples.append((i, j, k))
        triples.append((i, k, j))
    step = 0
    while arr != target and step < max_steps:
        curr_correct = count_correct(arr, target)
        best_gain = 0
        best_move = None
        for (i, j, k) in triples:
            arr2 = arr.copy()
            apply_3cycle(arr2, i, j, k)
            gain = count_correct(arr2, target) - curr_correct
            if gain > best_gain:
                best_gain = gain
                best_move = (i, j, k)
        if best_move is None:
            break
        i, j, k = best_move
        apply_3cycle(arr, i, j, k)
        step += 1
        print(f"rot({i},{j},{k}) -> {''.join(arr)}")
    return step

def invert_move(m: str) -> str: # inverts a move
    if m.endswith("'"):
        return m[:-1]
    if m.endswith("2"):
        return m
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
    N = 2
    region_size = N * M
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
    return re.sub(r'\[([^\]]+?,[^\]]+?)\]', _swap, alg)

def translator(): # translate the cylces.txt into usable algorithms (runs in O(n)-time and - just like generating the cylces for the orbits - parallelized)
    start = time.time()
    swap_plan = read_swap_plan(swap_plan_path)
    table = read_table(table_path)
    processed_algorithms = []

    for line in swap_plan:
        match = re.match(r'orbit\((\d+),(\d+)\) - rot\((\d+),(\d+),(\d+)\)', line)
        if match:
            a, b, c, d, e = map(int, match.groups())
            # some skipping conditions:
            if a == b or a == M or b == M:
                print(f"Skipping line due to conditions: {line}")
                print("....")
                continue

            triplet = f"{c},{d},{e}"
            inverted_triplet = f"{c},{e},{d}"
            print(f"Triplet: {triplet}, Inverted Triplet: {inverted_triplet}")

            # prepare a single-pass replacement map for "2" and "3"
            mapping = {
                '2': str(M - b + 1),
                '3': str(M - a + 1),
            }
            pattern = r'\b([23])(?=[A-Za-z])'

            for table_line in table:
                if triplet in table_line:
                    print(f"Found matching line in table: {table_line}")
                    if ':' in table_line:
                        algorithm_part = table_line.split(':', 1)[1].strip()
                        # apply both replacements in one pass
                        algorithm_part = re.sub(pattern,
                                                lambda m: mapping[m.group(1)],
                                                algorithm_part)
                        algorithm_part = swap_commutator_contents(algorithm_part)

                        print(f"Processed algorithm: {algorithm_part} for triplet {triplet}")
                        processed_algorithms.append(algorithm_part)
                    else:
                        print(f"Warning: No colon found in line: {table_line}")
                    break

                elif inverted_triplet in table_line:
                    print(f"Found matching inverted line in table: {table_line}")
                    if ':' in table_line:
                        algorithm_part = table_line.split(':', 1)[1].strip()
                        # apply both replacements in one pass
                        algorithm_part = re.sub(pattern,
                                                lambda m: mapping[m.group(1)],
                                                algorithm_part)
                        print(f"Processed inverted algorithm: {algorithm_part} for inverted triplet {inverted_triplet}")
                        processed_algorithms.append(algorithm_part)
                    else:
                        print(f"Warning: No colon found in line: {table_line}")
                    break

            print("....")
    write_output(processed_algorithms, output_path)
    end = time.time()
    print(f"\n✅ Done. Saved {len(processed_algorithms)} algorithm(s) to {output_path}.")
    print(f"translator() took {end - start:.2f} seconds")

def _compute_orbit_swap(args):
    """ Worker for one orbit: runs greedy_3cycle_sort, captures the
    printed rot(...) lines, and formats them as orbit(...) entries. """
    a, b, subcell_color, M = args
    target = ['W']*4 + ['O']*4 + ['G']*4 + ['R']*4 + ['B']*4 + ['Y']*4

    # build the starting 1×24 list for this orbit
    initial = [subcell_color[(k, a, b)] for k in range(1,25)]

    # capture stdout from greedy_3cycle_sort
    buf = io.StringIO()
    old = _sys.stdout
    _sys.stdout = buf
    greedy_3cycle_sort(target, initial)
    _sys.stdout = old

    out = []
    for line in buf.getvalue().splitlines():
        if not line.startswith("rot("):
            continue
        nums = line[len("rot("):].split(")")[0].split(",")
        i, j, k = [int(n)+1 for n in nums]
        out.append(f"orbit({a},{b}) - rot({i},{j},{k})")
    return out

# -----------------------------------------------------------------------------
# Main application
# -----------------------------------------------------------------------------

class CubeGridApp(QtWidgets.QMainWindow):
    def __init__(self, M=50, cell_size=30, gui=True):
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

    def rearrange_commutators_by_setup_moves(self):
        """
        Regroup solution.txt so that:
        - First block: lines with NO setup moves outside the brackets -> 'NONE'
        - Then: blocks by the EXACT outer token (including any numeric slice prefix and postfix),
                using the LEFT token if both sides exist.
        - Last: lines without brackets at all -> 'NO_BRACKET'
        """
        src = output_path
        dst = src[:-4] + "_grouped_by_setup.txt" if src.endswith(".txt") else src + "_grouped_by_setup.txt"

        try:
            with open(src, "r") as f:
                lines = [ln.strip() for ln in f if ln.strip()]
        except Exception as e:
            QMessageBox.critical(self, "File Error", f"Could not read {src}:\n{e}")
            return

        def key_for(line: str) -> str:
            m = COMM_RE.search(line)
            if not m:
                return "NO_BRACKET"
            pre = line[:m.start()]
            post = line[m.end():]
            left_tokens = MOVE_RE.findall(pre)
            right_tokens = MOVE_RE.findall(post)
            left = left_tokens[-1] if left_tokens else None
            right = right_tokens[0] if right_tokens else None
            if not left and not right:
                return "NONE"
            # EXACT token as written (e.g., 'R', "R'", 'R2', '16R2', ...)
            return left or right

        from collections import defaultdict
        buckets = defaultdict(list)
        order = []  # preserve first-seen order of groups

        for ln in lines:
            k = key_for(ln)
            if k not in buckets:
                order.append(k)
            buckets[k].append(ln)

        # Build final order: NONE first, then first-seen tokens (excluding NONE/NO_BRACKET),
        # finally NO_BRACKET if present.
        final_keys = []
        if "NONE" in buckets:
            final_keys.append("NONE")
        final_keys += [k for k in order if k not in ("NONE", "NO_BRACKET")]
        if "NO_BRACKET" in buckets:
            final_keys.append("NO_BRACKET")

        try:
            with open(dst, "w") as out:
                for k in final_keys:
                    out.write(f"# --- {k} ({len(buckets[k])}) ---\n")
                    out.write("\n".join(buckets[k]))
                    out.write("\n\n")
        except Exception as e:
            QMessageBox.critical(self, "File Error", f"Could not write {dst}:\n{e}")
            return

        self.solve_output.setPlainText(
            f"Grouped {sum(map(len, buckets.values()))} line(s) into {len(final_keys)} block(s). Saved to {dst}"
        )



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
        mf.addWidget(scramble_btn)

        plan_btn = QtWidgets.QPushButton("[2] Generate full swap-plan (cycles.txt)")
        plan_btn.clicked.connect(self.produce_full_plan)
        mf.addWidget(plan_btn)

        # NEW button
        rearr_btn = QtWidgets.QPushButton("[4] Rearrange commutators by setup moves")
        rearr_btn.clicked.connect(self.rearrange_commutators_by_setup_moves)
        mf.addWidget(rearr_btn)
        
        apply_btn = QtWidgets.QPushButton("[5] Apply found Solution on Cube")
        apply_btn.clicked.connect(self.apply_solution)
        mf.addWidget(apply_btn)

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
        
        trans_btn = QtWidgets.QPushButton("[3] Translate full swap list (cycles) into moves")
        trans_btn.clicked.connect(self.translate_full_swap); of.addWidget(trans_btn)
        self.orbit_output = QtWidgets.QLineEdit(); of.addWidget(self.orbit_output)
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
        fname = "cycles.txt"
        # prepare arguments for every (a,b)
        # pass a copy of the initial color-mapping and M to each worker
        args = [
            (a, b, self.initial_subcell_color, self.M)
            for a in range(1, self.M+1)
            for b in range(1, self.M+1)
        ]

        # run in parallel
        with ProcessPoolExecutor() as exe:
            # map() returns results in “args” order
            results = exe.map(_compute_orbit_swap, args)
            # write everything out as soon as each worker completes
            with open(fname, "w") as f:
                for orbit_moves in results:
                    for line in orbit_moves:
                        f.write(line + "\n")

        dt = time.time() - t0
        self.solve_output.setPlainText(f"Full swap-plan saved to {fname} in {dt:.2f}s")

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

        # break into 100-line chunks
        chunk_size = 100
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
            with open("cylces.txt", "r") as plan_file:             # Read the generated file
                output = plan_file.read()
            outfile.write(f"algorithm: {algorithm}\n\noutput:\n{output.strip()}\n\n")      # Format and write results
    qapp.quit()

if __name__ == '__main__':
    M = 50 # Radius of the cube. Not diameter
    app.use_app('pyqt5')
    qapp = QtWidgets.QApplication(sys.argv)
    win = CubeGridApp(M, cell_size=1)
    sys.exit(qapp.exec_())
    
    