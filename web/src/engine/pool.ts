// A small pool of engine workers. Jobs queue up; each worker takes the next one when it's free.
import type { EngineParams } from "./engine";
import type { JobRequest, JobResponse } from "./worker";

interface Pending {
  req: JobRequest;
  resolve: (r: Float32Array) => void;
  reject: (e: Error) => void;
  cancelled: () => boolean;
}

export class CancelledError extends Error {
  constructor() {
    super("cancelled");
  }
}

export class EnginePool {
  private idle: Worker[] = [];
  private queue: Pending[] = [];
  private running = new Map<number, Pending>();
  private nextId = 1;

  constructor(size = defaultPoolSize()) {
    for (let i = 0; i < size; i++) {
      const w = new Worker(new URL("./worker.ts", import.meta.url), { type: "module" });
      w.onmessage = (e: MessageEvent<JobResponse>) => this.finish(w, e.data);
      this.idle.push(w);
    }
  }

  /** Process one clip. Rejects with CancelledError if `cancelled()` turns true before it starts. */
  run(samples: Float32Array, sampleRate: number, params: EngineParams, cancelled: () => boolean = () => false): Promise<Float32Array> {
    return new Promise((resolve, reject) => {
      // copy, since the buffer is transferred to the worker
      const req: JobRequest = { id: this.nextId++, samples: samples.slice(), sampleRate, params };
      this.queue.push({ req, resolve, reject, cancelled });
      this.pump();
    });
  }

  private pump() {
    while (this.idle.length && this.queue.length) {
      const job = this.queue.shift()!;
      if (job.cancelled()) {
        job.reject(new CancelledError());
        continue;
      }
      const w = this.idle.pop()!;
      this.running.set(job.req.id, job);
      w.postMessage(job.req, [job.req.samples.buffer]);
    }
  }

  private finish(w: Worker, res: JobResponse) {
    const job = this.running.get(res.id);
    this.running.delete(res.id);
    this.idle.push(w);
    if (job) {
      if (res.ok) job.resolve(res.samples);
      else job.reject(new Error(res.error));
    }
    this.pump();
  }
}

function defaultPoolSize() {
  const cores = typeof navigator !== "undefined" && navigator.hardwareConcurrency ? navigator.hardwareConcurrency : 2;
  return Math.max(1, Math.min(4, cores - 1));
}
