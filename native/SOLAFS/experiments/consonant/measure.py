"""Consonant survival for the renders from render.py. Writes out/consonant/results.csv and prints a
summary pooled over the inputs.

Labels come from an STFT (8 ms frames, 2 ms hop), independent of the time-domain detector that drives
the schedule:
  fricatives - frames with more than 40% of their 0-8 kHz energy above 2.5 kHz, level > -45 dB
  bursts     - sudden rises in the log spectrum above 1 kHz (mean positive flux > 4 dB per frame) with
               the level up > 6 dB on the previous 20 ms; marked for 15 ms
Gaps under 10 ms are merged and events under 6 ms dropped. Voiced, nasal and liquid consonants are
not labelled. "Vowel" is the remaining audio above -25 dB.
Retention: the driver logs the input position each step copied, and input samples inside any copied
window [pos, pos+W) count as reaching the output. A consonant with under 2 ms retained is "lost".
"""
import argparse
import csv
import sys
from pathlib import Path

import numpy as np
from scipy import signal

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, SPEECH, SOLAFS_W_MS, read_wav, samples
from render import RATIOS, RENDERS, TRIMMED, VARIANTS


def labels(x, sr):
    n = int(0.008 * sr); hop = int(0.002 * sr)
    f, t, Z = signal.stft(x, sr, nperseg=n, noverlap=n - hop, boundary=None, padded=False)
    P = np.abs(Z) ** 2; band = f <= 8000
    tot = P[band].sum(0) + 1e-12; hf = P[(f > 2500) & band].sum(0) / tot
    lev = 10 * np.log10(tot / tot.max())
    fric = (hf > 0.4) & (lev > -45)
    L = 10 * np.log10(P[(f > 1000) & band] + 1e-10)
    flux = np.r_[0, np.mean(np.maximum(0, L[:, 1:] - L[:, :-1]), axis=0)]
    peaks, _ = signal.find_peaks(flux, height=4.0, distance=int(0.03 * sr / hop))
    burst = np.zeros_like(fric)
    for i in peaks:
        if lev[i] > -40 and lev[i] - lev[max(0, i - 10): i].min() > 6:
            burst[i: i + int(0.015 * sr / hop)] = True
    cons = fric | burst; vow = (~cons) & (lev > -25)
    centers = np.arange(len(t)) * hop + n // 2

    def to_mask(fr):
        m = np.zeros(len(x), bool)
        for i in np.flatnonzero(fr):
            m[max(0, centers[i] - hop // 2): centers[i] + hop // 2] = True
        return m
    cm = to_mask(cons)
    idx = np.flatnonzero(np.diff(np.r_[0, cm.astype(int), 0]))
    merged = []
    for a, b in zip(idx[::2], idx[1::2]):
        if merged and a - merged[-1][1] < 0.010 * sr:
            merged[-1][1] = b
        else:
            merged.append([a, b])
    return [(a, b) for a, b in merged if b - a >= 0.006 * sr], to_mask(vow) & ~cm


def retained(steps, W, n):
    m = np.zeros(n, bool)
    for pos in steps:
        m[pos: pos + W] = True
    return m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inputs", default=",".join(SPEECH))
    ap.add_argument("--ratios", default=",".join(map(str, RATIOS)))
    ap.add_argument("--variants", default=",".join(VARIANTS))
    a = ap.parse_args()
    inputs, ratios, variants = a.inputs.split(","), [float(q) for q in a.ratios.split(",")], a.variants.split(",")
    rows = []
    for name in inputs:
        x, sr = read_wav(TRIMMED / f"{name}.wav")
        W = samples(SOLAFS_W_MS, sr)
        events, vmask = labels(x, sr)
        for q in ratios:
            for v in variants:
                d = RENDERS / name / f"{q:g}"
                steps = [int(l.split()[0]) for l in (d / f"{v}.steps.txt").read_text().split("\n") if l]
                m = retained(steps, W, len(x))
                kept = np.array([m[a:b].sum() / sr * 1000 for a, b in events])
                dur = np.array([(b - a) / sr * 1000 for a, b in events])
                y, _ = read_wav(d / f"{v}.wav")
                rows.append({"input": name, "ratio": q, "variant": v, "events": len(events),
                             "cons_in_ms": dur.sum(), "cons_kept_ms": kept.sum(),
                             "kept_ms_per_event": kept.mean() if len(kept) else 0.0, "lost": int(np.sum(kept < 2.0)),
                             "vowel_in_ms": vmask.sum() / sr * 1000, "vowel_kept_ms": (m & vmask).sum() / sr * 1000,
                             "out_ms": len(y) / sr * 1000})
    out = OUT / "consonant" / "results.csv"
    with open(out, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
    print(f"wrote {out}\n")
    print(f"{'ratio':>5} {'variant':10} {'events':>6} {'cons kept':>10} {'per event':>9} {'lost':>6} {'vowel kept':>10}")
    for q in ratios:
        for v in variants:
            rs = [r for r in rows if r["ratio"] == q and r["variant"] == v]
            ev = sum(r["events"] for r in rs)
            ck = sum(r["cons_kept_ms"] for r in rs); ci = sum(r["cons_in_ms"] for r in rs)
            vk = sum(r["vowel_kept_ms"] for r in rs); vi = sum(r["vowel_in_ms"] for r in rs)
            lost = 100 * sum(r["lost"] for r in rs) / ev
            print(f"{q:>4g}x {v:10} {ev:6d} {100 * ck / ci:9.1f}% {ck / ev:7.1f}ms {lost:5.0f}% {100 * vk / vi:9.1f}%")
        print()


if __name__ == "__main__":
    main()
