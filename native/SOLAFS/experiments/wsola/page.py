"""SOLAFS vs WSOLA listening and measurement page: out/wsola/page/index.html, with audio copied alongside.
Run render.py and metrics.py first."""
import csv, json, random, shutil, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, input_path, page_assets

RENDERS = OUT / "wsola" / "renders"
DEST = OUT / "wsola" / "page"
VARIANTS = ["A-solafs", "B-hann-short", "C-linear-long", "D-wsola"]
LABEL = {
    "A-solafs": "A: SOLAFS (linear fade, 6.8 ms window)",
    "B-hann-short": "B: Hann fade, 6.8 ms window",
    "C-linear-long": "C: linear fade, 25 ms window",
    "D-wsola": "D: WSOLA (Hann fade, 25 ms window)",
}
SHORT = {"A-solafs": "A SOLAFS", "B-hann-short": "B Hann 6.8 ms", "C-linear-long": "C linear 25 ms", "D-wsola": "D WSOLA"}
COLORS = {"A-solafs": "--series-1", "B-hann-short": "--series-2", "C-linear-long": "--series-3", "D-wsola": "--series-4"}
PAIR = ("A-solafs", "D-wsola")
EXAMPLES = [
    ("bruce_walker", 5, "A short name at spearcon-level compression."),
    ("jeff_wilson", 8, "Heavy compression."),
    ("natures_miracle_med", 3, "A longer phrase at 3x."),
    ("allegra_sidner", 2, "16 kHz speech at 2x."),
    ("natures_miracle_med", 1.25, "Mild speed-up, where the longer WSOLA window measured better."),
    ("abc", 1.25, "Letters of the alphabet, mild speed-up."),
    ("natures_miracle_med", 0.5, "Slowed to half speed."),
    ("glide_tone", 3, "Synthetic gliding tone."),
]



def main():
    rows = list(csv.DictReader(open(OUT / "wsola" / "results.csv")))
    for r in rows:
        for k in ("ratio", "lsd_db", "clicks_ps", "periodicity", "level_db"):
            r[k] = None if r[k] in ("", "nan") else float(r[k])
    idx = {(r["input"], r["ratio"], r["variant"]): r for r in rows}
    ratios = sorted({r["ratio"] for r in rows})
    speech = [r for r in rows if r["input"] != "glide_tone"]
    mean = lambda xs: sum(xs) / len(xs)
    by = {m: {v: [mean([r[m] for r in speech if r["ratio"] == q and r["variant"] == v]) for q in ratios] for v in VARIANTS}
          for m in ("lsd_db", "level_db", "clicks_ps")}
    tone = {v: [idx[("glide_tone", q, v)]["periodicity"] for q in ratios] for v in VARIANTS}

    if DEST.exists():
        shutil.rmtree(DEST)
    (DEST / "audio").mkdir(parents=True)
    rng = random.Random(2026)
    examples = []
    for n, (inp, q, why) in enumerate(EXAMPLES, 1):
        files = {}
        for v in VARIANTS:
            dst = DEST / "audio" / inp / f"{q:g}" / f"{v}.wav"
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(RENDERS / inp / f"{q:g}" / f"{v}.wav", dst)
            files[v] = str(dst.relative_to(DEST))
        src = DEST / "audio" / "inputs" / f"{inp}.wav"
        src.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(input_path(inp), src)
        a_first = rng.random() < 0.5
        examples.append({"id": n, "input": inp, "ratio": q, "why": why, "original": str(src.relative_to(DEST)),
                         "files": files, "A": PAIR[0] if a_first else PAIR[1], "B": PAIR[1] if a_first else PAIR[0],
                         "metrics": {v: {k: idx[(inp, q, v)][k] for k in ("lsd_db", "clicks_ps", "periodicity", "level_db")} for v in VARIANTS}})
    shutil.copyfile(OUT / "wsola" / "results.csv", DEST / "results.csv")

    data = {"variants": VARIANTS, "label": LABEL, "short": SHORT, "colors": COLORS, "pair": PAIR, "ratios": ratios,
            "examples": examples, "by": by, "tone": tone}
    css, js = page_assets()
    html = PAGE.replace("/*CSS*/", "<style>\n" + css + "</style>").replace("/*DATA*/null", json.dumps(data)) \
               .replace("/*HELPERS*/", js)
    (DEST / "index.html").write_text(html)
    print("wrote", DEST / "index.html")


PAGE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>SOLAFS vs WSOLA: listening test and measurements</title>
/*CSS*/
</head><body><div class="viz-root"><main>
<h1>SOLAFS vs WSOLA</h1>
<p class="lead">WSOLA was added to the existing SOLAFS code as an option: the same algorithm with a raised-cosine (Hann)
cross-fade instead of a linear one, and, in its classic form, longer windows. This page compares the two by ear and by
measurement.</p>

<div class="card">
<h3>The four variants</h3>
<ul>
<li><strong>A: SOLAFS</strong> as used by spearcon.exe and the app: linear cross-fade; window 6.8 ms, overlap 3.4 ms.</li>
<li><strong>B</strong>: as A, with a Hann cross-fade (isolates the fade shape).</li>
<li><strong>C</strong>: as A, with a 25 ms window and 12.5 ms overlap (isolates the window length).</li>
<li><strong>D: WSOLA</strong> in its classic form: Hann cross-fade, 25 ms window, 12.5 ms overlap.</li>
</ul>
<p class="meta">All use the same similarity search (maximum skew 13.6 ms), fixed speed ratios and no trimming or fades,
rendered with the native SOLAFS code by build/solafs_exp (bit-identical to the web app's engine).</p>
</div>

<h2>Listening test</h2>
<p>Each example compares <strong>A</strong> and <strong>B</strong>: one is SOLAFS (variant A) and the other WSOLA
(variant D), assigned at random per example. Choose the one that sounds better, or "no difference", then reveal. Choices
are stored in this browser only. All four variants are available under each example.</p>
<div class="row" style="margin: 8px 0 4px">
  <button id="revealAll">Reveal all</button>
  <button id="clearVotes">Clear choices</button>
  <span class="score" id="score"></span>
</div>
<div id="examples"></div>

<h2>Measurements</h2>
<p>7 inputs (6 speech recordings and a synthetic tone) × 7 speed ratios × 4 variants: 196 files.</p>

<div class="card">
<h3>Spectral distance on speech, by ratio</h3>
<p class="meta">Log-spectral distance (dB) between each output and the input's spectrogram stretched to the output's
length, averaged over the six speech files. Lower is closer to an ideal time-scaled input.</p>
<div class="legend" id="legLsd"></div>
<div class="chart" id="chLsd"></div>
<details><summary>Table view</summary><div id="tblLsd"></div></details>
</div>

<div class="card">
<h3>Output level on speech, by ratio</h3>
<p class="meta">RMS level of the output relative to the input (dB). Cross-fading segments that are not fully alike
loses some energy; longer overlaps lose more at high compression.</p>
<div class="legend" id="legLvl"></div>
<div class="chart" id="chLvl"></div>
<details><summary>Table view</summary><div id="tblLvl"></div></details>
</div>

<div class="card">
<h3>Discontinuities and tone periodicity</h3>
<p class="meta">Click-like events per second (speech mean; lower is better) and gliding-tone periodicity (higher is
better; 1 is perfectly periodic). Periodicity is effectively identical across variants, since all four use the same
alignment search.</p>
<div id="tblClk"></div>
<div id="tblPer" style="margin-top:12px"></div>
</div>
</main></div>
<script>
const DATA = /*DATA*/null;
/*HELPERS*/
const KEY = "solafs-wsola-votes";
const votes = JSON.parse(localStorage.getItem(KEY) || "{}");
const reveals = [];
function player(label, src) {
  const p = el("div", {class: "player"}); p.append(el("span", {class: "lab"}, label));
  p.append(el("audio", {controls: "", preload: "none", src})); return p;
}
function updateScore() {
  let a = 0, d = 0, same = 0;
  for (const ex of DATA.examples) { const v = votes[ex.id]; if (!v) continue;
    if (v === "same") same++; else if (ex[v] === DATA.pair[0]) a++; else d++; }
  const n = a + d + same;
  document.getElementById("score").textContent = n ? `Choices so far (${n} of ${DATA.examples.length}): SOLAFS ${a}, WSOLA ${d}, no difference ${same}` : "";
}
const host = document.getElementById("examples");
for (const ex of DATA.examples) {
  const c = el("div", {class: "card"});
  c.append(el("h3", {}, `${ex.id}. ${ex.input} at ${ex.ratio}x`), el("div", {class: "meta"}, ex.why));
  const r = el("div", {class: "row"}); r.style.marginTop = "10px";
  r.append(player("Original", ex.original), player("A", ex.files[ex.A]), player("B", ex.files[ex.B])); c.append(r);
  const vote = el("div", {class: "vote"});
  for (const [val, txt] of [["A", "A is better"], ["B", "B is better"], ["same", "No difference"]]) {
    const lab = el("label"); const inp = el("input", {type: "radio", name: `v${ex.id}`, value: val});
    if (votes[ex.id] === val) inp.checked = true;
    inp.addEventListener("change", () => { votes[ex.id] = val; localStorage.setItem(KEY, JSON.stringify(votes)); updateScore(); });
    lab.append(inp, document.createTextNode(txt)); vote.append(lab);
  }
  c.append(vote);
  const rv = el("div", {class: "reveal"}); const btn = el("button", {}, "Reveal"); const ans = el("span"); ans.hidden = true;
  const ma = ex.metrics[DATA.pair[0]], md = ex.metrics[DATA.pair[1]];
  ans.append(document.createTextNode(" "), el("strong", {}, `A = ${DATA.short[ex.A]}, B = ${DATA.short[ex.B]}.`),
    document.createTextNode(` Spectral distance ${fmt(ma.lsd_db, 2)} → ${fmt(md.lsd_db, 2)} dB, level ${fmt(ma.level_db, 2)} → ${fmt(md.level_db, 2)} dB (SOLAFS → WSOLA).`));
  btn.addEventListener("click", () => { ans.hidden = !ans.hidden; btn.textContent = ans.hidden ? "Reveal" : "Hide"; });
  reveals.push([ans, btn]); rv.append(btn, ans); c.append(rv);
  const det = el("details"); det.append(el("summary", {}, "All four variants (labelled)"));
  const r2 = el("div", {class: "row"}); r2.style.marginTop = "8px";
  for (const v of DATA.variants) r2.append(player(DATA.label[v], ex.files[v]));
  det.append(r2); c.append(det); host.append(c);
}
document.getElementById("revealAll").addEventListener("click", () => { for (const [a, b] of reveals) { a.hidden = false; b.textContent = "Hide"; } });
document.getElementById("clearVotes").addEventListener("click", () => { for (const k in votes) delete votes[k]; localStorage.removeItem(KEY);
  document.querySelectorAll(".vote input").forEach(i => i.checked = false); updateScore(); });
updateScore();

const xl = DATA.ratios.map(r => `${r}x`);
const items = DATA.variants.map(v => [DATA.short[v], DATA.colors[v]]);
const series = m => DATA.variants.map(v => ({name: DATA.short[v], values: DATA.by[m][v], color: DATA.colors[v]}));
legend("legLsd", items, "line");
lineChart("chLsd", "Spectral distance (dB), lower is better", xl, series("lsd_db"), 3, 13, [3, 5, 7, 9, 11, 13], 0, 2, " dB");
legend("legLvl", items, "line");
lineChart("chLvl", "Output level relative to input (dB)", xl, series("level_db"), -1.2, 0.2, [-1.2, -0.8, -0.4, 0], 1, 2, " dB");
const head = ["Ratio", ...DATA.variants.map(v => DATA.short[v])];
table("tblLsd", head, DATA.ratios.map((r, i) => [`${r}x`, ...DATA.variants.map(v => DATA.by.lsd_db[v][i].toFixed(2))]));
table("tblLvl", head, DATA.ratios.map((r, i) => [`${r}x`, ...DATA.variants.map(v => DATA.by.level_db[v][i].toFixed(2))]));
table("tblClk", ["Clicks/s", ...head.slice(1)], DATA.ratios.map((r, i) => [`${r}x`, ...DATA.variants.map(v => DATA.by.clicks_ps[v][i].toFixed(2))]));
table("tblPer", ["Tone periodicity", ...head.slice(1)], DATA.ratios.map((r, i) => [`${r}x`, ...DATA.variants.map(v => DATA.tone[v][i].toFixed(4))]));
</script></body></html>
"""

if __name__ == "__main__":
    main()
