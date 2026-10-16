import { useRef, type ReactNode } from "react";
import { isInstalledApp, promptInstall, useCanInstall } from "../install";
import { guessPlatform, type Platform } from "../platform";

/**
 * "Install" where the browser lets the page start the install (Chromium), otherwise
 * "Installation" with instructions. Nothing when already running as the installed app.
 */
export function InstallButton() {
  const canInstall = useCanInstall();
  const dialog = useRef<HTMLDialogElement>(null);

  if (isInstalledApp()) return null;
  if (canInstall) {
    return (
      <button type="button" className="btn-small primary" onClick={() => void promptInstall()}
        title="Install Spearcon Factory as a Progressive Web App, for use offline and in its own window">
        Install
      </button>
    );
  }
  return (
    <>
      <button type="button" className="btn-small" onClick={() => dialog.current?.showModal()}>
        Installation
      </button>
      <InstallHelp dialogRef={dialog} />
    </>
  );
}

const STEPS: { platform: Platform; title: string; body: ReactNode }[] = [
  {
    platform: "ios",
    title: "iPhone and iPad",
    body: (
      <ol>
        <li>Select the <strong>Share</strong> button in Safari (or, on iOS 16.4 and later, in Chrome or Edge).</li>
        <li>Select <strong>Add to Home Screen</strong>, then <strong>Add</strong>.</li>
      </ol>
    ),
  },
  {
    platform: "mac-safari",
    title: "Safari on macOS",
    body: (
      <p>Select <strong>File → Add to Dock</strong>. This requires Safari 17 or later (macOS 14 Sonoma or later).</p>
    ),
  },
  {
    platform: "android",
    title: "Android",
    body: (
      <p>In Chrome, open the <strong>⋮</strong> menu and select <strong>Install app</strong> (in some versions,
        <strong> Add to Home screen</strong>). Samsung Internet and Firefox provide the same command in their menus.</p>
    ),
  },
  {
    platform: "desktop",
    title: "Chrome or Edge on a desktop computer",
    body: (
      <p>Select the <strong>install</strong> icon at the right of the address bar, or the <strong>Install</strong> command
        in the browser menu. If neither is present, the application may already be installed.</p>
    ),
  },
  {
    platform: "firefox-desktop",
    title: "Firefox on a desktop computer",
    body: (
      <p>Firefox does not support the installation of Progressive Web Apps. The application can be installed from
        Chrome, Edge or Safari. In Firefox it remains fully usable in a browser tab, including offline after the
        first visit.</p>
    ),
  },
];

function InstallHelp({ dialogRef }: { dialogRef: React.RefObject<HTMLDialogElement | null> }) {
  const platform = guessPlatform();
  const mine = STEPS.find((s) => s.platform === platform)!;
  const others = STEPS.filter((s) => s !== mine);
  return (
    <dialog ref={dialogRef} className="dialog" aria-labelledby="install-title"
      onClick={(e) => e.target === e.currentTarget && e.currentTarget.close()}>
      <div className="dialog-body">
        <h2 id="install-title">Installing Spearcon Factory</h2>
        <p className="dialog-lead">Spearcon Factory is a Progressive Web App (PWA): a web application that can be
          installed on a computer, phone or tablet. Once installed, it runs in its own window and remains available
          without a network connection. Audio is processed locally on the device in either case.</p>
        <h3>{mine.title}</h3>
        {mine.body}
        <details>
          <summary>Other platforms</summary>
          {others.map((s) => (
            <section key={s.platform}>
              <h3>{s.title}</h3>
              {s.body}
            </section>
          ))}
        </details>
        <form method="dialog" className="dialog-actions">
          <button type="submit">Close</button>
        </form>
      </div>
    </dialog>
  );
}
