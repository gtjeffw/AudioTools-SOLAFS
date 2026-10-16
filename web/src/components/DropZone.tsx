import { useRef, useState } from "react";

const ACCEPT = "audio/*,.wav,.mp3,.m4a,.aac,.ogg,.oga,.opus,.flac,.webm";

export function isAudioFile(f: File) {
  return f.type.startsWith("audio/") || /\.(wav|mp3|m4a|aac|ogg|oga|opus|flac|webm)$/i.test(f.name);
}

interface Props {
  onFiles: (files: File[]) => void;
  onDemo: () => void;
  compact: boolean;
}

export function DropZone({ onFiles, onDemo, compact }: Props) {
  const input = useRef<HTMLInputElement>(null);
  const [over, setOver] = useState(false);
  const take = (list: FileList | null) => {
    const files = Array.from(list ?? []).filter(isAudioFile);
    if (files.length) onFiles(files);
  };
  return (
    <div
      className={`dropzone${over ? " over" : ""}${compact ? " compact" : ""}`}
      onDragOver={(e) => {
        e.preventDefault();
        setOver(true);
      }}
      onDragLeave={() => setOver(false)}
      onDrop={(e) => {
        e.preventDefault();
        setOver(false);
        take(e.dataTransfer.files);
      }}
    >
      <input ref={input} type="file" accept={ACCEPT} multiple hidden onChange={(e) => {
        take(e.target.files);
        e.target.value = "";
      }} />
      <p>
        <strong>Drop audio files here</strong> or{" "}
        <button type="button" className="link" onClick={() => input.current?.click()}>choose files</button>
        <button type="button" className="btn-small demo-btn" onClick={onDemo}
          title="Adds four sample names and sets the length to 0.2 s">Load demo</button>
      </p>
      {!compact && <p className="note">WAV, MP3, M4A, OGG, FLAC… anything your browser can play. Files stay on this device.</p>}
    </div>
  );
}
