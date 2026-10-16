import type { LengthMode } from "../engine/spearconMath";
import { windowSamples } from "../engine/spearconMath";
import { DEFAULTS, SAMPLE_RATES, type Settings } from "../state/settings";
import { NumberField } from "./NumberField";

interface Props {
  settings: Settings;
  onChange: (s: Settings) => void;
}

const MODES: { value: LengthMode; label: string }[] = [
  { value: "log", label: "Log of length" },
  { value: "length", label: "Target length" },
  { value: "ratio", label: "Speed ratio" },
];

export function SettingsPanel({ settings: s, onChange }: Props) {
  const set = <K extends keyof Settings>(k: K, v: Settings[K]) => onChange({ ...s, [k]: v });
  const win = windowSamples(s.windowMs, s.overlapMs, s.maxSkewMs, s.sampleRate);
  const samples = (n: number) => `${n} samples at ${s.sampleRate / 1000} kHz`;

  return (
    <aside className="panel settings" aria-label="Settings">
      <div className="panel-head">
        <h2>Settings</h2>
        <button type="button" className="btn-small" onClick={() => onChange(DEFAULTS)}>Reset to defaults</button>
      </div>

      <section>
        <h3>Length</h3>
        <div className="segmented" role="radiogroup" aria-label="Length mode">
          {MODES.map((m) => (
            <label key={m.value} className={s.lengthMode === m.value ? "on" : ""}>
              <input type="radio" name="mode" value={m.value} checked={s.lengthMode === m.value}
                onChange={() => set("lengthMode", m.value)} />
              {m.label}
            </label>
          ))}
        </div>
        {s.lengthMode === "log" && (
          <>
            <p className="note">Output seconds = A · log₁₀(input seconds + 1) + B</p>
            <div className="grid2">
              <NumberField label="A" value={s.logA} step={0.05} onChange={(v) => set("logA", v)} />
              <NumberField label="B" value={s.logB} step={0.05} unit="s" onChange={(v) => set("logB", v)} />
            </div>
          </>
        )}
        {s.lengthMode === "length" && (
          <NumberField label="Every spearcon lasts" value={s.targetSec} step={0.05} min={0.01} unit="s"
            onChange={(v) => set("targetSec", v)} />
        )}
        {s.lengthMode === "ratio" && (
          <NumberField label="Speed-up" value={s.ratio} step={0.1} min={0.05} unit="×"
            hint="Input length ÷ output length; below 1 slows down" onChange={(v) => set("ratio", v)} />
        )}
        <label className="check">
          <input type="checkbox" checked={s.ramp} onChange={(e) => set("ramp", e.target.checked)} />
          Speed-up ramp (start slower, end faster; same total length)
        </label>
        {s.ramp && (
          <NumberField label="Starting speed" value={s.rampStartRatio} step={0.05} min={0.01} unit="×"
            onChange={(v) => set("rampStartRatio", v)} />
        )}
      </section>

      <section>
        <h3>Clean-up</h3>
        <label className="check">
          <input type="checkbox" checked={s.trim} onChange={(e) => set("trim", e.target.checked)} />
          Trim silence at the start and end first
        </label>
        {s.trim && (
          <div className="grid2">
            <NumberField label="Threshold" value={s.trimThresholdDb} step={1} max={0} unit="dBFS"
              onChange={(v) => set("trimThresholdDb", v)} />
            <NumberField label="Keep" value={s.trimMarginMs} step={1} min={0} unit="ms"
              onChange={(v) => set("trimMarginMs", v)} />
          </div>
        )}
        <div className="grid2">
          <NumberField label="Fade in" value={s.fadeInMs} step={1} min={0} unit="ms" onChange={(v) => set("fadeInMs", v)} />
          <NumberField label="Fade out" value={s.fadeOutMs} step={1} min={0} unit="ms" onChange={(v) => set("fadeOutMs", v)} />
          <NumberField label="Pad start" value={s.padStartMs} step={5} min={0} unit="ms" onChange={(v) => set("padStartMs", v)} />
          <NumberField label="Pad end" value={s.padEndMs} step={5} min={0} unit="ms" onChange={(v) => set("padEndMs", v)} />
        </div>
      </section>

      <section>
        <h3>Output</h3>
        <div className="grid2">
          <label className="field">
            <span className="field-label">Sample rate</span>
            <select value={s.sampleRate} onChange={(e) => set("sampleRate", Number(e.target.value))}>
              {SAMPLE_RATES.map((r) => <option key={r} value={r}>{r / 1000} kHz</option>)}
            </select>
          </label>
          <label className="field">
            <span className="field-label">File name suffix</span>
            <input type="text" value={s.suffix} onChange={(e) => set("suffix", e.target.value)} />
          </label>
        </div>
      </section>

      <details>
        <summary>SOLAFS window (advanced)</summary>
        <div className="grid3">
          <NumberField label="Window" value={s.windowMs} step={0.1} min={0.1} unit="ms"
            hint={win.ok ? samples(win.window) : undefined} onChange={(v) => set("windowMs", v)} />
          <NumberField label="Overlap" value={s.overlapMs} step={0.1} min={0.05} unit="ms"
            hint={win.ok ? samples(win.overlap) : undefined} onChange={(v) => set("overlapMs", v)} />
          <NumberField label="Max skew" value={s.maxSkewMs} step={0.5} min={0.1} unit="ms"
            hint={win.ok ? samples(win.maxSkew) : undefined} onChange={(v) => set("maxSkewMs", v)} />
        </div>
        {!win.ok && <p className="error-text">{win.error}</p>}
        <p className="note">Max skew should exceed the longest pitch period in the speech (about 12.5 ms for an
          80 Hz voice). Overlap can be at most half the window.</p>
      </details>
    </aside>
  );
}
