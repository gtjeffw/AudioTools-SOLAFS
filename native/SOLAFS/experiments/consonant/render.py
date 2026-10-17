"""Uniform vs consonant-aware SOLAFS at equal output length.

Renders out/consonant/renders/<input>/<ratio>/<variant>.wav, with the ratio schedule (.sched.txt),
anchors (.anchors.txt) and step log (.steps.txt) next to each. Every variant gets the same input,
trimmed of leading and trailing silence (-40 dB, 10 ms margin) as the web app does; the trimmed
inputs are written to out/consonant/inputs/.

  U-uniform  one ratio for the whole file (current SOLAFS)
  V3         ratio per 5 ms from detect.py; consonants get 1/3 of the vowel ratio
  V3L        V3 with the high-frequency burst detector and look-ahead
  V3A        V3L plus anchors at detected onsets

usage: render.py [--inputs a,b,...] [--ratios 3,5,...] [--variants U-uniform,V3L,...]
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import (OUT, SPEECH, SOLAFS_KMAX_MS, SOLAFS_W_MS, SOLAFS_WOV_MS, input_path, read_wav, run_driver,
                    samples, trim, write_wav)
from detect import anchors, schedule

RATIOS = [3, 5, 8, 12, 16, 20]
VARIANTS = {"U-uniform": None, "V3": (3.0, False, False), "V3L": (3.0, True, False), "V3A": (3.0, True, True)}  # (P, improved, anchors)
RMAX = 80.0
RENDERS = OUT / "consonant" / "renders"
TRIMMED = OUT / "consonant" / "inputs"


def render_input(name, ratios, variants):
    TRIMMED.mkdir(parents=True, exist_ok=True)
    x, sr = read_wav(input_path(name))
    x = trim(x, sr)
    tin = TRIMMED / f"{name}.wav"
    write_wav(tin, x, sr)
    anc = anchors(x, sr)
    w, wov, k = samples(SOLAFS_W_MS, sr), samples(SOLAFS_WOV_MS, sr), samples(SOLAFS_KMAX_MS, sr)

    def run(out, ratio=None, sched=None, anchored=False):
        return run_driver(tin, out, w, wov, k, ratio=ratio, schedule=sched, anchors=anc if anchored else None,
                          steps=out.with_suffix(".steps.txt"))

    for q in ratios:
        d = RENDERS / name / f"{q:g}"
        d.mkdir(parents=True, exist_ok=True)
        Lu = run(d / "U-uniform.wav", ratio=q)
        line = f"{name:20s} {q:4g}x  U={Lu / sr:.3f}s"
        for v in variants:
            P = VARIANTS[v]
            if P is None:
                continue
            # the output length moves in steps as the schedule changes: correct the target, keep the closest
            target, best = Lu, None
            for it in range(10):
                h, r, *_ = schedule(x, sr, target, P=P[0], RMAX=RMAX, improved=P[1])
                Lv = run(d / f"{v}.wav", sched=(h, r), anchored=P[2])
                if best is None or abs(Lv - Lu) < abs(best[0] - Lu):
                    best = (Lv, target)
                if abs(Lv - Lu) <= 0.005 * Lu:
                    break
                target *= (Lu / Lv) ** (1.0 if it < 3 else 0.5)
            Lv, target = best
            h, r, *_ = schedule(x, sr, target, P=P[0], RMAX=RMAX, improved=P[1])
            run(d / f"{v}.wav", sched=(h, r), anchored=P[2])
            line += f"  {v}={Lv / sr:.3f}s {100 * (Lv / Lu - 1):+.1f}% (r {r.min():.1f}-{r.max():.1f})"
        print(line, flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inputs", default=",".join(SPEECH))
    ap.add_argument("--ratios", default=",".join(map(str, RATIOS)))
    ap.add_argument("--variants", default=",".join(VARIANTS))
    a = ap.parse_args()
    for name in a.inputs.split(","):
        render_input(name, [float(q) for q in a.ratios.split(",")], a.variants.split(","))


if __name__ == "__main__":
    main()
