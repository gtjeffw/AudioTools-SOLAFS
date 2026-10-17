"""Consonant detector and ratio schedule. Time-domain only (no FFT), so it could run in the C++ stream.

Per 5 ms frame:
  level   - RMS energy in dB, relative to the loudest frame
  f_rms   - RMS frequency from the first difference: E[(x[n]-x[n-1])^2] / E[x^2] = 4 sin^2(pi f/sr)
            for a sine at f, so f_rms = sr/pi * asin(sqrt(ratio)/2). Vowels sit well below 1.5 kHz;
            fricatives (/s/, /f/, /sh/) and stop bursts sit above it.
  onset   - rise of the level over the previous 30 ms. A burst after a stop closure, or any syllable
            onset, jumps by 10+ dB; the 5 ms before and 25 ms after are protected (bursts and the
            formant transitions into the vowel carry place of articulation).
Consonant score c in [0,1]: high-f_rms frames, ramped from 1.5 to 2.5 kHz, plus protected onsets.
Relative ratio weight w = P**(-c) (P = protection: consonants are compressed P times less than
vowels); silence (< -40 dB) gets w = SIL. The schedule is r = k*w, clamped to [RMIN, RMAX], with
k found so the predicted output length hits the target.
improved=True adds a high-frequency-level burst detector and the look-ahead (see lookahead()).
"""
import numpy as np

FRAME_MS = 5.0


def features(x, sr):
    h = int(round(FRAME_MS / 1000 * sr))
    n = len(x) // h
    fr = x[: n * h].reshape(n, h)
    d = np.diff(x, prepend=x[0])[: n * h].reshape(n, h)
    e = np.mean(fr ** 2, axis=1) + 1e-12
    ed = np.mean(d ** 2, axis=1)
    # 10 ms energy (two frames) for a steadier level
    e2 = np.convolve(e, [0.5, 0.5], mode="same")
    level = 10 * np.log10(e2 / e2.max())
    f_rms = sr / np.pi * np.arcsin(np.clip(np.sqrt(ed / e) / 2, 0, 1))
    ed2 = np.convolve(ed + 1e-12, [0.5, 0.5], mode="same")
    hf_level = 10 * np.log10(ed2 / ed2.max())
    return h, level, f_rms, hf_level


def consonant_score(level, f_rms, hf_level=None):
    n = len(level)
    active = level > -40
    hf = np.clip((f_rms - 1500) / 1000, 0, 1) * (level > -35)
    c = hf.copy()
    back = int(30 / FRAME_MS)
    for t in range(n):
        lo = max(0, t - back)
        prev = level[lo:t].min() if t > lo else level[t]
        if level[t] > -35 and level[t] - prev > 10:
            c[max(0, t - 1): t + int(25 / FRAME_MS) + 1] = 1.0
        if hf_level is not None and t > 0:
            # bursts: a 6 dB rise of the high-frequency (first-difference) level over 20 ms
            lo4 = max(0, t - int(20 / FRAME_MS))
            if level[t] > -40 and hf_level[t] - hf_level[lo4:t].min() > 6:
                c[max(0, t - 1): t + int(20 / FRAME_MS) + 1] = 1.0
    c = np.convolve(np.maximum(c, np.r_[c[1:], 0]), np.ones(3) / 3, mode="same")
    return np.clip(c, 0, 1), active


def lookahead(r, h, Ss, reach):
    """Each ratio sets the jump to the next step (r*Ss samples, then up to Kmax of skew, then a window
    of W), so a protected region is only safe if the jump INTO it is short too: use the smallest ratio
    anywhere the jump could reach."""
    out = r.copy()
    for i in range(len(r)):
        j = i + int(np.ceil((r[i] * Ss + reach) / h)) + 1
        out[i] = r[i:j].min()
    return out


def schedule(x, sr, target_out, P=3.0, SIL=2.0, RMIN=1.0, RMAX=30.0, improved=False):
    h, level, f_rms, hf_level = features(x, sr)
    c, active = consonant_score(level, f_rms, hf_level if improved else None)
    w = np.where(active, P ** (-c), SIL)
    W = 300 / 44100 * sr; Ss = W / 2; K = 600 / 44100 * sr

    def ratios(k):
        r = np.clip(k * w, RMIN, RMAX)
        return lookahead(r, h, Ss, W + K) if improved else r

    def out_len(k):
        return np.sum(h / ratios(k))

    lo, hi = 1e-3, 1e3
    for _ in range(100):
        mid = np.sqrt(lo * hi)
        lo, hi = (mid, hi) if out_len(mid) > target_out else (lo, mid)
    r = ratios(np.sqrt(lo * hi))
    return h, r, c, level, f_rms


def anchors(x, sr):
    """Onsets of protected regions (rising edges of the improved consonant score), moved back by half
    the skew range so the window overlaps the onset wherever the similarity search moves it."""
    h, level, f_rms, hf_level = features(x, sr)
    c, _ = consonant_score(level, f_rms, hf_level)
    on = np.flatnonzero((c[1:] >= 0.5) & (c[:-1] < 0.5)) + 1
    K = 600 / 44100 * sr
    return [int(max(0, i * h - K / 2)) for i in on]
