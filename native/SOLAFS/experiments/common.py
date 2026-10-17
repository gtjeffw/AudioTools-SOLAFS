"""Shared paths and helpers for the SOLAFS experiments."""
import math
import subprocess
import wave
from pathlib import Path

import numpy as np

EXP = Path(__file__).resolve().parent
REPO = EXP.parents[2]
AUDIO = REPO / "native" / "bin" / "audio"
DRIVER = EXP / "build" / "solafs_exp"
OUT = EXP / "out"
PAGE = EXP / "page"

SPEECH = ["abc", "allegra_sidner", "anne_drum", "bruce_walker", "jeff_wilson", "natures_miracle_med"]


def input_path(name):
    """Recordings come from native/bin/audio; the synthetic glide tone lives in inputs/."""
    p = EXP / "inputs" / f"{name}.wav"
    return p if p.exists() else AUDIO / f"{name}.wav"


def read_wav(p):
    with wave.open(str(p)) as w:
        sr = w.getframerate()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(np.float64) / 32768.0
    return x, sr


def write_wav(p, x, sr):
    with wave.open(str(p), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes(np.clip(np.round(x * 32768), -32768, 32767).astype("<i2").tobytes())


def trim(x, sr, db=-40.0):
    """Drop leading and trailing silence (10 ms frames below db re the loudest), keeping a 10 ms margin."""
    n = int(0.01 * sr)
    e = np.array([np.mean(x[i:i + n] ** 2) for i in range(0, len(x) - n + 1, n)]) + 1e-12
    a = np.flatnonzero(10 * np.log10(e / e.max()) > db)
    return x[max(0, (a[0] - 1) * n): min(len(x), (a[-1] + 2) * n)]


def samples(ms, sr):
    """Milliseconds to samples, rounding halves up (as the web app does)."""
    return int(math.floor(ms * sr / 1000 + 0.5))


# spearcon.exe's defaults (300, 150 and 600 samples at 44.1 kHz), in ms so other rates scale
SOLAFS_W_MS, SOLAFS_WOV_MS, SOLAFS_KMAX_MS = 300 / 44.1, 150 / 44.1, 600 / 44.1


def run_driver(inp, out, w, wov, kmax, ratio=None, schedule=None, anchors=None, fade="linear", steps=None):
    """Run build/solafs_exp. schedule = (hop, ratios); anchors = list of input positions.
    Schedule and anchor files are written next to the output. Returns the output length in samples."""
    if not DRIVER.exists():
        raise SystemExit(f"{DRIVER} not found; run build.sh first")
    out = Path(out)
    args = [str(DRIVER), str(inp), str(out), str(w), str(wov), str(kmax), "-f", fade]
    if ratio is not None:
        args += ["-r", repr(float(ratio))]
    if schedule is not None:
        hop, r = schedule
        f = out.with_suffix(".sched.txt")
        f.write_text(f"{hop}\n" + "\n".join(f"{v:.6f}" for v in r) + "\n")
        args += ["-s", str(f)]
    if anchors is not None:
        f = out.with_suffix(".anchors.txt")
        f.write_text("".join(f"{a}\n" for a in anchors))
        args += ["-a", str(f)]
    if steps is not None:
        args += ["-l", str(steps)]
    subprocess.run(args, check=True)
    with wave.open(str(out)) as w:
        return w.getnframes()


def page_assets():
    """The shared CSS and chart JavaScript, for inlining into a page."""
    return (PAGE / "style.css").read_text(), (PAGE / "charts.js").read_text()
