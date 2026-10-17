"""Uniform vs consonant-aware SOLAFS listening and measurement page: out/consonant/page/index.html, with
audio and a schedule plot alongside. Run render.py and measure.py (default settings) first."""
import csv, json, random, shutil, sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, page_assets, read_wav
from render import RENDERS, TRIMMED

DEST = OUT / "consonant" / "page"


def plot_schedule(path, names=("bruce_walker", "jeff_wilson"), ratio=8):
    """Spectrogram of each name with the uniform ratio and the consonant-aware (V3L) schedule."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, axs = plt.subplots(len(names), 1, figsize=(11, 2.9 * len(names)))
    for ax, nm in zip(np.atleast_1d(axs), names):
        x, sr = read_wav(TRIMMED / f"{nm}.wav")
        f = RENDERS / nm / f"{ratio:g}" / "V3L.sched.txt"
        h = int(open(f).readline()); r = np.loadtxt(f, skiprows=1)
        ax.specgram(x + 1e-9, NFFT=512, Fs=sr, noverlap=448, cmap="Greys", vmin=-110); ax.set_ylim(0, 8000)
        ax.set_ylabel("Hz"); ax.set_title(f"{nm.replace('_', ' ')}: speed ratio over time, {ratio:g}x overall", fontsize=10, loc="left")
        t = (np.arange(len(r)) + 0.5) * h / sr
        a2 = ax.twinx(); a2.plot(t, r, color="#2a78d6", lw=2, label="consonant-aware (V3L)")
        a2.axhline(ratio, color="#eb6834", lw=2, label="uniform"); a2.set_ylabel("ratio"); a2.set_ylim(0, max(r.max(), ratio) * 1.1)
        a2.legend(loc="upper right", fontsize=8, frameon=False)
    np.atleast_1d(axs)[-1].set_xlabel("input time (s)")
    plt.tight_layout(); plt.savefig(path, dpi=90); plt.close(fig)


VARIANTS = ["U-uniform", "V3", "V3L", "V3A"]
LABEL = {"U-uniform": "U: uniform (current SOLAFS)", "V3": "V3: consonant-aware, no look-ahead",
         "V3L": "V3L: consonant-aware with look-ahead", "V3A": "V3A: V3L plus onset anchors"}
SHORT = {"U-uniform": "Uniform", "V3": "V3", "V3L": "V3L", "V3A": "V3A"}
COLORS = {"U-uniform": "--series-1", "V3": "--series-2", "V3L": "--series-3", "V3A": "--series-4"}
PAIR = ("U-uniform", "V3L")
EXAMPLES = [
    ("bruce_walker", 5, "About the demo's 0.2 s target."),
    ("jeff_wilson", 8, "About the demo's 0.2 s target."),
    ("allegra_sidner", 8, "16 kHz recording."),
    ("bruce_walker", 12, "Beyond the demo's target."),
    ("jeff_wilson", 12, ""),
    ("natures_miracle_med", 12, "A longer phrase."),
    ("abc", 16, "The alphabet: mostly consonant-vowel letter names."),
    ("bruce_walker", 20, "Extreme: about 20 steps for the whole name."),
    ("natures_miracle_med", 20, "Extreme."),
]


def main():
    rows = list(csv.DictReader(open(OUT / "consonant" / "results.csv")))
    for r in rows:
        for k in r:
            if k not in ("input", "variant"): r[k] = float(r[k])
    ratios = sorted({int(r["ratio"]) for r in rows})

    def pooled(q, v):
        rs = [r for r in rows if r["ratio"] == q and r["variant"] == v]
        ev = sum(r["events"] for r in rs)
        return {"kept": 100 * sum(r["cons_kept_ms"] for r in rs) / sum(r["cons_in_ms"] for r in rs),
                "lost": 100 * sum(r["lost"] for r in rs) / ev,
                "vowel": 100 * sum(r["vowel_kept_ms"] for r in rs) / sum(r["vowel_in_ms"] for r in rs)}
    by = {m: {v: [pooled(q, v)[m] for q in ratios] for v in VARIANTS} for m in ("kept", "lost", "vowel")}
    idx = {(r["input"], int(r["ratio"]), r["variant"]): r for r in rows}

    if DEST.exists(): shutil.rmtree(DEST)
    (DEST / "audio").mkdir(parents=True)
    rng = random.Random(7); examples = []
    for n, (inp, q, why) in enumerate(EXAMPLES, 1):
        files = {}
        for v in VARIANTS:
            dst = DEST / "audio" / inp / f"{q:g}" / f"{v}.wav"; dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(RENDERS / inp / f"{q:g}" / f"{v}.wav", dst); files[v] = str(dst.relative_to(DEST))
        src = DEST / "audio" / "inputs" / f"{inp}.wav"; src.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(TRIMMED / f"{inp}.wav", src)
        a_first = rng.random() < 0.5
        examples.append({"id": n, "input": inp, "ratio": q, "why": why, "original": str(src.relative_to(DEST)), "files": files,
                         "A": PAIR[0] if a_first else PAIR[1], "B": PAIR[1] if a_first else PAIR[0],
                         "metrics": {v: {"kept": 100 * idx[(inp, q, v)]["cons_kept_ms"] / idx[(inp, q, v)]["cons_in_ms"],
                                         "out_ms": idx[(inp, q, v)]["out_ms"]} for v in VARIANTS}})
    plot_schedule(DEST / "schedule.png")
    shutil.copyfile(OUT / "consonant" / "results.csv", DEST / "results.csv")
    data = {"variants": VARIANTS, "label": LABEL, "short": SHORT, "colors": COLORS, "pair": PAIR, "ratios": ratios,
            "examples": examples, "by": by}
    css, js = page_assets()
    html = PAGE.replace("/*CSS*/", "<style>\n" + css + "</style>").replace("/*DATA*/null", json.dumps(data)) \
               .replace("/*HELPERS*/", js)
    (DEST / "index.html").write_text(html); print("wrote", DEST / "index.html")



PAGE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Consonant-aware SOLAFS: listening test and measurements</title>
/*CSS*/
<style>.fig{max-width:100%;border-radius:8px;background:#fff;margin-top:8px}</style>
</head><body><div class="viz-root"><main>
<h1>Consonant-aware SOLAFS</h1>
<p class="lead">At high compression, uniform SOLAFS keeps one 6.8 ms window for every <em>ratio</em> × 3.4 ms of
input, so a short consonant (a stop burst lasts 5–15 ms) can fall entirely in the part that is skipped. Here the
speed ratio varies through the word instead: consonants are compressed three times less than vowels, and silence
twice as much as vowels, scaled so the total length is the same as uniform compression.</p>

<div class="card">
<h3>The variants</h3>
<ul>
<li><strong>Uniform</strong>: current SOLAFS, one ratio for the whole file.</li>
<li><strong>V3</strong>: ratio per 5 ms of input from a time-domain consonant detector (no FFT): RMS frequency from
the sample-to-sample difference, and jumps in level. Consonants get 1/3 of the vowel ratio.</li>
<li><strong>V3L</strong>: V3 plus a more sensitive burst detector and <em>look-ahead</em>. Each ratio sets the jump
to the next step, so a jump from a heavily compressed vowel could land past a protected consonant; with look-ahead
each step uses the lowest ratio anywhere its jump could reach.</li>
<li><strong>V3A</strong>: V3L plus <em>anchors</em> that force a step to land just before each detected onset.</li>
</ul>
<p class="meta">All use the same window (6.8 ms), overlap (3.4 ms) and similarity search (13.6 ms) as spearcon.exe,
on the same input trimmed of leading and trailing silence. Output lengths match uniform to within 0.5% in most
cases, and to within one or two 3.4 ms steps in the shortest outputs.</p>
<img class="fig" src="schedule.png" alt="Spectrograms of Bruce Walker and Jeff Wilson with the consonant-aware speed ratio over time, against the constant uniform ratio of 8">
</div>

<h2>Listening test</h2>
<p>Each example compares <strong>A</strong> and <strong>B</strong>: one is uniform SOLAFS and the other V3L,
assigned at random. Choose the one in which the name or phrase is easier to recognise, or "no difference", then
reveal. Choices are stored in this browser only. All four variants are available under each example.</p>
<div class="row" style="margin: 8px 0 4px">
  <button id="revealAll">Reveal all</button>
  <button id="clearVotes">Clear choices</button>
  <span class="score" id="score"></span>
</div>
<div id="examples"></div>

<h2>Measurements</h2>
<p>Six speech recordings × six ratios × four variants. Consonants are labelled from a spectrogram, independently of
the detector that sets the ratios: fricative frames (more than 40% of the energy above 2.5 kHz) and stop bursts
(sudden broadband rises), 133 events in all. Voiced, nasal and liquid consonants are not labelled. The driver logs
the input position of every step, so the amount of each consonant that reaches the output is known exactly.</p>

<div class="card">
<h3>Consonant material kept, by ratio</h3>
<p class="meta">Share of labelled consonant input that appears in the output, pooled over all six recordings.
Uniform keeps about 1/ratio of everything; higher is better.</p>
<div class="legend" id="legKept"></div><div class="chart" id="chKept"></div>
<details><summary>Table view</summary><div id="tblKept"></div></details>
</div>

<div class="card">
<h3>Consonants lost, by ratio</h3>
<p class="meta">Share of labelled consonants of which less than 2 ms reaches the output. Lower is better.</p>
<div class="legend" id="legLost"></div><div class="chart" id="chLost"></div>
<details><summary>Table view</summary><div id="tblLost"></div></details>
</div>

<div class="card">
<h3>Vowel material kept</h3>
<p class="meta">The cost: at equal length, what consonants gain comes from vowels (and pauses inside the phrase).</p>
<div id="tblVowel"></div>
</div>
</main></div>
<script>
const DATA = /*DATA*/null;
/*HELPERS*/
const KEY = "solafs-consonant-votes";
const votes = JSON.parse(localStorage.getItem(KEY) || "{}");
const reveals = [];
function player(label, src) {
  const p = el("div", {class: "player"}); p.append(el("span", {class: "lab"}, label));
  p.append(el("audio", {controls: "", preload: "none", src})); return p;
}
function updateScore() {
  let u = 0, v = 0, same = 0;
  for (const ex of DATA.examples) { const c = votes[ex.id]; if (!c) continue;
    if (c === "same") same++; else if (ex[c] === DATA.pair[0]) u++; else v++; }
  const n = u + v + same;
  document.getElementById("score").textContent = n ? `Choices so far (${n} of ${DATA.examples.length}): uniform ${u}, consonant-aware ${v}, no difference ${same}` : "";
}
const host = document.getElementById("examples");
for (const ex of DATA.examples) {
  const c = el("div", {class: "card"});
  const ms = Math.round(ex.metrics[DATA.pair[0]].out_ms);
  c.append(el("h3", {}, `${ex.id}. ${ex.input} at ${ex.ratio}x (${ms} ms)`), el("div", {class: "meta"}, ex.why));
  const r = el("div", {class: "row"}); r.style.marginTop = "10px";
  r.append(player("Original", ex.original), player("A", ex.files[ex.A]), player("B", ex.files[ex.B])); c.append(r);
  const vote = el("div", {class: "vote"});
  for (const [val, txt] of [["A", "A is easier to recognise"], ["B", "B is easier to recognise"], ["same", "No difference"]]) {
    const lab = el("label"); const inp = el("input", {type: "radio", name: `v${ex.id}`, value: val});
    if (votes[ex.id] === val) inp.checked = true;
    inp.addEventListener("change", () => { votes[ex.id] = val; localStorage.setItem(KEY, JSON.stringify(votes)); updateScore(); });
    lab.append(inp, document.createTextNode(txt)); vote.append(lab);
  }
  c.append(vote);
  const rv = el("div", {class: "reveal"}); const btn = el("button", {}, "Reveal"); const ans = el("span"); ans.hidden = true;
  const mu = ex.metrics[DATA.pair[0]], mv = ex.metrics[DATA.pair[1]];
  ans.append(document.createTextNode(" "), el("strong", {}, `A = ${DATA.short[ex.A]}, B = ${DATA.short[ex.B]}.`),
    document.createTextNode(` Consonant material kept ${fmt(mu.kept, 0)}% → ${fmt(mv.kept, 0)}% (uniform → V3L).`));
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
legend("legKept", items, "line");
lineChart("chKept", "Consonant material kept (%)", xl, series("kept"), 0, 70, [0, 10, 20, 30, 40, 50, 60, 70], 0, 1, "%");
legend("legLost", items, "line");
lineChart("chLost", "Consonants lost (%)", xl, series("lost"), 0, 80, [0, 20, 40, 60, 80], 0, 0, "%");
const head = ["Ratio", ...DATA.variants.map(v => DATA.short[v])];
const tab = (id, m, d) => table(id, head, DATA.ratios.map((r, i) => [`${r}x`, ...DATA.variants.map(v => DATA.by[m][v][i].toFixed(d) + "%")]));
tab("tblKept", "kept", 1); tab("tblLost", "lost", 0); tab("tblVowel", "vowel", 1);
</script></body></html>
"""

if __name__ == "__main__":
    main()
