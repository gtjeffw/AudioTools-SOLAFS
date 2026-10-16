// The WebAssembly engine + the TypeScript ratio math must reproduce spearcon.exe byte for byte.
// fixtures/native-spearcon.json holds SHA-256s of spearcon.exe outputs for native/bin/audio files.
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { beforeAll, describe, expect, it } from "vitest";
import { decodeWav16 } from "../src/audio/wav";
import { Engine } from "../src/engine/engine";
import { planRatio, type LengthSettings } from "../src/engine/spearconMath";

const AUDIO = join(__dirname, "../../native/bin/audio");
const fixture = JSON.parse(readFileSync(join(__dirname, "fixtures/native-spearcon.json"), "utf8")) as {
  cases: { input: string; args: string; sha256: string }[];
};

/** spearcon.exe's command-line options, as processArgs() reads them. */
function parseArgs(args: string) {
  const s: LengthSettings = { lengthMode: "log", logA: 1, logB: 0.25, targetSec: 0.25, ratio: 1, ramp: false, rampStartRatio: 1 };
  const win = { window: 300, overlap: 150, maxSkew: 600 };
  for (const a of args.split(" ")) {
    const [k, v] = a.split(":");
    if (k === "LENGTH_SEC") Object.assign(s, { lengthMode: "length", targetSec: Number(v) });
    else if (k === "IN_TO_OUT_RATIO") Object.assign(s, { lengthMode: "ratio", ratio: Number(v) });
    else if (k === "LOG_TIME_A") Object.assign(s, { lengthMode: "log", logA: Number(v) });
    else if (k === "LOG_TIME_B") Object.assign(s, { lengthMode: "log", logB: Number(v) });
    else if (k === "COMPSTEPMODE") s.ramp = v === "LINEAR";
    else if (k === "CS_START_RAT") Object.assign(s, { ramp: true, rampStartRatio: Number(v) });
    else if (k === "SOLAFS_WINDOWSIZE") win.window = Number(v);
    else if (k === "SOLAFS_OVERLAP") win.overlap = Number(v);
    else if (k === "SOLAFS_MAXSKEW") win.maxSkew = Number(v);
  }
  return { s, win };
}

let engine: Engine;
beforeAll(async () => {
  engine = await Engine.create();
});

describe("WebAssembly engine matches native spearcon.exe", () => {
  for (const c of fixture.cases) {
    it(`${c.input} ${c.args}`, () => {
      const wav = new Uint8Array(readFileSync(join(AUDIO, c.input)));
      const { samples, sampleRate } = decodeWav16(wav);
      const { s, win } = parseArgs(c.args);
      const plan = planRatio(s, samples.length, sampleRate, win);
      expect(plan.ok).toBe(true);
      if (!plan.ok) return;
      const out = engine.runWav(wav, { ratio: plan.ratio, ratioStep: plan.ratioStep, ...win });
      expect(out).not.toBeNull();
      expect(createHash("sha256").update(out!).digest("hex")).toBe(c.sha256);
    });
  }
});

describe("Engine.run (float in, float out)", () => {
  it("round-trips through 16-bit WAV and shortens by the ratio", () => {
    const { samples, sampleRate } = decodeWav16(new Uint8Array(readFileSync(join(AUDIO, "bruce_walker.wav"))));
    const out = engine.run(samples, sampleRate, { ratio: 2, ratioStep: 0, window: 300, overlap: 150, maxSkew: 600 });
    expect(out).not.toBeNull();
    expect(out!.length / samples.length).toBeCloseTo(0.5, 1);
  });

  it("returns null for a clip too short for the window settings", () => {
    expect(engine.run(new Float32Array(500), 44100, { ratio: 2, ratioStep: 0, window: 300, overlap: 150, maxSkew: 600 })).toBeNull();
  });
});
