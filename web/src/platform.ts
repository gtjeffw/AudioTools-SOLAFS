// Which install instructions to show first (see components/InstallButton.tsx).

export type Platform = "ios" | "android" | "mac-safari" | "firefox-desktop" | "desktop";

/**
 * Best guess at the platform, only to pick which install instructions to show first.
 * iPadOS reports itself as a Mac, so a Mac with a touchscreen counts as iOS.
 */
export function guessPlatform(ua = navigator.userAgent, touchPoints = navigator.maxTouchPoints): Platform {
  if (/iPhone|iPad|iPod/.test(ua) || (/Macintosh/.test(ua) && touchPoints > 1)) return "ios";
  if (/Android/.test(ua)) return "android";
  if (/Firefox\//.test(ua)) return "firefox-desktop";
  if (/Macintosh/.test(ua) && /Safari\//.test(ua) && !/Chrome\/|Chromium\/|Edg\//.test(ua)) return "mac-safari";
  return "desktop";
}
