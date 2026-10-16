import { useState } from "react";
import { applyTheme, loadTheme, type Theme } from "../theme";

export function ThemeSelect() {
  const [theme, setTheme] = useState<Theme>(loadTheme);
  return (
    <label className="theme-select">
      <span>Theme</span>
      <select
        value={theme}
        onChange={(e) => {
          const t = e.target.value as Theme;
          setTheme(t);
          applyTheme(t);
        }}
      >
        <option value="system">System</option>
        <option value="light">Light</option>
        <option value="dark">Dark</option>
      </select>
    </label>
  );
}
