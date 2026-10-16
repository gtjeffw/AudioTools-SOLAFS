// One clip from decoded input to finished spearcon: trim → SOLAFS (in a worker) → fades → padding.
import { applyFades, pad, trimSilence } from "../audio/dsp";
import type { Settings } from "../state/settings";
import type { EnginePool } from "./pool";
import { minInputSamples, planRatio, windowSamples } from "./spearconMath";

export type PipelineResult =
  | { ok: true; output: Float32Array; ratio: number; trimmedSeconds: number }
  | { ok: false; error: string; trimmedSeconds?: number };

export async function makeSpearcon(
  pool: EnginePool,
  input: Float32Array,
  s: Settings,
  cancelled: () => boolean,
): Promise<PipelineResult> {
  const rate = s.sampleRate;
  const trimmed = s.trim ? trimSilence(input, rate, s.trimThresholdDb, s.trimMarginMs) : input;
  if (!trimmed) return { ok: false, error: `Nothing louder than ${s.trimThresholdDb} dBFS (lower the trim threshold)` };
  const trimmedSeconds = trimmed.length / rate;

  const win = windowSamples(s.windowMs, s.overlapMs, s.maxSkewMs, rate);
  if (!win.ok) return { ok: false, error: win.error, trimmedSeconds };

  const need = minInputSamples(win);
  if (trimmed.length < need) {
    const ms = (n: number) => `${Math.round((n / rate) * 1000)} ms`;
    return { ok: false, error: `Too short (${ms(trimmed.length)}); these window settings need at least ${ms(need)}`, trimmedSeconds };
  }

  const plan = planRatio(s, trimmed.length, rate, win);
  if (!plan.ok) return { ok: false, error: plan.error, trimmedSeconds };

  const out = await pool.run(
    trimmed,
    rate,
    { ratio: plan.ratio, ratioStep: plan.ratioStep, window: win.window, overlap: win.overlap, maxSkew: win.maxSkew },
    cancelled,
  );
  const shaped = pad(applyFades(out, rate, s.fadeInMs, s.fadeOutMs), rate, s.padStartMs, s.padEndMs);
  // with a ramp, report the average speed-up rather than the starting one
  const ratio = s.ramp ? trimmedSeconds / (out.length / rate) : plan.ratio;
  return { ok: true, output: shaped, ratio, trimmedSeconds };
}
