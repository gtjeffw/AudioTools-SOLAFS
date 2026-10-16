import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import App from "./App";
import "./install"; // listens for the browser's install offer before the app mounts
import "./styles.css";

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
