"""Objective measures for the SOLAFS vs WSOLA renders. Writes out/wsola/results.csv.

For each render in out/wsola/renders/<input>/<ratio>/<variant>.wav:
  dur_err     - output length vs input length / ratio, in percent
  lsd_db      - log-spectral distance (dB, 0-7.5 kHz) between the output's spectrogram and the input's
                spectrogram stretched in time to the output's length. Lower = closer to an ideal
                time-scaled version of the input.
  clicks_ps   - discontinuities per second: samples whose second difference is > 8x the local (20 ms)
                RMS of the second difference, merged within 1 ms. Lower = fewer splice clicks.
  periodicity - glide_tone only: mean of the max normalised autocorrelation over 90-300 Hz lags, 40 ms
                frames. A perfectly periodic tone scores 1; misaligned splices pull it down.
  level_db    - output RMS relative to the input's
"""
import csv
import sys
from pathlib import Path

import numpy as np
from scipy import ndimage, signal

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, input_path, read_wav

RENDERS = OUT / "wsola" / "renders"


def logspec(x, sr):
    n = int(0.032 * sr)
    f, t, Z = signal.stft(x, sr, nperseg=n, noverlap=3 * n // 4, boundary=None, padded=False)
    return f, t, np.abs(Z) ** 2


def lsd(x_in, x_out, sr, ratio):
    f, t_in, P_in = logspec(x_in, sr)
    _, t_out, P_out = logspec(x_out, sr)
    if P_out.shape[1] < 2 or P_in.shape[1] < 2:
        return np.nan
    # the output frame at time t corresponds to input time t * ratio
    src = np.clip(t_out * ratio, t_in[0], t_in[-1])
    P_ref = np.stack([np.interp(src, t_in, row) for row in P_in])
    band = f <= 7500
    eps = 1e-10
    D_ref = 10 * np.log10(P_ref[band] + eps)
    D_out = 10 * np.log10(P_out[band] + eps)
    e_ref = D_ref.max(axis=0)
    active = e_ref > e_ref.max() - 40  # skip silence
    d = np.sqrt(np.mean((D_out[:, active] - D_ref[:, active]) ** 2, axis=0))
    return float(np.mean(d))


def clicks_per_sec(x, sr):
    d2 = np.zeros_like(x)
    d2[1:-1] = x[2:] - 2 * x[1:-1] + x[:-2]
    win = max(3, int(0.020 * sr))
    rms = np.sqrt(ndimage.uniform_filter1d(d2 ** 2, win) + 1e-12)
    hits = np.flatnonzero((np.abs(d2) > 8 * rms) & (np.abs(d2) > 1e-3))
    if hits.size == 0:
        return 0.0
    events = 1 + np.count_nonzero(np.diff(hits) > int(0.001 * sr))
    return events / (len(x) / sr)


def periodicity(x, sr):
    n, hop = int(0.040 * sr), int(0.010 * sr)
    lo, hi = int(sr / 300), int(sr / 90)
    vals = []
    for s in range(0, len(x) - n - hi, hop):
        a = x[s:s + n]
        ea = np.dot(a, a)
        if ea < 1e-6:
            continue
        best = 0.0
        for l in range(lo, hi + 1):
            b = x[s + l:s + l + n]
            best = max(best, np.dot(a, b) / np.sqrt(ea * np.dot(b, b) + 1e-12))
        vals.append(best)
    return float(np.mean(vals)) if vals else np.nan


def main():
    rows = []
    for in_dir in sorted(RENDERS.iterdir()):
        x_in, sr = read_wav(input_path(in_dir.name))
        for r_dir in sorted(in_dir.iterdir(), key=lambda p: float(p.name)):
            ratio = float(r_dir.name)
            for f in sorted(r_dir.glob("*.wav")):
                y, _ = read_wav(f)
                rows.append({"input": in_dir.name, "ratio": ratio, "variant": f.stem,
                             "dur_err": 100 * (len(y) / (len(x_in) / ratio) - 1),
                             "lsd_db": lsd(x_in, y, sr, ratio),
                             "clicks_ps": clicks_per_sec(y, sr),
                             "periodicity": periodicity(y, sr) if in_dir.name == "glide_tone" else np.nan,
                             "level_db": 20 * np.log10(np.sqrt(np.mean(y ** 2)) / np.sqrt(np.mean(x_in ** 2)))})
    out = OUT / "wsola" / "results.csv"
    with open(out, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]))
        w.writeheader(); w.writerows(rows)
    print(f"wrote {out} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
