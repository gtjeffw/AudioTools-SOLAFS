// Generates the PWA icons in public/ from public/icon.svg: npm run generate-pwa-assets
import { defineConfig, minimal2023Preset } from "@vite-pwa/assets-generator/config";

export default defineConfig({
  preset: {
    ...minimal2023Preset,
    maskable: { ...minimal2023Preset.maskable, resizeOptions: { background: "#2a78d6" } },
    apple: { ...minimal2023Preset.apple, resizeOptions: { background: "#2a78d6" } },
  },
  images: ["public/icon.svg"],
});
