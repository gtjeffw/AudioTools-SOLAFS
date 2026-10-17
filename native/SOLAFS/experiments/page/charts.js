// Shared helpers for the experiment pages: elements, legend, tooltip, line chart, table.
function el(tag, attrs, text) {
  const e = document.createElement(tag);
  for (const k in (attrs || {})) e.setAttribute(k, attrs[k]);
  if (text !== undefined) e.textContent = text;
  return e;
}
const SVGNS = "http://www.w3.org/2000/svg";
function sv(tag, attrs) { const e = document.createElementNS(SVGNS, tag); for (const k in attrs) e.setAttribute(k, attrs[k]); return e; }
function fmt(x, d) { return x === null || x === undefined ? "–" : x.toFixed(d); }

function legend(id, items, kind) {
  const L = document.getElementById(id);
  for (const [name, color] of items) {
    const s = el("span");
    const k = el("i", {class: kind === "line" ? "key-line" : "key-rect"});
    k.style.background = `var(${color})`;
    s.append(k, document.createTextNode(name));
    L.append(s);
  }
}
function tooltip(container) {
  const t = el("div", {class: "tip"});
  container.append(t);
  return t;
}
function showTip(tip, container, x, y, title, rows) {
  tip.replaceChildren();
  tip.append(el("div", {class: "t"}, title));
  for (const [val, name, color] of rows) {
    const r = el("div");
    const k = el("i", {class: "key-line"});
    k.style.background = `var(${color})`;
    k.style.marginRight = "6px";
    k.style.verticalAlign = "middle";
    r.append(k, el("span", {class: "v"}, val), document.createTextNode(" "), el("span", {class: "n"}, name));
    tip.append(r);
  }
  tip.style.display = "block";
  const cw = container.clientWidth;
  const tw = tip.offsetWidth;
  // right of the pointer, or flipped to its left near the right edge so it doesn't cover the marks
  tip.style.left = Math.max(0, x + 12 + tw <= cw ? x + 12 : x - 12 - tw) + "px";
  tip.style.top = Math.max(0, y - 10) + "px";
}

function lineChart(id, title, xLabels, series, yMin, yMax, yTicks, dec, tipDec, unit) {
  const c = document.getElementById(id);
  const W = 460, H = 260, ml = 48, mr = 16, mt = 26, mb = 34;
  const svg = sv("svg", {viewBox: `0 0 ${W} ${H}`, role: "img", "aria-label": title});
  const t = sv("text", {x: 0, y: 14, fill: "var(--text-primary)", "font-size": 13, "font-weight": 600});
  t.textContent = title; svg.append(t);
  const X = i => ml + i * (W - ml - mr) / (xLabels.length - 1);
  const Y = v => mt + (yMax - v) / (yMax - yMin) * (H - mt - mb);
  for (const v of yTicks) {
    svg.append(sv("line", {x1: ml, x2: W - mr, y1: Y(v), y2: Y(v), stroke: "var(--grid)", "stroke-width": 1}));
    const tx = sv("text", {x: ml - 6, y: Y(v) + 4, "text-anchor": "end", fill: "var(--text-muted)", "font-size": 11});
    tx.textContent = v.toFixed(dec); svg.append(tx);
  }
  xLabels.forEach((lab, i) => {
    const tx = sv("text", {x: X(i), y: H - mb + 16, "text-anchor": "middle", fill: "var(--text-muted)", "font-size": 11});
    tx.textContent = lab; svg.append(tx);
  });
  const xt = sv("text", {x: (ml + W - mr) / 2, y: H - 4, "text-anchor": "middle", fill: "var(--text-muted)", "font-size": 11});
  xt.textContent = "speed ratio (input ÷ output duration)"; svg.append(xt);
  const cross = sv("line", {y1: mt, y2: H - mb, stroke: "var(--axis)", "stroke-width": 1, visibility: "hidden"});
  svg.append(cross);
  for (const s of series) {
    const d = s.values.map((v, i) => `${i ? "L" : "M"}${X(i)},${Y(v)}`).join("");
    svg.append(sv("path", {d, fill: "none", stroke: `var(${s.color})`, "stroke-width": 2, "stroke-linejoin": "round", "stroke-linecap": "round"}));
    const li = s.values.length - 1;
    svg.append(sv("circle", {cx: X(li), cy: Y(s.values[li]), r: 4, fill: `var(${s.color})`, stroke: "var(--surface-1)", "stroke-width": 2}));
  }
  const dots = series.map(s => { const d = sv("circle", {r: 4, fill: `var(${s.color})`, stroke: "var(--surface-1)", "stroke-width": 2, visibility: "hidden"}); svg.append(d); return d; });
  const hit = sv("rect", {x: ml, y: mt, width: W - ml - mr, height: H - mt - mb, fill: "transparent", tabindex: 0});
  svg.append(hit);
  c.append(svg);
  const tip = tooltip(c);
  function at(i) {
    cross.setAttribute("x1", X(i)); cross.setAttribute("x2", X(i)); cross.setAttribute("visibility", "visible");
    series.forEach((s, k) => { dots[k].setAttribute("cx", X(i)); dots[k].setAttribute("cy", Y(s.values[i])); dots[k].setAttribute("visibility", "visible"); });
    const scale = c.clientWidth / W;
    showTip(tip, c, X(i) * scale, Y(Math.max(...series.map(s => s.values[i]))) * scale,
      `${xLabels[i]} speed`, series.map(s => [s.values[i].toFixed(tipDec) + (unit || ''), s.name, s.color]));
  }
  hit.addEventListener("pointermove", e => {
    const r = svg.getBoundingClientRect();
    const x = (e.clientX - r.left) / r.width * W;
    let best = 0;
    xLabels.forEach((_, i) => { if (Math.abs(X(i) - x) < Math.abs(X(best) - x)) best = i; });
    at(best);
  });
  let fi = 0;
  hit.addEventListener("focus", () => at(fi));
  hit.addEventListener("keydown", e => { if (e.key === "ArrowRight") at(fi = Math.min(fi + 1, xLabels.length - 1)); if (e.key === "ArrowLeft") at(fi = Math.max(fi - 1, 0)); });
  const hide = () => { tip.style.display = "none"; cross.setAttribute("visibility", "hidden"); dots.forEach(d => d.setAttribute("visibility", "hidden")); };
  hit.addEventListener("pointerleave", hide); hit.addEventListener("blur", hide);
}

function table(id, head, rows) {
  const tb = el("table");
  const tr = el("tr");
  for (const h of head) tr.append(el("th", {}, h));
  tb.append(tr);
  for (const r of rows) { const x = el("tr"); for (const v of r) x.append(el("td", {}, v)); tb.append(x); }
  const wrap = el("div");
  wrap.style.overflowX = "auto";
  wrap.append(tb);
  document.getElementById(id).append(wrap);
}

