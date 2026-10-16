import { zipSync } from "fflate";

/** `base` + `suffix` + `.wav`, with " (2)", " (3)"… added to repeats. */
export function outputNames(bases: string[], suffix: string): string[] {
  const used = new Set<string>();
  return bases.map((b) => {
    const clean = b.replace(/[\\/:*?"<>|]/g, "_") || "clip";
    let name = `${clean}${suffix}.wav`;
    for (let n = 2; used.has(name.toLowerCase()); n++) name = `${clean}${suffix} (${n}).wav`;
    used.add(name.toLowerCase());
    return name;
  });
}

export function baseName(fileName: string): string {
  return fileName.replace(/\.[^.]+$/, "");
}

/** ZIP of the WAVs (stored, not compressed: PCM barely compresses) plus a JSON manifest. */
export function buildZip(files: { name: string; bytes: Uint8Array }[], manifest: unknown): Uint8Array {
  const entries: Record<string, [Uint8Array, { level: 0 }]> = {};
  for (const f of files) entries[f.name] = [f.bytes, { level: 0 }];
  entries["spearcon-settings.json"] = [new TextEncoder().encode(JSON.stringify(manifest, null, 2)), { level: 0 }];
  return zipSync(entries);
}

export function download(bytes: Uint8Array | Blob, name: string, type: string) {
  const blob = bytes instanceof Blob ? bytes : new Blob([bytes as Uint8Array<ArrayBuffer>], { type });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = name;
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 10_000);
}
