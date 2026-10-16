// How fast to play each file: the same arithmetic as genSpearcon() in
// native/Spearcon/source/Spearcon/Spearcon_Main.cpp, so the app and spearcon.exe agree,
// plus the validation spearcon.exe doesn't do (it would run SOLAFS with a negative or infinite ratio).

export type LengthMode = "log" | "length" | "ratio";

export interface LengthSettings {
  lengthMode: LengthMode;
  logA: number; // log mode: output seconds = logA * log10(input seconds + 1) + logB
  logB: number;
  targetSec: number; // length mode
  ratio: number; // ratio mode: input duration / output duration
  ramp: boolean; // linear compression stepping (COMPSTEPMODE:LINEAR)
  rampStartRatio: number; // CS_START_RAT
}

export interface WindowSamples {
  window: number;
  overlap: number;
  maxSkew: number;
}

export type RatioPlan =
  | { ok: true; ratio: number; ratioStep: number; outSeconds: number }
  | { ok: false; error: string };

/** Speed ratio (input/output duration) for a file of `numSamples` at `sampleRate`, mirroring genSpearcon(). */
export function planRatio(s: LengthSettings, numSamples: number, sampleRate: number, win: WindowSamples): RatioPlan {
  const fileLen = numSamples / sampleRate;
  let ratio: number;
  switch (s.lengthMode) {
    case "log": {
      const newLen = s.logA * Math.log10(fileLen + 1.0) + s.logB;
      if (!(newLen > 0)) return { ok: false, error: "Log formula gives an output length of zero or less" };
      ratio = fileLen / newLen;
      break;
    }
    case "length":
      if (!(s.targetSec > 0)) return { ok: false, error: "Target length must be more than 0" };
      ratio = fileLen / s.targetSec;
      break;
    case "ratio":
      ratio = s.ratio;
      break;
  }
  if (!(ratio > 0) || !Number.isFinite(ratio)) return { ok: false, error: "Speed ratio must be more than 0" };

  let ratioStep = 0;
  if (s.ramp) {
    // genSpearcon(), CS_LINEAR: the ratio grows by m each synthesis step, chosen so the steps still
    // add up to the input length. C int truncation is reproduced with Math.trunc.
    const SD = win.window - win.overlap;
    const inFileNumSamples = numSamples;
    const outFileNumSamples = Math.trunc(inFileNumSamples / ratio);
    const N = Math.trunc(outFileNumSamples / SD) - 2;
    const startRatio = s.rampStartRatio;
    if (N < 1) return { ok: false, error: "Output too short for a speed-up ramp" };
    if (!(startRatio > 0)) return { ok: false, error: "Ramp start ratio must be more than 0" };
    const m = ((inFileNumSamples / SD - N * startRatio) * 2) / (N * (N + 1));
    if (!(startRatio + N * m > 0)) return { ok: false, error: "Ramp start ratio is too high for this length" };
    ratio = startRatio;
    ratioStep = m;
    return { ok: true, ratio, ratioStep, outSeconds: outFileNumSamples / sampleRate };
  }
  return { ok: true, ratio, ratioStep, outSeconds: fileLen / ratio };
}

/** Window, overlap and max skew in samples at `sampleRate`, plus the checks SOLAFS_BeginStream() makes. */
export function windowSamples(windowMs: number, overlapMs: number, maxSkewMs: number, sampleRate: number):
  | ({ ok: true } & WindowSamples)
  | { ok: false; error: string } {
  const toSamples = (ms: number) => Math.round((ms * sampleRate) / 1000);
  const window = toSamples(windowMs);
  const overlap = toSamples(overlapMs);
  const maxSkew = toSamples(maxSkewMs);
  if (overlap < 1 || window < 2 || maxSkew < 1) return { ok: false, error: "Window, overlap and max skew must be at least 1 sample" };
  if (2 * overlap > window) return { ok: false, error: "Overlap can be at most half the window" };
  return { ok: true, window, overlap, maxSkew };
}

/**
 * Shortest input SOLAFS() accepts: it primes a circular buffer of window + maxSkew samples (rounded up to
 * a power of two) and gives up if the file can't fill it.
 */
export function minInputSamples(win: WindowSamples): number {
  let n = 1;
  while (n < win.window + win.maxSkew) n *= 2;
  return n;
}
