import type { ClipItem } from "../state/files";

interface Props {
  items: ClipItem[];
  playingId: number | null;
  onPlayInput: (it: ClipItem) => void;
  onPlayOutput: (it: ClipItem) => void;
  onDownload: (it: ClipItem) => void;
  onRemove: (it: ClipItem) => void;
}

const sec = (s?: number) => (s === undefined ? "–" : `${s.toFixed(2)} s`);

export function FileTable({ items, playingId, onPlayInput, onPlayOutput, onDownload, onRemove }: Props) {
  return (
    <div className="table-wrap">
      <table className="files">
        <thead>
          <tr>
            <th>File</th>
            <th className="num">Input</th>
            <th className="num">Spearcon</th>
            <th className="num">Speed</th>
            <th>Listen</th>
            <th aria-label="Actions" />
          </tr>
        </thead>
        <tbody>
          {items.map((it) => {
            const outSec = it.output && it.outputRate ? it.output.length / it.outputRate : undefined;
            return (
              <tr key={it.id} className={playingId === it.id ? "playing" : ""}>
                <td className="name">
                  <span title={it.name}>{it.name}</span>
                  {it.status === "error" && <span className="error-text">{it.error}</span>}
                  {(it.status === "decoding" || it.status === "queued" || it.status === "processing") && (
                    <span className="muted">{it.status === "decoding" ? "Decoding…" : "Processing…"}</span>
                  )}
                </td>
                <td className="num" title={it.input && it.decodedRate ? `Original: ${sec(it.input.length / it.decodedRate)}` : undefined}>
                  {sec(it.trimmedSeconds)}
                </td>
                <td className="num">{sec(outSec)}</td>
                <td className="num">{it.ratio ? `${it.ratio.toFixed(2)}×` : "–"}</td>
                <td className="listen">
                  <button type="button" className="btn-small" disabled={!it.input} onClick={() => onPlayInput(it)}>Original</button>
                  <button type="button" className="btn-small primary" disabled={it.status !== "done"} onClick={() => onPlayOutput(it)}>
                    Spearcon
                  </button>
                </td>
                <td className="actions">
                  <button type="button" className="btn-small" disabled={it.status !== "done"} onClick={() => onDownload(it)}
                    title="Download this spearcon as WAV">WAV</button>
                  <button type="button" className="btn-icon" onClick={() => onRemove(it)} aria-label={`Remove ${it.name}`} title="Remove">×</button>
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
