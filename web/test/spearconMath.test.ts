import { describe, expect, it } from "vitest";
import { minInputSamples, planRatio, windowSamples, type LengthSettings } from "../src/engine/spearconMath";

const base: LengthSettings = { lengthMode: "log", logA: 1, logB: 0.25, targetSec: 0.25, ratio: 2, ramp: false, rampStartRatio: 1 };
const win = { window: 300, overlap: 150, maxSkew: 600 };
const rate = 44100;

describe("planRatio", () => {
  it("log mode: output = A*log10(len+1)+B", () => {
    const n = 2 * rate; // 2 s
    const p = planRatio(base, n, rate, win);
    expect(p.ok && p.ratio).toBeCloseTo(2 / (Math.log10(3) + 0.25), 12);
    expect(p.ok && p.outSeconds).toBeCloseTo(Math.log10(3) + 0.25, 12);
  });

  it("length and ratio modes", () => {
    const n = 3 * rate;
    const len = planRatio({ ...base, lengthMode: "length", targetSec: 0.5 }, n, rate, win);
    expect(len.ok && len.ratio).toBeCloseTo(6, 12);
    const r = planRatio({ ...base, lengthMode: "ratio", ratio: 1.5 }, n, rate, win);
    expect(r.ok && r.outSeconds).toBeCloseTo(2, 12);
  });

  it("linear ramp: genSpearcon's step, with C int truncation", () => {
    const n = 54740; // bruce_walker.wav
    const p = planRatio({ ...base, lengthMode: "length", targetSec: 0.3, ramp: true, rampStartRatio: 0.1 }, n, rate, win);
    const ratio = n / rate / 0.3;
    const SD = 150;
    const N = Math.trunc(Math.trunc(n / ratio) / SD) - 2;
    const m = ((n / SD - N * 0.1) * 2) / (N * (N + 1));
    expect(p.ok && p.ratio).toBe(0.1);
    expect(p.ok && p.ratioStep).toBe(m);
    // the steps add up to the input length: sum over N synthesis steps of SD*(start + k*m)
    expect(N * 0.1 * SD + ((m * N * (N + 1)) / 2) * SD).toBeCloseTo(n, 6);
  });

  it("rejects settings spearcon.exe would run with a bad ratio", () => {
    expect(planRatio({ ...base, logA: 1, logB: -5 }, rate, rate, win).ok).toBe(false);
    expect(planRatio({ ...base, lengthMode: "length", targetSec: 0 }, rate, rate, win).ok).toBe(false);
    expect(planRatio({ ...base, lengthMode: "ratio", ratio: 0 }, rate, rate, win).ok).toBe(false);
    // 1 s at 200x is 220 output samples: fewer than 3 synthesis steps of 150, so N < 1
    expect(planRatio({ ...base, lengthMode: "ratio", ratio: 200, ramp: true }, rate, rate, win).ok).toBe(false);
    expect(planRatio({ ...base, lengthMode: "ratio", ratio: 2, ramp: true, rampStartRatio: 10 }, rate, rate, win).ok).toBe(false);
  });
});

describe("windowSamples", () => {
  it("converts ms to samples and enforces overlap <= window/2", () => {
    expect(windowSamples(6.8027, 3.4014, 13.6054, 44100)).toEqual({ ok: true, window: 300, overlap: 150, maxSkew: 600 });
    expect(windowSamples(10, 6, 10, 44100).ok).toBe(false);
  });
});

describe("minInputSamples", () => {
  it("is window+maxSkew rounded up to a power of two (SOLAFS's input buffer)", () => {
    expect(minInputSamples(win)).toBe(1024);
    expect(minInputSamples({ window: 400, overlap: 200, maxSkew: 624 })).toBe(1024);
    expect(minInputSamples({ window: 400, overlap: 200, maxSkew: 625 })).toBe(2048);
  });
});
