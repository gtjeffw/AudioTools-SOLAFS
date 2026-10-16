// Playback through one shared AudioContext. Starting anything stops whatever was playing.
import { guessPlatform } from "../platform";
import { encodeWav16 } from "./wav";

export interface Clip {
  samples: Float32Array;
  sampleRate: number;
}

export class Player {
  private ctx: AudioContext | null = null;
  private sources: AudioBufferSourceNode[] = [];
  private timers: number[] = [];
  private token = 0;

  private context(): AudioContext {
    if (!this.ctx) {
      allowPlaybackWhenSilenced();
      this.ctx = new AudioContext();
    }
    if (this.ctx.state === "suspended") void this.ctx.resume();
    return this.ctx;
  }

  stop() {
    this.token++;
    for (const s of this.sources) {
      try {
        s.stop();
      } catch {
        /* already stopped */
      }
    }
    this.sources = [];
    for (const t of this.timers) clearTimeout(t);
    this.timers = [];
  }

  /** Play one clip. `onEnd` runs when it finishes (not when stopped early). */
  play(clip: Clip, onEnd?: () => void) {
    this.playSequence([clip], 0, undefined, onEnd);
  }

  /** Play clips back to back with `gapMs` between them, calling `onIndex` as each one starts. */
  playSequence(clips: Clip[], gapMs: number, onIndex?: (i: number) => void, onEnd?: () => void) {
    this.stop();
    const token = this.token;
    const ctx = this.context();
    let t = ctx.currentTime + 0.05;
    clips.forEach((clip, i) => {
      const buf = ctx.createBuffer(1, Math.max(1, clip.samples.length), clip.sampleRate);
      buf.copyToChannel(clip.samples as Float32Array<ArrayBuffer>, 0);
      const src = ctx.createBufferSource();
      src.buffer = buf;
      src.connect(ctx.destination);
      src.start(t);
      this.sources.push(src);
      const startDelay = Math.max(0, (t - ctx.currentTime) * 1000);
      this.timers.push(window.setTimeout(() => token === this.token && onIndex?.(i), startDelay));
      if (i === clips.length - 1) src.onended = () => token === this.token && onEnd?.();
      t += buf.duration + gapMs / 1000;
    });
  }
}

/**
 * iOS treats Web Audio as "ambient" sound, which the ring/silent switch mutes (media elements such
 * as <video> are "playback" and are not muted). Ask for playback behaviour instead:
 * - iOS/Safari 17+: the Audio Session API, navigator.audioSession.type = "playback".
 * - Older iOS: playing a media element during the user's tap switches the page's audio session to
 *   playback, which then applies to Web Audio too; a fraction of a second of silence is enough.
 * Must run inside a user gesture (it does: the first play button press).
 */
function allowPlaybackWhenSilenced() {
  const session = (navigator as Navigator & { audioSession?: { type: string } }).audioSession;
  if (session) {
    session.type = "playback";
    return;
  }
  if (guessPlatform() !== "ios") return;
  const url = URL.createObjectURL(new Blob([encodeWav16(new Float32Array(4410), 44100) as Uint8Array<ArrayBuffer>], { type: "audio/wav" }));
  const el = new Audio(url);
  el.setAttribute("playsinline", "");
  void el.play().catch(() => {}).finally(() => setTimeout(() => URL.revokeObjectURL(url), 1000));
}
