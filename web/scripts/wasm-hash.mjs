// Hash of everything that goes into the WebAssembly engine build.
//
//   node scripts/wasm-hash.mjs          print the hash of the current sources
//   node scripts/wasm-hash.mjs --check  exit 1 if the committed engine was built from different sources
//
// web/wasm/build.sh writes the hash next to the build output, so a stale committed engine is caught.
// Line endings are normalized first, so a CRLF/LF checkout difference doesn't count as a change.
import { createHash } from "node:crypto";
import { readFileSync, existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const WEB = join(dirname(fileURLToPath(import.meta.url)), "..");
const REPO = join(WEB, "..");

export const SOURCES = [
  "native/WAVIO/src/WavIO/WavIO.cpp",
  "native/WAVIO/src/WavIO/WavIO.hpp",
  "native/WAVIO/src/WavIO/FileIO.cpp",
  "native/WAVIO/src/WavIO/FileIO.hpp",
  "native/WAVIO/src/Common/CommonTypes.hpp",
  "native/WAVIO/src/Common/System.hpp",
  "native/WAVIO/src/Common/HelpDebug.cpp",
  "native/WAVIO/src/Common/HelpDebug.hpp",
  "native/WAVIO/src/Common/HelperTools.hpp",
  "native/CIRCULAR_BUFFER/src/CircFBuf/CircFBuf.cpp",
  "native/CIRCULAR_BUFFER/src/CircFBuf/CircFBuf.hpp",
  "native/SOLAFS/src/SOLAFS/SOLAFS.cpp",
  "native/SOLAFS/src/SOLAFS/SOLAFS.hpp",
  "web/wasm/solafs_wasm.cpp",
  "web/wasm/compat/tchar.h",
  "web/wasm/build.sh",
];

export const HASH_FILE = join(WEB, "src/engine/generated/solafs.build-hash");

export function sourceHash() {
  const h = createHash("sha256");
  for (const rel of SOURCES) {
    h.update(rel + "\0");
    h.update(readFileSync(join(REPO, rel), "utf8").replace(/\r\n/g, "\n"));
    h.update("\0");
  }
  return h.digest("hex");
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const current = sourceHash();
  if (process.argv.includes("--check")) {
    const built = existsSync(HASH_FILE) ? readFileSync(HASH_FILE, "utf8").trim() : "(none)";
    if (built !== current) {
      console.error(
        `The committed WebAssembly engine is out of date with the C++ sources.\n` +
          `  built from: ${built}\n  sources:    ${current}\n` +
          `Rebuild it with: npm run build:wasm (needs Docker)`,
      );
      process.exit(1);
    }
    console.log("WebAssembly engine is up to date.");
  } else {
    console.log(current);
  }
}
