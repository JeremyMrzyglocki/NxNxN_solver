import sys
import random
import numpy as np
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QMessageBox
from vispy import scene, app

# Color palette: white, yellow, orange, red
PALETTE = np.array([
    [1.0, 1.0, 1.0],    # white
    [1.0, 1.0, 0.0],    # yellow
    [1.0, 0.647, 0.0],  # orange
    [1.0, 0.0, 0.0]     # red (cross-out and rectangles)
], dtype=np.float32)

class GridUI(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Grid Generator")

        # Central widget and layouts
        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        vbox = QtWidgets.QVBoxLayout(central)

        # Controls
        ctrl = QtWidgets.QHBoxLayout()
        vbox.addLayout(ctrl)
        ctrl.addWidget(QtWidgets.QLabel("M="))
        self.entryM = QtWidgets.QLineEdit("500")
        self.entryM.setFixedWidth(50)
        ctrl.addWidget(self.entryM)

        ctrl.addWidget(QtWidgets.QLabel("p="))
        self.entryP = QtWidgets.QLineEdit("0.0005")
        self.entryP.setFixedWidth(50)
        ctrl.addWidget(self.entryP)

        self.btnGen = QtWidgets.QPushButton("Generate")
        ctrl.addWidget(self.btnGen)
        self.btnCross = QtWidgets.QPushButton("Cross Out")
        self.btnCross.setEnabled(False)
        ctrl.addWidget(self.btnCross)

        # Vispy canvas with PanZoomCamera locked to square aspect
        self.canvas = scene.SceneCanvas(keys='interactive', show=False)
        self.view = self.canvas.central_widget.add_view()
        self.view.camera = scene.cameras.PanZoomCamera(aspect=1)
        self.image = scene.visuals.Image(np.zeros((10,10,3), dtype=np.float32),
                                         parent=self.view.scene, interpolate=False)
        vbox.addWidget(self.canvas.native)

        # Statistics text box
        self.stats_box = QtWidgets.QPlainTextEdit()
        self.stats_box.setReadOnly(True)
        self.stats_box.setFixedHeight(100)
        vbox.addWidget(self.stats_box)

        # State
        self.labels = None
        self.rects = []

        # Signals
        self.btnGen.clicked.connect(self.generate_grid)
        self.btnCross.clicked.connect(self.cross_out)

    def generate_grid(self):
        # Generate grid and update stats
        try:
            M = int(self.entryM.text())
            p = float(self.entryP.text())
            if M <= 0 or not (0 <= p <= 0.5):
                raise ValueError
        except ValueError:
            QMessageBox.critical(self, "Invalid input", "Enter integer M>0 and 0 ≤ p ≤ 0.5.")
            return

        rnd = np.random.rand(M, M)
        labels = np.zeros((M, M), dtype=np.int8)
        labels[rnd < p] = 1
        labels[(rnd >= p) & (rnd < 2*p)] = 2
        self.labels = labels

        # Display
        data = PALETTE[labels]
        self.image.set_data(data)
        self.view.camera.set_range(x=(0, M), y=(0, M))
        self.canvas.show()
        self.btnCross.setEnabled(True)

        # Update stats
        total = M*M
        count_yellow = np.sum(labels == 1)
        count_orange = np.sum(labels == 2)
        count_white = np.sum(labels == 0)
        stats = [f"Grid {M}x{M}",
                 f"Total cells: {total}",
                 f"Yellow: {count_yellow}",
                 f"Orange: {count_orange}",
                 f"White: {count_white}"]
        self.stats_box.setPlainText("\n".join(stats))

        # Clear any previous rectangles
        self.rects = []

    def cross_out(self):
        if self.labels is None:
            return

        rows_keep = np.any(self.labels > 0, axis=1)
        cols_keep = np.any(self.labels > 0, axis=0)
        M, N = self.labels.shape

        if not rows_keep.any() or not cols_keep.any():
            QMessageBox.information(self, "Empty", "No colored cells to keep.")
            return

        # Red overlay on original for removed
        data_orig = PALETTE[self.labels].copy()
        for i, keep in enumerate(rows_keep):
            if not keep:
                data_orig[i, :, :] = PALETTE[3]
        for j, keep in enumerate(cols_keep):
            if not keep:
                data_orig[:, j, :] = PALETTE[3]
        self.image.set_data(data_orig)

        # Reduced grid
        reduced = self.labels[np.ix_(rows_keep, cols_keep)]
        M2, N2 = reduced.shape
        data2 = PALETTE[reduced]

        # Compute stats and combinatorial rectangles
        total2 = M2 * N2
        yellow2 = np.sum(reduced == 1)
        orange2 = np.sum(reduced == 2)
        white2 = np.sum(reduced == 0)
        rects = []
        for r1 in range(M2):
            for r2 in range(r1+1, M2):
                cols = np.where((reduced[r1]>0) & (reduced[r2]>0))[0]
                for i in range(len(cols)):
                    for j in range(i+1, len(cols)):
                        rects.append((r1, r2, cols[i], cols[j]))
        self.rects = rects
        num_rects = len(rects)

        # New window with reduced grid + rectangles + stats
        win = QtWidgets.QMainWindow(self)
        win.setWindowTitle("Reduced Grid + Rectangles")
        central = QtWidgets.QWidget()
        win.setCentralWidget(central)
        vb = QtWidgets.QVBoxLayout(central)

        canvas2 = scene.SceneCanvas(keys='interactive', show=True)
        view2 = canvas2.central_widget.add_view()
        view2.camera = scene.cameras.PanZoomCamera(aspect=1)
        scene.visuals.Image(data2, parent=view2.scene, interpolate=False)
        view2.camera.set_range(x=(0, N2), y=(0, M2))

        # Overlay rectangle outlines
        for (r1, r2, c1, c2) in rects:
            # define corners of unit squares
            pts = np.array([
                [c1, r1], [c2+1, r1], [c2+1, r2+1], [c1, r2+1], [c1, r1]
            ], dtype=np.float32)
            scene.visuals.Line(pos=pts, color=(1, 0, 0, 0.6), parent=view2.scene, width=1)

        vb.addWidget(canvas2.native)

        stats2 = QtWidgets.QPlainTextEdit()
        stats2.setReadOnly(True)
        stats_lines = [f"Reduced {M2}x{N2}",
                       f"Total cells: {total2}",
                       f"Yellow: {yellow2}",
                       f"Orange: {orange2}",
                       f"White: {white2}",
                       f"Combinatorial rectangles: {num_rects}"]
        stats2.setPlainText("\n".join(stats_lines))
        stats2.setFixedHeight(120)
        vb.addWidget(stats2)

        win.show()

if __name__ == '__main__':
    appQt = QtWidgets.QApplication(sys.argv)
    gui = GridUI()
    gui.show()
    sys.exit(appQt.exec_())
