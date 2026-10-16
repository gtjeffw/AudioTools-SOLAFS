// "Install app" support. Chromium browsers (Chrome, Edge, Opera, Samsung Internet; desktop and
// Android) fire `beforeinstallprompt` once the site is installable; holding on to that event lets the
// page open the browser's install dialog from its own button. Safari and Firefox never fire it, so
// the button simply never appears there (iOS installs via Share → Add to Home Screen instead).
//
// This module is imported before React renders, because the event can fire before the app mounts.
import { useSyncExternalStore } from "react";

interface BeforeInstallPromptEvent extends Event {
  prompt(): Promise<void>;
  userChoice: Promise<{ outcome: "accepted" | "dismissed" }>;
}

let deferred: BeforeInstallPromptEvent | null = null;
const listeners = new Set<() => void>();
const notify = () => listeners.forEach((l) => l());

/** True when running as the installed app rather than in a browser tab. */
export function isInstalledApp() {
  return (
    window.matchMedia("(display-mode: standalone)").matches ||
    window.matchMedia("(display-mode: window-controls-overlay)").matches ||
    (navigator as Navigator & { standalone?: boolean }).standalone === true
  );
}

window.addEventListener("beforeinstallprompt", (e) => {
  e.preventDefault(); // we show our own button instead of the browser's mini-infobar
  if (isInstalledApp()) return;
  deferred = e as BeforeInstallPromptEvent;
  notify();
});

window.addEventListener("appinstalled", () => {
  deferred = null;
  notify();
});

/** Opens the browser's install dialog. The event can only be used once, so the button hides afterwards. */
export async function promptInstall() {
  const e = deferred;
  if (!e) return;
  deferred = null;
  notify();
  await e.prompt();
  await e.userChoice;
}

/** Whether this browser has offered to install the app (and it isn't installed yet). */
export function useCanInstall(): boolean {
  return useSyncExternalStore(
    (l) => {
      listeners.add(l);
      return () => listeners.delete(l);
    },
    () => deferred !== null,
  );
}
