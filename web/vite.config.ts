import react from "@vitejs/plugin-react";
import { defineConfig, searchForWorkspaceRoot } from "vite";
import { VitePWA } from "vite-plugin-pwa";

export default defineConfig({
  plugins: [
    react(),
    VitePWA({
      registerType: "prompt",
      injectRegister: false, // registered from UpdatePrompt via virtual:pwa-register/react
      includeAssets: ["favicon.ico", "icon.svg", "apple-touch-icon-180x180.png"],
      manifest: {
        name: "Spearcon Factory",
        short_name: "Spearcons",
        description: "Make spearcons in batches: speed speech up without changing its pitch, using SOLAFS.",
        theme_color: "#2a78d6",
        background_color: "#f9f9f7",
        display: "standalone",
        start_url: "/",
        icons: [
          { src: "pwa-64x64.png", sizes: "64x64", type: "image/png" },
          { src: "pwa-192x192.png", sizes: "192x192", type: "image/png" },
          { src: "pwa-512x512.png", sizes: "512x512", type: "image/png" },
          { src: "maskable-icon-512x512.png", sizes: "512x512", type: "image/png", purpose: "maskable" },
        ],
        // installed app: "Open with Spearcon Factory" for audio files (Chromium)
        file_handlers: [
          {
            action: "/",
            accept: { "audio/*": [".wav", ".mp3", ".m4a", ".aac", ".ogg", ".oga", ".opus", ".flac", ".webm"] },
          },
        ],
      },
      workbox: {
        // wav: the demo clips, so "Load demo" works offline too
        globPatterns: ["**/*.{js,css,html,wasm,svg,png,ico,webmanifest,wav}"],
        cleanupOutdatedCaches: true,
      },
    }),
  ],
  worker: { format: "es" },
  // the demo clips live in ../native/bin/audio; let the dev server serve them
  server: { fs: { allow: [searchForWorkspaceRoot(process.cwd()), "../native/bin/audio"] } },
});
