// Web Worker: one engine instance per worker. SOLAFS keeps its state in C globals, so a module
// instance must process one clip at a time; running several workers is how the app goes parallel.
import { Engine, type EngineParams } from "./engine";

export interface JobRequest {
  id: number;
  samples: Float32Array;
  sampleRate: number;
  params: EngineParams;
}

export type JobResponse = { id: number; ok: true; samples: Float32Array } | { id: number; ok: false; error: string };

const enginePromise = Engine.create();

self.onmessage = async (e: MessageEvent<JobRequest>) => {
  const { id, samples, sampleRate, params } = e.data;
  let res: JobResponse;
  try {
    const out = (await enginePromise).run(samples, sampleRate, params);
    res = out ? { id, ok: true, samples: out } : { id, ok: false, error: "SOLAFS could not process this file" };
  } catch (err) {
    res = { id, ok: false, error: err instanceof Error ? err.message : String(err) };
  }
  const transfer = res.ok ? [res.samples.buffer] : [];
  (self as unknown as Worker).postMessage(res, transfer);
};
