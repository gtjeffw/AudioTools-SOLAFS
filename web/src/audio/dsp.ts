// Pre/post-processing around SOLAFS, in the spirit of native/SilentStripper: trim silence, fade, pad.
// The threshold is in dBFS of the -1..1 signal (SilentStripper compared raw 16-bit values).

export function dbToGain(db: number): number {
  return Math.pow(10, db / 20);
}

/**
 * Cut leading and trailing silence: everything before the first and after the last sample at or above
 * `thresholdDb`, keeping `marginMs` of context on each side. Returns null if nothing reaches the threshold.
 */
export function trimSilence(x: Float32Array, sampleRate: number, thresholdDb: number, marginMs: number): Float32Array | null {
  const thr = dbToGain(thresholdDb);
  let first = -1;
  let last = -1;
  for (let i = 0; i < x.length; i++) {
    if (Math.abs(x[i]) >= thr) {
      if (first < 0) first = i;
      last = i;
    }
  }
  if (first < 0) return null;
  const margin = Math.round((marginMs * sampleRate) / 1000);
  return x.slice(Math.max(0, first - margin), Math.min(x.length, last + 1 + margin));
}

/** Linear fade-in and fade-out, in place. Each fade is capped at half the clip. */
export function applyFades(x: Float32Array, sampleRate: number, inMs: number, outMs: number): Float32Array {
  const half = Math.floor(x.length / 2);
  const nIn = Math.min(half, Math.round((inMs * sampleRate) / 1000));
  const nOut = Math.min(half, Math.round((outMs * sampleRate) / 1000));
  for (let i = 0; i < nIn; i++) x[i] *= i / nIn;
  for (let i = 0; i < nOut; i++) x[x.length - 1 - i] *= i / nOut;
  return x;
}

/** Silence before and after. */
export function pad(x: Float32Array, sampleRate: number, startMs: number, endMs: number): Float32Array {
  const a = Math.round((startMs * sampleRate) / 1000);
  const b = Math.round((endMs * sampleRate) / 1000);
  if (a === 0 && b === 0) return x;
  const out = new Float32Array(a + x.length + b);
  out.set(x, a);
  return out;
}

/** Peak level in dBFS (for display). */
export function peakDb(x: Float32Array): number {
  let p = 0;
  for (let i = 0; i < x.length; i++) p = Math.max(p, Math.abs(x[i]));
  return p > 0 ? 20 * Math.log10(p) : -Infinity;
}
