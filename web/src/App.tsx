import { useCallback, useEffect, useMemo, useReducer, useRef, useState } from "react";
import { decodeToMono } from "./audio/decode";
import { Player } from "./audio/player";
import { encodeWav16 } from "./audio/wav";
import { DropZone, isAudioFile } from "./components/DropZone";
import { FileTable } from "./components/FileTable";
import { InstallButton } from "./components/InstallButton";
import { NumberField } from "./components/NumberField";
import { SettingsPanel } from "./components/SettingsPanel";
import { ThemeSelect } from "./components/ThemeSelect";
import { UpdatePrompt } from "./components/UpdatePrompt";
import { DEMO_TARGET_SEC, loadDemoFiles } from "./demo";
import logoDark from "./assets/logo-dark.png";
import logoLight from "./assets/logo-light.png";
import { makeSpearcon } from "./engine/pipeline";
import { CancelledError, EnginePool } from "./engine/pool";
import { baseName, buildZip, download, outputNames } from "./export/zip";
import { filesReducer, type ClipItem } from "./state/files";
import { loadSettings, saveSettings, type Settings } from "./state/settings";

let nextId = 1;

export default function App() {
  const [settings, setSettings] = useState<Settings>(loadSettings);
  const [items, dispatch] = useReducer(filesReducer, []);
  const [playingId, setPlayingId] = useState<number | null>(null);
  const [playingAll, setPlayingAll] = useState(false);

  const pool = useMemo(() => new EnginePool(), []);
  const player = useMemo(() => new Player(), []);
  // Decoded input per clip, keyed by sample rate, so changing other settings doesn't re-decode.
  const decoded = useRef(new Map<number, { rate: number; samples: Float32Array }>());
  const itemsRef = useRef(items);
  itemsRef.current = items;
  const generation = useRef(0);

  useEffect(() => saveSettings(settings), [settings]);

  const processOne = useCallback(
    async (item: ClipItem, s: Settings, gen: number) => {
      const cancelled = () => gen !== generation.current || !itemsRef.current.some((i) => i.id === item.id);
      const update = (patch: Partial<ClipItem>) => !cancelled() && dispatch({ type: "update", id: item.id, patch });
      try {
        let dec = decoded.current.get(item.id);
        if (!dec || dec.rate !== s.sampleRate) {
          update({ status: "decoding", error: undefined });
          const samples = await decodeToMono(await item.file.arrayBuffer(), s.sampleRate);
          dec = { rate: s.sampleRate, samples };
          decoded.current.set(item.id, dec);
        }
        update({ status: "processing", input: dec.samples, decodedRate: dec.rate, error: undefined });
        const res = await makeSpearcon(pool, dec.samples, s, cancelled);
        if (res.ok) {
          update({ status: "done", output: res.output, outputRate: s.sampleRate, ratio: res.ratio, trimmedSeconds: res.trimmedSeconds });
        } else {
          update({ status: "error", error: res.error, output: undefined, ratio: undefined, trimmedSeconds: res.trimmedSeconds });
        }
      } catch (err) {
        if (err instanceof CancelledError) return;
        const msg = err instanceof Error ? err.message : String(err);
        update({ status: "error", error: /decod/i.test(msg) ? "This file could not be decoded" : msg, output: undefined });
      }
    },
    [pool],
  );

  // Re-process everything shortly after the settings stop changing. The playback gap and the
  // file name suffix don't affect the audio, so they're left out of the key.
  const settingsRef = useRef(settings);
  settingsRef.current = settings;
  const audioKey = JSON.stringify({ ...settings, playGapMs: 0, suffix: "" });
  useEffect(() => {
    const t = setTimeout(() => {
      const gen = ++generation.current;
      for (const it of itemsRef.current) void processOne(it, settingsRef.current, gen);
    }, 250);
    return () => clearTimeout(t);
  }, [audioKey, processOne]);

  const addFiles = useCallback(
    (files: File[], s: Settings = settingsRef.current) => {
      const added: ClipItem[] = files.map((file) => ({ id: nextId++, file, name: file.name, status: "decoding" }));
      dispatch({ type: "add", items: added });
      itemsRef.current = [...itemsRef.current, ...added];
      for (const it of added) void processOne(it, s, generation.current);
    },
    [processOne],
  );

  // Demo: four test recordings, made into 0.2 s spearcons.
  const [demoError, setDemoError] = useState<string | null>(null);
  const loadDemo = useCallback(async () => {
    setDemoError(null);
    try {
      const have = new Set(itemsRef.current.map((it) => it.name));
      const files = (await loadDemoFiles()).filter((f) => !have.has(f.name)); // don't add them twice
      const s: Settings = { ...settingsRef.current, lengthMode: "length", targetSec: DEMO_TARGET_SEC };
      settingsRef.current = s;
      setSettings(s);
      if (files.length) addFiles(files, s);
    } catch (err) {
      setDemoError(err instanceof Error ? err.message : String(err));
    }
  }, [addFiles]);

  // Installed PWA: audio files opened with the app (manifest file_handlers).
  useEffect(() => {
    const lq = (window as unknown as { launchQueue?: { setConsumer(cb: (p: { files: FileSystemFileHandle[] }) => void): void } }).launchQueue;
    lq?.setConsumer(async ({ files }) => {
      const list = await Promise.all(files.map((h) => h.getFile()));
      addFiles(list.filter(isAudioFile));
    });
  }, [addFiles]);

  const done = items.filter((i) => i.status === "done" && i.output);
  const busy = items.some((i) => i.status === "decoding" || i.status === "queued" || i.status === "processing");

  const stop = () => {
    player.stop();
    setPlayingId(null);
    setPlayingAll(false);
  };
  const play = (it: ClipItem, which: "input" | "output") => {
    const samples = which === "input" ? it.input : it.output;
    const rate = which === "input" ? it.decodedRate : it.outputRate;
    if (!samples || !rate) return;
    setPlayingAll(false);
    setPlayingId(it.id);
    player.play({ samples, sampleRate: rate }, () => setPlayingId(null));
  };
  const playAll = () => {
    if (!done.length) return;
    setPlayingAll(true);
    player.playSequence(
      done.map((it) => ({ samples: it.output!, sampleRate: it.outputRate! })),
      settings.playGapMs,
      (i) => setPlayingId(done[i].id),
      () => {
        setPlayingId(null);
        setPlayingAll(false);
      },
    );
  };

  const exportZip = () => {
    const names = outputNames(done.map((it) => baseName(it.name)), settings.suffix);
    const files = done.map((it, i) => ({ name: names[i], bytes: encodeWav16(it.output!, it.outputRate!) }));
    const manifest = {
      generator: "Spearcon Factory (SOLAFS, Hejna & Musicus 1991)",
      created: new Date().toISOString(),
      settings,
      clips: done.map((it, i) => ({
        source: it.name,
        output: names[i],
        inputSeconds: it.trimmedSeconds,
        outputSeconds: it.output!.length / it.outputRate!,
        speedRatio: it.ratio,
      })),
    };
    download(buildZip(files, manifest), "spearcons.zip", "application/zip");
  };

  const downloadOne = (it: ClipItem) => {
    const [name] = outputNames([baseName(it.name)], settings.suffix);
    download(encodeWav16(it.output!, it.outputRate!), name, "audio/wav");
  };

  const remove = (it: ClipItem) => {
    if (playingId === it.id) stop();
    decoded.current.delete(it.id);
    dispatch({ type: "remove", id: it.id });
  };

  return (
    <div className="app">
      <header className="top">
        <div className="brand">
          {/* two versions: the dark one has a lighter outline so it shows on the dark page */}
          <img className="logo-light" src={logoLight} alt="" width={201} height={128} />
          <img className="logo-dark" src={logoDark} alt="" width={201} height={128} />
          <div>
            <h1>
              <span className="title-1">Spearcon</span> <span className="title-2">Factory</span>
            </h1>
            <p className="tagline">Batch generation of spearcons: speech compressed in time with the SOLAFS algorithm, which preserves pitch.</p>
            <p className="privacy">All processing takes place locally in the browser; audio files are not uploaded.</p>
          </div>
        </div>
        <div className="top-actions">
          <InstallButton />
          <ThemeSelect />
        </div>
      </header>

      <main className="layout">
        <DropZone onFiles={(files) => addFiles(files)} onDemo={loadDemo} compact={items.length > 0} />
        {demoError && <p className="error-text">{demoError}</p>}

        <SettingsPanel settings={settings} onChange={setSettings} />

        <section className="work" aria-label="Files">
          {items.length > 0 && (
            <>
              <div className="toolbar">
                {playingAll ? (
                  <button type="button" onClick={stop}>Stop</button>
                ) : (
                  <button type="button" onClick={playAll} disabled={!done.length}>Play all</button>
                )}
                <NumberField label="Interval" value={settings.playGapMs} step={50} min={0} unit="ms"
                  onChange={(v) => setSettings({ ...settings, playGapMs: v })} />
                <span className="spacer" />
                <span className="muted">{busy ? "Processing…" : `${done.length} of ${items.length} ready`}</span>
                <button type="button" className="primary" onClick={exportZip} disabled={!done.length || busy}>
                  Export ZIP
                </button>
                <button type="button" onClick={() => { stop(); decoded.current.clear(); dispatch({ type: "clear" }); }}>
                  Clear list
                </button>
              </div>
              <FileTable
                items={items}
                playingId={playingId}
                onPlayInput={(it) => play(it, "input")}
                onPlayOutput={(it) => play(it, "output")}
                onDownload={downloadOne}
                onRemove={remove}
              />
            </>
          )}
        </section>
      </main>

      <footer className="foot">
        <p>Based on SOLAFS time-scale modification (Hejna &amp; Musicus, 1991).</p>
        <p>© 2009–2026 Georgia Institute of Technology, Jeff Wilson. Released under the MIT License.</p>
      </footer>
      <UpdatePrompt />
    </div>
  );
}
