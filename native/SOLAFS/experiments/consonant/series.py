"""Speed series for the full Nature's Miracle clip (27 s): uniform vs V3L at 1.5x to 12x, labelled, for
listening. Renders with render.py's code, then writes out/consonant/series/index.html."""
import shutil
import sys
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common import OUT, page_assets
from render import RENDERS, TRIMMED, render_input

NAME = "natures_miracle"
RATIOS = [1.5, 2, 3, 4, 5, 6, 8, 10, 12]
DEST = OUT / "consonant" / "series"


def seconds(p):
    with wave.open(str(p)) as w:
        return w.getnframes() / w.getframerate()


def main():
    render_input(NAME, RATIOS, ["U-uniform", "V3L"])
    if DEST.exists():
        shutil.rmtree(DEST)
    d = DEST / "audio"; d.mkdir(parents=True)
    shutil.copyfile(TRIMMED / f"{NAME}.wav", d / "original.wav")
    rows = []
    for q in RATIOS:
        cells = []
        for v, lab in (("U-uniform", "Uniform"), ("V3L", "Consonant-aware")):
            dst = d / f"{q:g}-{v}.wav"
            shutil.copyfile(RENDERS / NAME / f"{q:g}" / f"{v}.wav", dst)
            cells.append(f'<div class="player"><span class="lab">{lab}</span><audio controls preload="none" src="audio/{dst.name}"></audio></div>')
        rows.append(f'<div class="card"><h3>{q:g}x <span class="meta">({seconds(d / f"{q:g}-U-uniform.wav"):.2f} s)</span></h3>'
                    f'<div class="row">{"".join(cells)}</div></div>')
    css, _ = page_assets()
    html = f"""<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Nature's Miracle: speed series</title><style>
{css}</style></head><body><div class="viz-root"><main>
<h1>Nature&rsquo;s Miracle (full clip), 1.5&times; to 12&times;</h1>
<p class="lead">Uniform SOLAFS against the consonant-aware schedule with look-ahead (V3L), at equal length. Both are
labelled.</p>
<div class="card"><h3>Original <span class="meta">({seconds(d / "original.wav"):.2f} s, silence trimmed)</span></h3><div class="row">
<div class="player"><span class="lab">Original</span><audio controls preload="none" src="audio/original.wav"></audio></div></div></div>
{"".join(rows)}
</main></div></body></html>"""
    (DEST / "index.html").write_text(html)
    print("wrote", DEST / "index.html")


if __name__ == "__main__":
    main()
