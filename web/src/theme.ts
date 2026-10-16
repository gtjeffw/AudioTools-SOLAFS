// Light / dark / follow the system. Stored in localStorage and applied as data-theme on <html>
// (styles.css). index.html applies the saved choice before the app loads, to avoid a flash.

export type Theme = "system" | "light" | "dark";

export const THEME_KEY = "spearcon-factory.theme";

export function loadTheme(): Theme {
  try {
    const t = localStorage.getItem(THEME_KEY);
    if (t === "light" || t === "dark") return t;
  } catch {
    /* storage disabled */
  }
  return "system";
}

export function applyTheme(t: Theme) {
  if (t === "system") document.documentElement.removeAttribute("data-theme");
  else document.documentElement.setAttribute("data-theme", t);
  try {
    if (t === "system") localStorage.removeItem(THEME_KEY);
    else localStorage.setItem(THEME_KEY, t);
  } catch {
    /* storage disabled */
  }
}
