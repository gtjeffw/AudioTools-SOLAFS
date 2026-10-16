import { useRegisterSW } from "virtual:pwa-register/react";

/**
 * "New version" prompt for the service worker. Shown after a deploy, so an open or installed copy
 * doesn't keep running the old version. (The plugin's "ready to work offline" notice isn't shown.)
 */
export function UpdatePrompt() {
  const {
    needRefresh: [needRefresh, setNeedRefresh],
    updateServiceWorker,
  } = useRegisterSW();

  if (!needRefresh) return null;
  return (
    <div className="toast" role="status">
      <span>A new version is available.</span>
      <button type="button" className="primary btn-small" onClick={() => updateServiceWorker(true)}>
        Reload
      </button>
      <button type="button" className="btn-small" onClick={() => setNeedRefresh(false)}>
        Later
      </button>
    </div>
  );
}
