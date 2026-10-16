import type { LengthSettings } from "../engine/spearconMath";

export interface Settings extends LengthSettings {
  trim: boolean;
  trimThresholdDb: number;
  trimMarginMs: number;
  fadeInMs: number;
  fadeOutMs: number;
  padStartMs: number;
  padEndMs: number;
  sampleRate: number;
  suffix: string;
  windowMs: number;
  overlapMs: number;
  maxSkewMs: number;
  playGapMs: number;
}

// spearcon.exe's window defaults are in samples (300 / 150 / 600). These are those at 44.1 kHz,
// to 4 decimals, so 44.1 kHz output uses exactly the same windows.
const SAMPLES_AT_44K = (n: number) => Math.round((n / 44.1) * 1e4) / 1e4;

/** What spearcon.exe does with no options: log length, no trimming, fades or padding. */
export const SPEARCON_EXE: Settings = {
  lengthMode: "log",
  logA: 1.0,
  logB: 0.25,
  targetSec: 0.25,
  ratio: 2.0,
  ramp: false,
  rampStartRatio: 1.0,
  trim: false,
  trimThresholdDb: -45,
  trimMarginMs: 5,
  fadeInMs: 0,
  fadeOutMs: 0,
  padStartMs: 0,
  padEndMs: 0,
  sampleRate: 44100,
  suffix: "_spearcon",
  windowMs: SAMPLES_AT_44K(300),
  overlapMs: SAMPLES_AT_44K(150),
  maxSkewMs: SAMPLES_AT_44K(600),
  playGapMs: 250,
};

/** The app's defaults: spearcon.exe plus silence trimming and short fades, which most source clips need. */
export const DEFAULTS: Settings = {
  ...SPEARCON_EXE,
  trim: true,
  fadeInMs: 2,
  fadeOutMs: 5,
};

export const SAMPLE_RATES = [16000, 22050, 32000, 44100, 48000];

const KEY = "spearcon-factory.settings.v1";

export function loadSettings(): Settings {
  try {
    const raw = localStorage.getItem(KEY);
    if (raw) return { ...DEFAULTS, ...JSON.parse(raw) };
  } catch {
    /* ignore unreadable settings */
  }
  return DEFAULTS;
}

export function saveSettings(s: Settings) {
  try {
    localStorage.setItem(KEY, JSON.stringify(s));
  } catch {
    /* storage full or disabled */
  }
}
