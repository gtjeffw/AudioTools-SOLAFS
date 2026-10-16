import { unzipSync } from "fflate";
import { describe, expect, it } from "vitest";
import { applyFades, dbToGain, pad, trimSilence } from "../src/audio/dsp";
import { decodeWav16, encodeWav16 } from "../src/audio/wav";
import { buildZip, outputNames } from "../src/export/zip";

describe("WAV", () => {
  it("round-trips 16-bit values exactly", () => {
    const ints = [-32768, -1, 0, 1, 12345, 32767];
    const x = Float32Array.from(ints, (v) => v / 32768);
    const { samples, sampleRate } = decodeWav16(encodeWav16(x, 22050));
    expect(sampleRate).toBe(22050);
    expect(Array.from(samples, (v) => Math.round(v * 32768))).toEqual(ints);
  });

  it("clamps out-of-range floats", () => {
    const { samples } = decodeWav16(encodeWav16(Float32Array.from([1.5, -1.5]), 8000));
    expect(Array.from(samples, (v) => Math.round(v * 32768))).toEqual([32767, -32768]);
  });
});

describe("trimSilence", () => {
  const rate = 1000; // 1 sample per ms keeps the arithmetic readable
  const x = new Float32Array(100);
  x.fill(0.5, 40, 60);
  x[10] = dbToGain(-60); // below a -45 dB threshold
  it("cuts to the loud part plus the margin", () => {
    const t = trimSilence(x, rate, -45, 5)!;
    expect(t.length).toBe(20 + 2 * 5);
    expect(t[5]).toBe(0.5);
  });
  it("returns null when nothing reaches the threshold", () => {
    expect(trimSilence(new Float32Array(50), rate, -45, 5)).toBeNull();
  });
});

describe("fades and padding", () => {
  it("fades linearly from and to zero", () => {
    const x = applyFades(new Float32Array(100).fill(1), 1000, 10, 20);
    expect(x[0]).toBe(0);
    expect(x[5]).toBeCloseTo(0.5);
    expect(x[50]).toBe(1);
    expect(x[99]).toBe(0);
    expect(x[89]).toBeCloseTo(0.5);
  });
  it("pads with silence", () => {
    const p = pad(new Float32Array([1, 1]), 1000, 3, 2);
    expect(Array.from(p)).toEqual([0, 0, 0, 1, 1, 0, 0]);
  });
});

describe("export", () => {
  it("names outputs and de-duplicates", () => {
    expect(outputNames(["a", "b", "a", "A"], "_spearcon")).toEqual(["a_spearcon.wav", "b_spearcon.wav", "a_spearcon (2).wav", "A_spearcon (3).wav"]);
    expect(outputNames(["x/y:z"], "")).toEqual(["x_y_z.wav"]);
  });
  it("zips the WAVs with a settings manifest", () => {
    const wav = encodeWav16(new Float32Array(10), 8000);
    const files = unzipSync(buildZip([{ name: "a_spearcon.wav", bytes: wav }], { hello: 1 }));
    expect(Object.keys(files).sort()).toEqual(["a_spearcon.wav", "spearcon-settings.json"]);
    expect(files["a_spearcon.wav"]).toEqual(wav);
    expect(JSON.parse(new TextDecoder().decode(files["spearcon-settings.json"]))).toEqual({ hello: 1 });
  });
});
