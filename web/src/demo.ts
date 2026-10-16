// Demo clips, bundled from the repo's test audio (native/bin/audio) by Vite as hashed assets.
// Each URL must be a literal for Vite to pick it up.
const DEMO_URLS: [string, string][] = [
  ["allegra_sidner.wav", new URL("../../native/bin/audio/allegra_sidner.wav", import.meta.url).href],
  ["anne_drum.wav", new URL("../../native/bin/audio/anne_drum.wav", import.meta.url).href],
  ["bruce_walker.wav", new URL("../../native/bin/audio/bruce_walker.wav", import.meta.url).href],
  ["jeff_wilson.wav", new URL("../../native/bin/audio/jeff_wilson.wav", import.meta.url).href],
];

/** Demo spearcons are made at a fixed length. */
export const DEMO_TARGET_SEC = 0.2;

export async function loadDemoFiles(): Promise<File[]> {
  return Promise.all(
    DEMO_URLS.map(async ([name, url]) => {
      const res = await fetch(url);
      if (!res.ok) throw new Error(`Couldn't load demo clip ${name}`);
      return new File([await res.blob()], name, { type: "audio/wav" });
    }),
  );
}
