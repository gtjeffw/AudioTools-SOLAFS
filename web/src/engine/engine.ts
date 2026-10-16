// Runs the WebAssembly SOLAFS engine on one clip: write a 16-bit WAV into the module's in-memory
// filesystem, call solafs_file() (native code path, see web/wasm/solafs_wasm.cpp), read the result back.
import createSolafsModule, { type SolafsModule } from "./generated/solafs.mjs";
import { decodeWav16, encodeWav16 } from "../audio/wav";

export interface EngineParams {
  ratio: number;
  ratioStep: number;
  window: number;
  overlap: number;
  maxSkew: number;
}

export class Engine {
  private constructor(private readonly mod: SolafsModule) {}

  static async create(): Promise<Engine> {
    return new Engine(await createSolafsModule({ print: () => {}, printErr: (s) => console.warn("[solafs]", s) }));
  }

  /** WAV bytes in, WAV bytes out (null if SOLAFS rejected the input). */
  runWav(wav: Uint8Array, p: EngineParams): Uint8Array | null {
    const { FS } = this.mod;
    FS.writeFile("/in.wav", wav);
    try {
      const ok = this.mod.ccall(
        "solafs_file",
        "number",
        ["string", "string", "number", "number", "number", "number", "number"],
        ["/in.wav", "/out.wav", p.ratio, p.ratioStep, p.window, p.overlap, p.maxSkew],
      );
      return ok ? FS.readFile("/out.wav") : null;
    } finally {
      FS.unlink("/in.wav");
      if (FS.analyzePath("/out.wav").exists) FS.unlink("/out.wav");
    }
  }

  run(samples: Float32Array, sampleRate: number, p: EngineParams): Float32Array | null {
    const out = this.runWav(encodeWav16(samples, sampleRate), p);
    return out ? decodeWav16(out).samples : null;
  }
}
