// Decode an audio file into mono floats at the output sample rate.
//
// 16-bit PCM WAV is parsed directly. decodeAudioData isn't exact for it: Chrome scales positive
// samples by 1/32767 and negative ones by 1/32768, so a round trip back to 16 bits moves large
// positive samples by one step, and the output no longer matches spearcon.exe for the same file.
// Everything else (MP3, M4A, OGG, FLAC, 24-bit WAV…) goes through the browser's decoder.
import { decodeWav16 } from "./wav";

export async function decodeToMono(bytes: ArrayBuffer, sampleRate: number): Promise<Float32Array> {
  const wav = tryDecodeWav16(bytes);
  if (wav) return wav.sampleRate === sampleRate ? wav.samples : resample(wav.samples, wav.sampleRate, sampleRate);

  // decodeAudioData resamples to its context's rate, so decoding at `sampleRate` resamples too.
  const ctx = new OfflineAudioContext(1, 1, sampleRate);
  const buf = await ctx.decodeAudioData(bytes.slice(0)); // decodeAudioData detaches its input
  return mixDown(buf);
}

function tryDecodeWav16(bytes: ArrayBuffer) {
  try {
    return decodeWav16(new Uint8Array(bytes));
  } catch {
    return null; // not RIFF/WAVE, or not 16-bit PCM
  }
}

async function resample(x: Float32Array, from: number, to: number): Promise<Float32Array> {
  const ctx = new OfflineAudioContext(1, Math.max(1, Math.ceil((x.length * to) / from)), to);
  const buf = ctx.createBuffer(1, Math.max(1, x.length), from);
  buf.copyToChannel(x as Float32Array<ArrayBuffer>, 0);
  const src = ctx.createBufferSource();
  src.buffer = buf;
  src.connect(ctx.destination);
  src.start();
  return (await ctx.startRendering()).getChannelData(0).slice();
}

function mixDown(buf: AudioBuffer): Float32Array {
  if (buf.numberOfChannels === 1) return buf.getChannelData(0).slice();
  const out = new Float32Array(buf.length);
  for (let c = 0; c < buf.numberOfChannels; c++) {
    const ch = buf.getChannelData(c);
    for (let i = 0; i < out.length; i++) out[i] += ch[i];
  }
  for (let i = 0; i < out.length; i++) out[i] /= buf.numberOfChannels;
  return out;
}
