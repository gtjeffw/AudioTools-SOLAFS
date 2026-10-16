import { useRegisterSW } from "virtual:pwa-register/react";

/** Offline-ready notice and "new version" prompt for the service worker. */
export function UpdatePrompt() {
  const {
    offlineReady: [offlineReady, setOfflineReady],
    needRefresh: [needRefresh, setNeedRefresh],
    updateServiceWorker,
  } = useRegisterSW();

  if (!offlineReady && !needRefresh) return null;
  return (
    <div className="toast" role="status">
      <span>{needRefresh ? "A new version is available." : "Ready to work offline."}</span>
      {needRefresh && (
        <button type="button" className="primary btn-small" onClick={() => updateServiceWorker(true)}>
          Reload
        </button>
      )}
      <button type="button" className="btn-small" onClick={() => {
        setOfflineReady(false);
        setNeedRefresh(false);
      }}>
        Close
      </button>
    </div>
  );
}
