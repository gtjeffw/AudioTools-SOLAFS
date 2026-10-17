# SOLAFS experiments

Two studies of variations on the SOLAFS code in `../src/SOLAFS/`, both run in October 2026:

1. **SOLAFS vs WSOLA** (`wsola/`): whether a raised-cosine cross-fade and longer windows (classic WSOLA)
   improve on SOLAFS as spearcon.exe uses it.
2. **Consonant-aware compression** (`consonant/`): whether varying the speed ratio through a word,
   compressing consonants less than vowels, keeps more of the consonants at high compression.

Neither changes the library, spearcon.exe or the Spearcon Factory web app. Both run the unmodified SOLAFS
code through an experiment driver, `solafs_exp.cpp`, which reproduces `SOLAFS()` exactly (bit-identical
output to spearcon.exe and the web app's engine at the same settings) and adds the options the studies
need.

In short: neither change was audibly better in informal listening. The fade shape makes almost no
difference; window length matters, and long WSOLA windows measure worse at spearcon ratios. Variable
compression keeps measurably more consonant material at 5–12×, but at those ratios no schedule can keep
most short consonants.

## Running

Needs clang and Python 3.12 with the packages in `requirements.txt`. Inputs are the recordings in
`native/bin/audio`, plus a synthetic gliding tone in `inputs/`. Everything generated goes to `out/`
(git-ignored): renders, CSVs and listening pages.

```sh
./build.sh                    # builds build/solafs_exp
python wsola/render.py        # 196 renders
python wsola/metrics.py       # out/wsola/results.csv
python wsola/page.py          # out/wsola/page/index.html
python consonant/render.py    # 144 renders, with ratio schedules and step logs
python consonant/measure.py   # out/consonant/results.csv and a summary
python consonant/page.py      # out/consonant/page/index.html
python consonant/series.py    # out/consonant/series/index.html: the full Nature's Miracle clip, 1.5x to 12x
```

Serve `out/` over HTTP to use the pages (for example `python -m http.server -d out`). The results
from the runs reported here are kept in `wsola/results.csv` and `consonant/results.csv`; re-running
reproduces them exactly.

`solafs_exp IN OUT W WOV KMAX [-r ratio] [-s schedule] [-a anchors] [-f linear|hann] [-l steplog]`:
see the header of `solafs_exp.cpp`.

## 1. SOLAFS vs WSOLA

When the overlap is half the window, SOLAFS and WSOLA differ only in the cross-fade: the similarity
search, the skew range (one-sided in SOLAFS, which only shifts the range) and the overlap-add are the
same. SOLAFS's linear fade gives a triangular window; WSOLA uses a Hann window. `-f hann` overwrites
SOLAFS's fade table with sin² after `SOLAFS_BeginStream()`.
`wsola/solafs-crossfade-option.patch` adds the same option to the library itself
(`SOLAFS_SetCrossFadeShape()`). It isn't applied because the web app's engine build hashes
`SOLAFS.cpp`, and the Hann fade adds about 7 KB (`sin()`) to the engine.

Variants (maximum skew 13.6 ms for all):

| | Cross-fade | Window / overlap |
|---|---|---|
| A: SOLAFS (spearcon.exe, web app) | linear | 6.8 / 3.4 ms |
| B | Hann | 6.8 / 3.4 ms |
| C | linear | 25 / 12.5 ms |
| D: WSOLA | Hann | 25 / 12.5 ms |

Six speech recordings and the gliding tone, at seven fixed ratios. Speech means:

| Ratio | Log-spectral distance A / B / C / D (dB, lower is better) | Level change A / B / C / D (dB) |
|---|---|---|
| 0.5× | 7.47 / 7.44 / 8.71 / 8.64 | −0.01 / −0.00 / −0.14 / −0.10 |
| 0.8× | 6.59 / 6.58 / 7.31 / 7.25 | −0.01 / −0.01 / −0.06 / −0.04 |
| 1.25× | 4.44 / 4.46 / 3.97 / 3.94 | 0.01 / 0.01 / −0.09 / −0.06 |
| 2× | 6.39 / 6.44 / 6.64 / 6.64 | −0.09 / −0.07 / −0.32 / −0.24 |
| 3× | 7.47 / 7.52 / 8.36 / 8.39 | −0.09 / −0.06 / −0.59 / −0.47 |
| 5× | 8.73 / 8.80 / 10.74 / 10.83 | −0.17 / −0.12 / −0.98 / −0.78 |
| 8× | 10.28 / 10.33 / 12.33 / 12.39 | −0.36 / −0.29 / −1.04 / −0.73 |

Log-spectral distance compares each output with the input's spectrogram stretched to the output's length.

- **The fade shape is negligible.** A and B, and C and D, are within about 0.1 dB of each other throughout.
  Glide-tone periodicity is identical across all four variants, since all use the same search.
- **Window length is what matters.** Long windows are better only at a mild speed-up (1.25×). At spearcon
  ratios (3–8×) they are 1–2 dB worse, lose up to 1 dB of level, and have more click-like events at 5–8×.
- **D vs A per file:** D is better in 6 of the 42 speech cases (six recordings at seven ratios), and worse
  by 0.96 dB on average.
- **A player's speed control** could use long windows near 1× and short ones at high compression, as
  SoundTouch does. Changing the window during a stream would need a restart of the stream and a
  cross-fade between the two, because `SOLAFS_BeginStream()` fixes the window.

## 2. Consonant-aware compression

At ratio *r*, each step keeps one 6.8 ms window and then jumps *r* × 3.4 ms ahead. At 8× that skips about
20 ms of input per step, which can contain a whole stop burst (5–15 ms). The similarity search also prefers
voiced, periodic material, which biases it against bursts.

`consonant/detect.py` sets a ratio for every 5 ms of input from time-domain features only (no FFT), so it
could run inside the C++ stream:

- **RMS frequency**, from the energy of the sample-to-sample difference;
- **jumps in level**, and in high-frequency level, for bursts and syllable onsets.

Consonants get 1/3 of the vowel ratio, and pauses inside the phrase twice the vowel ratio. The schedule is
scaled so the output length matches uniform compression; it matches within 0.5% in most cases, and within
one or two 3.4 ms steps in the shortest outputs. The driver takes the schedule with `-s`.

| Variant | |
|---|---|
| U | uniform ratio (current SOLAFS) |
| V3 | detector schedule |
| V3L | V3, plus a more sensitive burst detector and look-ahead: each step uses the lowest ratio anywhere its jump can reach (the jump, plus skew, plus window), so a jump from a heavily compressed vowel can't land past a protected consonant |
| V3A | V3L, plus anchors (`-a`) that force a step to land just before each detected onset |

Measurement (`consonant/measure.py`): 133 consonant events, labelled from an STFT independently of the
detector. These are fricatives (more than 40% of energy above 2.5 kHz) and stop bursts (sudden broadband
rises). Voiced, nasal and liquid consonants are not labelled. The driver's step log (`-l`) gives exactly
which input reaches the output.

| Ratio | Consonant material kept, U / V3 / V3L / V3A | Consonants lost (< 2 ms kept), U / V3 / V3L / V3A | Vowel material kept, U / V3L |
|---|---|---|---|
| 3× | 57 / 66 / 61 / 61% | 3 / 8 / 11 / 11% | 57 / 47% |
| 5× | 39 / 48 / 44 / 45% | 14 / 25 / 17 / 15% | 40 / 33% |
| 8× | 24 / 32 / 31 / 27% | 38 / 42 / 26 / 35% | 25 / 22% |
| 12× | 15 / 22 / 20 / 15% | 53 / 50 / 46 / 58% | 17 / 15% |
| 16× | 13 / 18 / 14 / 12% | 60 / 59 / 59 / 59% | 13 / 11% |
| 20× | 9 / 12 / 11 / 8% | 68 / 68 / 62 / 71% | 10 / 10% |

- **More consonant material.** Variable compression keeps a quarter to a half more consonant material at
  5–12×, taken from the vowels.
- **Look-ahead is essential.** Without it, half the lost consonants had been detected and protected, but
  the jump from a heavily compressed vowel landed past them. With look-ahead, losses at 8× fall from 38%
  to 26%.
- **Anchors cost more than they save.** Each forced step comes out of a fixed budget. At 20×, "Bruce
  Walker" gets about 20 steps in all, so everything else is compressed harder.
- **Past about 12×, it's a selection problem.** With one or two windows per speech sound, a schedule can
  only choose which sounds survive. Choosing one window per phoneme and joining them would suit that
  better than adjusting the speed.
- **Listening.** In informal blind listening (`out/consonant/page/`, and the full Nature's Miracle clip in
  `out/consonant/series/`), no difference was audible.
- **Caveats.** The detector thresholds were set by eye on two names and still miss some short bursts. The
  retention measure is a proxy for intelligibility, not a test of it.
