"""SOLAFS vs WSOLA: renders 7 inputs x 7 speed ratios x 4 variants to out/wsola/renders/<input>/<ratio>/<variant>.wav.

  A-solafs       linear cross-fade, window 6.8 ms, overlap 3.4 ms (spearcon.exe and the web app)
  B-hann-short   Hann cross-fade, same window (isolates the fade shape)
  C-linear-long  linear cross-fade, window 25 ms, overlap 12.5 ms (isolates the window length)
  D-wsola        Hann cross-fade, window 25 ms, overlap 12.5 ms: classic WSOLA
All use a 13.6 ms maximum skew, a fixed ratio, and no trimming or fades.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, SPEECH, SOLAFS_KMAX_MS, SOLAFS_W_MS, SOLAFS_WOV_MS, input_path, read_wav, run_driver, samples

INPUTS = SPEECH + ["glide_tone"]
RATIOS = [0.5, 0.8, 1.25, 2, 3, 5, 8]
VARIANTS = {
    "A-solafs": ("linear", SOLAFS_W_MS, SOLAFS_WOV_MS),
    "B-hann-short": ("hann", SOLAFS_W_MS, SOLAFS_WOV_MS),
    "C-linear-long": ("linear", 25.0, 12.5),
    "D-wsola": ("hann", 25.0, 12.5),
}
RENDERS = OUT / "wsola" / "renders"


def main():
    n = 0
    for name in INPUTS:
        _, sr = read_wav(input_path(name))
        for q in RATIOS:
            d = RENDERS / name / f"{q:g}"
            d.mkdir(parents=True, exist_ok=True)
            for v, (fade, w_ms, wov_ms) in VARIANTS.items():
                run_driver(input_path(name), d / f"{v}.wav", samples(w_ms, sr), samples(wov_ms, sr),
                           samples(SOLAFS_KMAX_MS, sr), ratio=q, fade=fade)
                n += 1
    print(f"rendered {n} files to {RENDERS}")


if __name__ == "__main__":
    main()
