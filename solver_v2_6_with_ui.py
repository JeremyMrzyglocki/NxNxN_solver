import sys, re, time
from pathlib import Path
import numpy as np
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import (QApplication, QLineEdit, QPushButton, QTextEdit, QHBoxLayout, QVBoxLayout, QWidget, QCheckBox, QButtonGroup)
from PyQt5.QtCore import QTimer
from vispy import scene, app as vispy_app
from solver_v2_6 import SolverV2_6 as Solver
import imageio.v3 as iio

# ---- color dictionaries (same as your solver) ----
COLOR_ID = {'W': 0, 'O': 1, 'G': 2, 'R': 3, 'B': 4, 'Y': 5}
ID_COLOR = ['W', 'O', 'G', 'R', 'B', 'Y']
CUBE_RGB = {'W': (255,255,255), 'O': (255,165,0), 'G': (0,128,0), 'R': (255,0,0), 'B': (0,0,255), 'Y': (255,255,0)}

def create_state_matrix(M):
    region_size = 2 * M
    rows, cols = 3 * region_size, 4 * region_size
    face_map = {
        (0, 1): 1,  (1, 0): 5,  (1, 1): 9,
        (1, 2): 13, (1, 3): 17, (2, 1): 21,
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
                base = face_map[key]
                br, bc = ri // M, ci // M
                k = base + quad_offset[(br, bc)]
                a, b = ab(br, bc, ri % M, ci % M)
                row.append((k, a, b))
            else:
                row.append(None)
        matrix.append(row)
    return matrix


class SolverUI(QtWidgets.QMainWindow):
    def __init__(self, M, seed=123456, cell_size=1):
        super().__init__()
        self.setWindowTitle("NxNxN Solver v2_6 — minimal UI")
        self.M = M
        self.seed = seed
        self.cell_size = cell_size
        self.selected_mode = 'row_wise'

        # ----- solver instance -----
        self.solver = Solver(M=M, seed=seed)
        self.baseline_subcell_color = None

        # for animation playback
        self._moves = []
        self._move_idx = 0
        self._moves_per_tick = 50  # tune for your machine
        self._anim_running = False

        # cube layout mapping for rendering
        self.layout = create_state_matrix(M)

        # UI
        self._build_ui()
        self._create_vispy_canvas()
        self.update_image()

        # timer for animation
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._apply_next_chunk)

        self.showMaximized()

    def on_solver(self, mode):
        return self.on_run_solver(mode)

    # ---------- UI scaffold ----------
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        vbox = QVBoxLayout(central)
        vbox.setContentsMargins(10, 10, 10, 10)
        vbox.setSpacing(8)

        # Row 0: mode selection (exclusive checkboxes)
        row_modes = QHBoxLayout()
        self.cb_row    = QCheckBox("row_wise")
        self.cb_sort1D = QCheckBox("sorting_network")
        self.cb_sort2D = QCheckBox("sorting_network_2D")

        self.mode_group = QButtonGroup(self)
        self.mode_group.setExclusive(True)
        self.mode_group.addButton(self.cb_row, 0)
        self.mode_group.addButton(self.cb_sort1D, 1)
        self.mode_group.addButton(self.cb_sort2D, 2)
        self.cb_row.setChecked(True)

        def _on_mode_clicked(_checked):
            if self.cb_row.isChecked():
                self.selected_mode = 'row_wise'
            elif self.cb_sort1D.isChecked():
                self.selected_mode = 'sorting_network'
            elif self.cb_sort2D.isChecked():
                self.selected_mode = 'sorting_network_2D'

        self.cb_row.toggled.connect(_on_mode_clicked)
        self.cb_sort1D.toggled.connect(_on_mode_clicked)
        self.cb_sort2D.toggled.connect(_on_mode_clicked)

        row_modes.addWidget(self.cb_row)
        row_modes.addWidget(self.cb_sort1D)
        row_modes.addWidget(self.cb_sort2D)
        row_modes.addStretch(1)
        vbox.addLayout(row_modes)



        # Row 1: moves + run button
        row1 = QHBoxLayout()
        row1.setSpacing(8)

        self.move_entry = QLineEdit()
        self.move_entry.setPlaceholderText("Moves (e.g. 1R U2 3F' …). Live-applied on the cube.")
        self.move_entry.textChanged.connect(lambda _: self.on_moves_changed(live=True))
        self.move_entry.setMinimumHeight(36)
        row1.addWidget(self.move_entry, 3)
        vbox.addLayout(row1)

        # Row 2: basic actions
        
        row2 = QHBoxLayout()
        row2.setSpacing(8)

        self.scramble_btn = QPushButton("[1.] Scramble")
        self.scramble_btn.clicked.connect(self.on_scramble)
        row2.addWidget(self.scramble_btn)

        self.run_btn = QPushButton("[2.] Apply solver_v2_6 (parallelized 1D or sorting_network_1D or sorting_network_2D)")
        self.run_btn.clicked.connect(lambda _: self.on_run_solver(self.selected_mode))  # or on_solver
        row2.addWidget(self.run_btn)

        self.play_parallel_btn = QPushButton("[3.] Play solution in UI (recommended only for M <= 30)")
        self.play_parallel_btn.setToolTip("Play wave_*_parallel.txt from latest run_dir")
        self.play_parallel_btn.clicked.connect(lambda: self.play_solution())
        row2.addWidget(self.play_parallel_btn)

        row2.addStretch(1)
        vbox.addLayout(row2)

        # Output box
        self.output = QTextEdit()
        self.output.setFixedHeight(130)
        vbox.addWidget(self.output)

        # Canvas container
        self.canvas_container = QWidget()
        self.canvas_layout = QHBoxLayout(self.canvas_container)
        self.canvas_layout.setContentsMargins(0, 0, 0, 0)
        vbox.addWidget(self.canvas_container, 1)

    # ---------- VisPy canvas ----------
    def _create_vispy_canvas(self):
        self.canvas = scene.SceneCanvas(keys='interactive', bgcolor='black', parent=self.canvas_container)
        self.canvas.create_native()
        self.canvas.native.setParent(self.canvas_container)
        self.canvas_layout.addWidget(self.canvas.native)

        self.view = self.canvas.central_widget.add_view()
        self.view.camera = scene.PanZoomCamera(aspect=1)

        rows, cols = len(self.layout), len(self.layout[0])
        height, width = rows * self.cell_size, cols * self.cell_size
        data = np.zeros((height, width, 3), dtype=np.uint8)
        self.image = scene.visuals.Image(data, parent=self.view.scene, interpolation='nearest')

        self.view.camera.set_range(x=(width, 0), y=(height, 0))

    # ---------- rendering ----------
    def update_image(self):
        cf = self.solver.subcell_color
        rows, cols = len(self.layout), len(self.layout[0])
        h, w = rows * self.cell_size, cols * self.cell_size

        buf = np.zeros((h, w, 3), dtype=np.uint8)
        for i, row in enumerate(self.layout):
            for j, cell in enumerate(row):
                if not cell:
                    continue
                k, a, b = cell
                clr = cf[(k, a, b)]
                rgb = CUBE_RGB[clr]
                y0, y1 = i*self.cell_size, (i+1)*self.cell_size
                x0, x1 = j*self.cell_size, (j+1)*self.cell_size
                buf[y0:y1, x0:x1, :] = rgb

        # flip (as before)
        buf = buf[:, ::-1, :]
        # store the final (displayed) frame for saving
        self._last_frame = buf

        self.image.set_data(buf)
        self.canvas.update()


    def _tokenize_solution_text(self, text: str):
        text = text.upper()
        text = re.sub(r"\[([^\]]+)\]", lambda m: self._expand_commutator(m.group(0)), text)
        text = text.replace("(", " ").replace(")", " ")
        return [t for t in text.split() if t]

    def _decode_move_token(self, tok: str):
        cnt = 1
        if tok.endswith("2"):
            cnt, tok = 2, tok[:-1]
        elif tok.endswith("'"):
            cnt, tok = 3, tok[:-1]
        if not tok:
            return None
        face, prefix = tok[-1], tok[:-1]
        if face not in "RLFBUD":
            return None
        if prefix == "":
            p, spec = 1, True
        elif prefix.isdigit():
            p, spec = int(prefix), False
        else:
            return None
        return (face, p, cnt, spec)

    @staticmethod
    def _expand_commutator(expr: str) -> str:
        inner = expr.strip()[1:-1]
        try:
            a_part, b_part = inner.split(",", 1)
        except ValueError:
            return expr
        A = a_part.strip().split()
        B = b_part.strip().split()
        def invert(m):
            if m.endswith("'"): return m[:-1]
            if m.endswith("2"): return m
            return m + "'"
        A_inv = [invert(m) for m in reversed(A)]
        B_inv = [invert(m) for m in reversed(B)]
        return " ".join(A + B + A_inv + B_inv)

    # ---------- live move textbox ----------
    def on_moves_changed(self, live=True):
        text = self.move_entry.text()
        tokens = self._tokenize_solution_text(text)

        moves = []
        for t in tokens:
            m = self._decode_move_token(t)
            if m:
                moves.append(m)

        # apply on top of baseline
        if self.baseline_subcell_color is None:
            # if not scrambled yet, baseline is the current
            self.baseline_subcell_color = self.solver.subcell_color.copy()
        self.solver.subcell_color = self.baseline_subcell_color.copy()

        for face, p, cnt, spec in moves:
            fn = getattr(self, f"apply_p{face}")
            for _ in range(cnt):
                fn(p)
                if spec:
                    self._rotate_surface(face, 1)
        self.update_image()

    # ---------- scramble / reset ----------
    def on_scramble(self):
        self.solver.scramble_all_orbits()
        self.baseline_subcell_color = self.solver.subcell_color.copy()
        self.output.append("Scrambled.")
        self.update_image()

    # ---------- run solver (pipeline) ----------
    def on_run_solver(self, mode=None):
        if mode is None:
            mode = getattr(self, 'selected_mode', 'row_wise')
        self.output.append("▶️  Running solver_v2_6 pipeline …")
        QApplication.processEvents()

        if self.baseline_subcell_color is not None:
            self.solver.subcell_color = self.baseline_subcell_color.copy()
            self.solver.baseline_subcell_color = self.baseline_subcell_color.copy()
        t0 = time.time()
        try:
            self.solver.run_pipeline(scramble=False, mode=mode, sector_count=6)  
            self.output.append(f"✅ Done. Outputs in: {self.solver.run_dir}")
        except Exception as e:
            self.output.append(f"✖ solver failed: {e}")
        finally:
            self.output.append(f"Elapsed: {time.time() - t0:.2f}s")
            
    # ---------- solution playback ----------
    def play_solution(self, mode=None):
        mode = self.selected_mode if mode is None else mode
        run_dir = Path(self.solver.run_dir)

        if mode == 'sorting_network_2D':
            files = [run_dir / "solution_2D.txt"] if (run_dir / "solution_2D.txt").exists() else []
            empty_msg = "No solution.txt file in run_dir."
        else:
            wave_re = re.compile(r"wave_(\d+)_parallel\.txt$")
            waves = sorted(
                (p for p in run_dir.glob("wave_*_parallel.txt") if wave_re.search(p.name)),
                key=lambda p: int(wave_re.search(p.name).group(1))
            )
            files = waves
            empty_msg = "No wave_*_parallel.txt files in run_dir."

        if not files:
            self.output.append(empty_msg)
            return
        self._start_animation_from_files(files)


    def _start_animation_from_files(self, files):
        try:
            combined_text = "\n".join(Path(p).read_text(encoding="utf-8") for p in files)
        except OSError as e:
            self.output.append(f"Could not read solution file(s): {e}")
            return

        tokens = self._tokenize_solution_text(combined_text)
        moves = []
        for t in tokens:
            m = self._decode_move_token(t)
            if m:
                moves.append(m)

        if not moves:
            self.output.append("No moves found in solution files.")
            return

        # Start from current baseline
        if self.baseline_subcell_color is None:
            self.baseline_subcell_color = self.solver.subcell_color.copy()

        self.solver.subcell_color = self.baseline_subcell_color.copy()

        self._moves = moves
        self._move_idx = 0
        self._anim_running = True
        self._animation_start = time.time()

        if self._timer.isActive():
            self._timer.stop()
        self._timer.start(1)  # ms per tick
        self.output.append(f"▶️  Playing {len(self._moves)} moves …")


    def _apply_next_chunk(self):
        if self._move_idx >= len(self._moves):
            self._timer.stop()
            self._anim_running = False
            total = time.time() - self._animation_start
            # keep solved state as new baseline
            self.baseline_subcell_color = self.solver.subcell_color.copy()
            self.update_image()
            self.output.append(f"✅ Applied {len(self._moves)} moves in {total:.2f}s")
            # finalize capture
            if getattr(self, '_capture_enabled', False):
                self._capture_enabled = False
            return

        end = min(self._move_idx + self._moves_per_tick, len(self._moves))
        for face, p, cnt, spec in self._moves[self._move_idx:end]:
            fn = getattr(self, f"apply_p{face}")
            for _ in range(cnt):
                fn(p)

                # capture logic per atomic move
                if getattr(self, '_capture_enabled', False):
                    self._moves_applied += 1
                    while (self._capture_next_idx < len(self._capture_thresholds)
                        and self._moves_applied >= self._capture_thresholds[self._capture_next_idx]):
                        self._save_current_frame()
                        self._capture_next_idx += 1

                # IMPORTANT: rotate surface per atomic move if 'spec' is True
                if spec:
                    self._rotate_surface(face, 1)

        self._move_idx = end

        # one repaint per tick
        self.update_image()
        self.output.setPlainText(
            f"{self._move_idx}/{len(self._moves)} moves "
            f"({100*self._move_idx/len(self._moves):.1f}%)"
        )

    # ---------- per-face surface rotate (outer face when spec=True) ----------
    def _rotate_surface(self, face, count):
        surface_cycles = {
            'U': [1, 2, 3, 4],
            'L': [5, 6, 7, 8],
            'F': [9, 10, 11, 12],
            'R': [13, 14, 15, 16],
            'B': [17, 18, 19, 20],
            'D': [21, 22, 23, 24],
        }
        cycle = surface_cycles[face]
        for _ in range(count):
            old = self.solver.subcell_color.copy()
            for k in cycle:
                prev = cycle[(cycle.index(k) - 1) % len(cycle)]
                for a in range(1, self.M + 1):
                    for b in range(1, self.M + 1):
                        self.solver.subcell_color[(k, a, b)] = old[(prev, a, b)]

    # ---------- p-layer moves  ----------
    def apply_move(self, p, cycle1, cycle2, fixed1='b', fixed2='a'):
        M = self.M
        b_t = M + 1 - p
        a_t = b_t
        old = self.solver.subcell_color.copy()
        for i, k in enumerate(cycle1):
            prev = cycle1[i - 1]
            if fixed1 == 'b':
                for a in range(1, M + 1):
                    self.solver.subcell_color[(k, a, b_t)] = old[(prev, a, b_t)]
            else:
                for b in range(1, M + 1):
                    self.solver.subcell_color[(k, a_t, b)] = old[(prev, a_t, b)]
        for i, k in enumerate(cycle2):
            prev = cycle2[i - 1]
            if fixed2 == 'b':
                for a in range(1, M + 1):
                    self.solver.subcell_color[(k, a, b_t)] = old[(prev, a, b_t)]
            else:
                for b in range(1, M + 1):
                    self.solver.subcell_color[(k, a_t, b)] = old[(prev, a_t, b)]

    def apply_pR(self, p): self.apply_move(p, [2,20,22,10], [11,3,17,23], 'b','a')
    def apply_pL(self, p): self.apply_move(p, [1,9,21,19], [4,12,24,18], 'a','b')
    def apply_pF(self, p): self.apply_move(p, [4,13,22,7], [3,16,21,6], 'a','b')
    def apply_pB(self, p): self.apply_move(p, [2,5,24,15], [1,8,23,14], 'a','b')
    def apply_pU(self, p): self.apply_move(p, [9,5,17,13], [10,6,18,14], 'b','a')
    def apply_pD(self, p): self.apply_move(p, [12,16,20,8], [11,15,19,7], 'a','b')


def main():
    vispy_app.use_app('pyqt5')
    qapp = QApplication(sys.argv)
    win = SolverUI(M=15, seed=103, cell_size=1)
    win.show()
    sys.exit(qapp.exec_())

if __name__ == "__main__":
    main()
    
