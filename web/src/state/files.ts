// The list of clips and what has happened to each one.

export type Status = "decoding" | "queued" | "processing" | "done" | "error";

export interface ClipItem {
  id: number;
  file: File;
  name: string;
  status: Status;
  error?: string;
  /** Decoded mono input at `decodedRate`. */
  input?: Float32Array;
  decodedRate?: number;
  /** After trimming, before SOLAFS. */
  trimmedSeconds?: number;
  ratio?: number;
  output?: Float32Array;
  outputRate?: number;
}

export type Action =
  | { type: "add"; items: ClipItem[] }
  | { type: "update"; id: number; patch: Partial<ClipItem> }
  | { type: "remove"; id: number }
  | { type: "clear" };

export function filesReducer(state: ClipItem[], a: Action): ClipItem[] {
  switch (a.type) {
    case "add":
      return [...state, ...a.items];
    case "update":
      return state.map((it) => (it.id === a.id ? { ...it, ...a.patch } : it));
    case "remove":
      return state.filter((it) => it.id !== a.id);
    case "clear":
      return [];
  }
}
