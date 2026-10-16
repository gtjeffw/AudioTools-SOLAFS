# AudioTools: SOLAFS and spearcons

Tools for making **spearcons**: speech sped up until it is no longer quite recognized as speech, used as short
auditory menu cues. The core is **SOLAFS** (Synchronized Overlap-Add, Fixed Synthesis; Hejna & Musicus, 1991), a
time-scale modification algorithm that changes the speed of speech without changing its pitch.

There are two ways to use it:

- **Spearcon Factory** (`web/`): a Progressive Web App (PWA) for generating spearcons in batches. Audio files are
  added by drag and drop, processed with configurable settings, auditioned, and exported together as a ZIP archive.
  It runs the same C++ SOLAFS code, compiled to WebAssembly, entirely on the local device, and can be installed
  for offline use.
- **Command-line tools** (`native/`): the original C++ (Windows, Visual Studio). `spearcon.exe` makes one spearcon;
  `SilentStripper.exe` trims silence from a folder of WAV files.

## Repository layout

| Path | What |
|---|---|
| `web/` | Spearcon Factory web app (Vite + React + TypeScript) and the WebAssembly build of the engine |
| `native/SOLAFS/` | SOLAFS itself (`src/SOLAFS/SOLAFS.cpp`); `project/SOLAFS/SOLAFS.sln` is the Visual Studio solution for everything |
| `native/SOLAFS/src/SOLAFS/docs/` | references: notes on the changes made to Ellis's MATLAB `solafs.m` while porting it, and US patent 5,175,769, which describes the same algorithm |
| `native/WAVIO/`, `native/CIRCULAR_BUFFER/` | WAV file I/O and the circular buffer SOLAFS streams through |
| `native/Spearcon/` | `spearcon.exe` |
| `native/SilentStripper/` | `SilentStripper.exe` |
| `native/bin/` | test scripts (`spearconTest*.bat`, `SilentStripper_*.bat`) and test audio in `bin/audio/` |

## Spearcon Factory (web app)

**Usage:** audio files are added by dropping them onto the page or selecting them (WAV, MP3, M4A, OGG, FLAC and
other formats supported by the browser).

- **Settings** apply to all files; any change re-processes the batch in parallel Web Workers. Settings persist in
  the browser.
  - **Duration:** *Logarithmic* (the spearcon.exe default) compresses longer phrases proportionally more:
    output duration (s) = A · log₁₀(input duration + 1) + B. *Fixed duration* gives every spearcon the same
    duration; *Fixed ratio* applies a constant compression ratio.
  - **Progressive compression:** the ratio increases over the course of the clip, leaving the total duration
    unchanged.
  - **Pre- and post-processing:** leading and trailing silence can be trimmed before compression (recommended,
    since silence otherwise counts towards the input duration); fades and silence can be added afterwards.
  - **SOLAFS window (advanced):** window length, overlap and maximum skew, in milliseconds.
- **Playback** of the original or the spearcon for any file, or of all spearcons in sequence with a configurable
  interval, as they would be heard in a menu.
- **Export ZIP:** 16-bit WAV files named `<file>_spearcon.wav` (the suffix is configurable), with
  `spearcon-settings.json` recording the settings and each file's durations and compression ratio.
- **Load demo** adds four example recordings from `native/bin/audio` and sets a fixed duration of 0.2 s.
  **Reset to defaults** restores the default settings.
- **Install** (Chromium-based browsers) installs the PWA directly; elsewhere, **Installation** describes the
  procedure for the current platform.
- The defaults are those of spearcon.exe, with silence trimming and short fades added. With trimming and fades
  disabled, a 16-bit WAV at 44.1 kHz yields output byte-identical to the command-line tool (see *Testing*).

**Developing** (Node 24, pinned in `web/.nvmrc`):

```bash
cd web
nvm use
npm install
npm run dev        # http://localhost:5173
npm test           # unit tests + engine parity with the native C++
npm run build      # production build in web/dist (PWA included)
npm run preview    # serve web/dist locally
```

**The engine:** `web/wasm/solafs_wasm.cpp` wraps the native `SOLAFS()` and WavIO code. It is compiled with Emscripten
inside the official `emscripten/emsdk` Docker image, so nothing extra is installed on your machine. The compiled
engine (`web/src/engine/generated/solafs.mjs` and `solafs.wasm`) is committed, so the web app builds and deploys with
only Node. Rebuild it after changing any of the C++ it uses, with Docker running:

```bash
npm run build:wasm
```

`npm run build` and `npm test` fail with a message if the committed engine is older than the C++ sources
(`npm run check:wasm` compares a hash of the inputs).

**Testing:** `web/test/engine.test.ts` runs the WebAssembly engine, with the app's TypeScript port of spearcon.exe's
length arithmetic, on the files in `native/bin/audio` with 7 different option sets. All 42 outputs must be
byte-identical to the native C++ build. The reference hashes in `web/test/fixtures/native-spearcon.json` come from
the native build compiled with clang `-ffp-contract=off`. WebAssembly has no fused multiply-add, and a native build
that fuses multiply-adds (clang's default on ARM) rounds slightly differently, which occasionally changes which
alignment SOLAFS picks.

**Deploying (Cloudflare Pages):** connect the GitHub repository in Cloudflare Pages with these build settings:

| Setting | Value |
|---|---|
| Root directory | `web` |
| Build command | `npm run build` |
| Build output directory | `dist` |
| Build watch paths | `web/**`, `native/**` (the build bundles the demo clips from `native/bin/audio` and checks the engine against the C++ sources) |

Node comes from `web/.nvmrc`. If the build ever uses a different version, set the environment variable
`NODE_VERSION=24`. The build needs only Node, because the engine is committed. `web/public/_headers` sets caching:
hashed assets are cached forever, while `index.html`, the service worker and the manifest are always revalidated, so
new versions reach users. After a deploy, installed copies show a "new version available" prompt.

## spearcon.exe

Makes a spearcon from one WAV file:

```
spearcon INFILE:in.wav OUTFILE:out.wav [PARAMETER:value ...]
```

Input should be 16-bit PCM WAV, ideally mono; only the first channel is used. Output is mono 16-bit at the input's
sample rate. Run it with no arguments to print its built-in help.

| Parameter | Value | Default | Meaning |
|---|---|---|---|
| `INFILE`, `OUTFILE` | path | required | Input and output WAV files. The value is everything after the first `:`, so `INFILE:C:\audio\x.wav` works. |
| `TIMESCALE` | `LOG`, `LEN_SEC`, `IN_TO_OUT_RATIO` | `LOG` | How the output length is chosen. Usually implied by the parameters below. |
| `LOG_TIME_A`, `LOG_TIME_B` | number | 1.0, 0.25 | LOG mode: output seconds = A · log₁₀(input seconds + 1) + B. (The built-in help omits the + 1.) |
| `LENGTH_SEC` | seconds | 0.25 | LEN_SEC mode: exact output length. |
| `IN_TO_OUT_RATIO` | number | 1.0 | Ratio mode: input length ÷ output length (2 = twice as fast). |
| `COMPSTEPMODE` | `NONE`, `LINEAR` | `NONE` | `LINEAR` starts slower and speeds up through the clip, keeping the output length. |
| `CS_START_RAT` | number | 1.0 | The starting speed for `LINEAR`; implies `COMPSTEPMODE:LINEAR`. |
| `SOLAFS_WINDOWSIZE` | samples | 300 | SOLAFS window. |
| `SOLAFS_OVERLAP` | samples | 150 | Overlap between windows; at most half the window. |
| `SOLAFS_MAXSKEW` | samples | 600 | Maximum shift when aligning windows. Should exceed the longest pitch period. |

Mixing parameters from different modes (for example `LENGTH_SEC` with `LOG_TIME_A`) is an error. Examples, from
`native/bin/spearconTest.bat`:

```
spearcon INFILE:audio\bruce_walker.wav OUTFILE:audio\spearcon_bruce.wav LENGTH_SEC:0.2
spearcon INFILE:audio\natures_miracle_med.wav OUTFILE:audio\spearcon_step.wav LENGTH_SEC:2.0 COMPSTEPMODE:LINEAR CS_START_RAT:0.01
```

Things to know:

- The window settings are in **samples**, so they mean different durations at different sample rates.
- Changing `SOLAFS_WINDOWSIZE` doesn't change the overlap or max-skew defaults (150 and 600); set all three together.
- The exit code is always 0, and "Spearcon generated." is printed even when processing failed. Check the messages
  above it, or the output file.
- Files shorter than about window + max skew (≈ 23 ms at the defaults, 44.1 kHz) are rejected.

## SilentStripper.exe

Trims leading and trailing silence from every WAV file in a folder, fades the ends, and pads with silence:

```
SilentStripper <inFolder>/* <outFolder>/ <silenceThresh> <envRampSize_ms> <beginPad_ms> <endPad_ms>
SilentStripper ./audio/* ./audio/output/ 300 10 20 20
```

- The input must be a folder followed by `*`; all `.wav`/`.WAV` files in it are processed. A single file name doesn't
  work.
- The output folder must already exist, and the path should end with `/` (otherwise its last character is replaced
  with `/`). Output files keep their input names. Don't point it at the input folder, or each file is overwritten
  while it's being read.
- `silenceThresh` is compared with **raw 16-bit sample values** (0–32767), not 0–1. The `0.01` and `0.08` in the
  test scripts only treat exact zeros as silence.
- `beginPad_ms` and `endPad_ms` must be more than 0.
- Mono files only. It waits for Enter before exiting.

## Building the native tools

Open `native/SOLAFS/project/SOLAFS/SOLAFS.sln` in Visual Studio. The projects are Visual Studio 2008 (`.vcproj`), so
a current Visual Studio converts them first. Build Debug or Release (Win32, Unicode); executables go to
`native/bin/Debug/` or `native/bin/Release/`, where the `.bat` test scripts expect them.

The SOLAFS, WavIO and circular-buffer sources also compile as standard C++ with clang. That's how the WebAssembly
engine is built, using `web/wasm/compat/tchar.h` in place of Windows' `<tchar.h>`.

## SOLAFS references

- D. Hejna and B. R. Musicus, "The SOLAFS Time-Scale Modification Algorithm," BBN Technical Report, July 1991. The
  only scan online (linked from Dan Ellis's page below) is missing page 4.
- US patent 5,175,769, "Method for time-scale modification of signals" (Hejna, Musicus, Crowe; 1992), in
  `native/SOLAFS/src/SOLAFS/docs/US5175769.pdf`. It covers the same algorithm, including the shift-prediction
  derivation that's on the missing page (columns 9–10), and gives a flowchart (FIGs. 5–7).
- D. Ellis, `solafs.m`, the MATLAB implementation this C++ was ported from: <https://www.ee.columbia.edu/~dpwe/LabROSA/matlab/solafs-matlab.html>.
  `native/SOLAFS/src/SOLAFS/docs/solafs-matlab-notes.md` records the changes made to it while porting.

## License

MIT (see `LICENSE`). Copyright © 2009–2026 Georgia Institute of Technology, Jeff Wilson. The original tools were
written at Georgia Tech's Interactive Media Technology Center.

Third-party material keeps its own terms:

- `native/SilentStripper/source/SilentStripper/dirent.c` and `dirent.h`: © Kevlin Henney, under the permissive
  notice in those files.
- `native/SOLAFS/src/SOLAFS/docs/US5175769.pdf`: a US patent, in the public domain. The patent expired in 2011.

## History

Recovered in October 2026 from an SVN working copy of `https://gadget.ipat.gatech.edu/imtc/AudioTools`; that server
no longer exists, so the revision history is lost. The first commit is the checked-out state: r5, dated 2011-10-07
(files last changed in r2–r5 on 2011-10-06/07). SVN's `trunk/` became `native/`; `tags/` and `branches/` were empty.
