// Types for the Emscripten-generated loader (solafs.mjs), covering only what the app uses.
export interface SolafsModule {
  FS: {
    writeFile(path: string, data: Uint8Array): void;
    readFile(path: string): Uint8Array;
    unlink(path: string): void;
    analyzePath(path: string): { exists: boolean };
  };
  ccall(
    name: "solafs_file",
    returnType: "number",
    argTypes: ["string", "string", "number", "number", "number", "number", "number"],
    args: [string, string, number, number, number, number, number],
  ): number;
}

declare const createSolafsModule: (options?: { print?: (s: string) => void; printErr?: (s: string) => void }) => Promise<SolafsModule>;
export default createSolafsModule;
