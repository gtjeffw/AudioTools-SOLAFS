// 16-bit PCM WAV encode/decode, mono. This is the format the native engine reads and writes.

export interface PcmAudio {
  samples: Float32Array; // -1..1
  sampleRate: number;
}

/** Float samples (-1..1) to a mono 16-bit PCM WAV file. */
export function encodeWav16(samples: Float32Array, sampleRate: number): Uint8Array {
  const dataBytes = samples.length * 2;
  const buf = new ArrayBuffer(44 + dataBytes);
  const v = new DataView(buf);
  const ascii = (off: number, s: string) => {
    for (let i = 0; i < s.length; i++) v.setUint8(off + i, s.charCodeAt(i));
  };
  ascii(0, "RIFF");
  v.setUint32(4, 36 + dataBytes, true);
  ascii(8, "WAVE");
  ascii(12, "fmt ");
  v.setUint32(16, 16, true);
  v.setUint16(20, 1, true); // PCM
  v.setUint16(22, 1, true); // mono
  v.setUint32(24, sampleRate, true);
  v.setUint32(28, sampleRate * 2, true);
  v.setUint16(32, 2, true);
  v.setUint16(34, 16, true);
  ascii(36, "data");
  v.setUint32(40, dataBytes, true);
  for (let i = 0; i < samples.length; i++) {
    // x*32768 rounded and clamped: exact for audio that came from a 16-bit source
    const s = Math.max(-32768, Math.min(32767, Math.round(samples[i] * 32768)));
    v.setInt16(44 + i * 2, s, true);
  }
  return new Uint8Array(buf);
}

/** Mono or multichannel 16-bit PCM WAV to mono floats (channels averaged). */
export function decodeWav16(bytes: Uint8Array): PcmAudio {
  const v = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const tag = (off: number) => String.fromCharCode(v.getUint8(off), v.getUint8(off + 1), v.getUint8(off + 2), v.getUint8(off + 3));
  if (bytes.length < 12 || tag(0) !== "RIFF" || tag(8) !== "WAVE") throw new Error("Not a WAV file");
  let off = 12;
  let sampleRate = 0;
  let channels = 0;
  let bits = 0;
  while (off + 8 <= bytes.length) {
    const id = tag(off);
    const size = v.getUint32(off + 4, true);
    const body = off + 8;
    if (id === "fmt ") {
      if (v.getUint16(body, true) !== 1) throw new Error("Only PCM WAV is supported");
      channels = v.getUint16(body + 2, true);
      sampleRate = v.getUint32(body + 4, true);
      bits = v.getUint16(body + 14, true);
    } else if (id === "data") {
      if (bits !== 16 || channels < 1) throw new Error("Only 16-bit PCM WAV is supported");
      const frames = Math.floor(Math.min(size, bytes.length - body) / (2 * channels));
      const out = new Float32Array(frames);
      for (let i = 0; i < frames; i++) {
        let acc = 0;
        for (let c = 0; c < channels; c++) acc += v.getInt16(body + (i * channels + c) * 2, true);
        out[i] = acc / channels / 32768;
      }
      return { samples: out, sampleRate };
    }
    off = body + size + (size & 1); // chunks are word-aligned
  }
  throw new Error("WAV file has no data chunk");
}
