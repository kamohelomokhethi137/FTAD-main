#!/bin/bash
# Build the DOCX edition of the report from the same chapter sources as the PDF.
#
# The PDF pipeline (build.sh) typesets with XeLaTeX. This one goes through
# pandoc's docx writer and then rewrites the OOXML package so the result meets
# the report specification: Tahoma 12 pt, 1.5 line spacing, A4, "Page X of Y".
#
# Three things the PDF pipeline does not have to solve:
#   * Word cannot embed PDF graphics, so the six Graphviz views are rasterised
#     to 300 dpi PNG for this edition only.
#   * The title page date carries a LaTeX line break ("\\6 November") that would
#     print as a stray backslash in Word, so it is rewritten on the fly.
#   * A Word TOC field is empty until the reader updates it, so the contents
#     page is written out as real paragraphs (make_toc.py). Its page numbers can
#     only be known after layout, so the build injects, renders, reads the pages
#     back and repeats until the numbers stop moving.
set -euo pipefail
cd "$(dirname "$0")"

TMP=/tmp/opencode/docx-build
mkdir -p "$TMP"

python3 assemble.py

# --- figures: PDF -> PNG for Word -------------------------------------------
mkdir -p figures/png
for f in figures/*.pdf; do
    b=$(basename "$f" .pdf)
    if [ ! -f "figures/png/$b.png" ] || [ "$f" -nt "figures/png/$b.png" ]; then
        pdftoppm -png -r 300 -singlefile "$f" "figures/png/$b"
        # The Graphviz page carries its own margins; trim them so the drawing
        # fills the width it is given, as the vector original does in the PDF.
        magick "figures/png/$b.png" -trim +repage "figures/png/$b.png"
    fi
done

# --- DOCX source: same text, PNG figures, plain date, front matter ----------
python3 - <<'PY'
import pathlib
import re

text = pathlib.Path("master.md").read_text()

# Word has no use for the LaTeX line break in the date.
text = re.sub(r"^date: .*$",
              'date: "BIST4110 Software Testing and Reliability, 6 November 2026"',
              text, count=1, flags=re.M)

# Word cannot embed PDF; the PNG renderings sit beside them.
text = text.replace("](figures/", "](figures/png/").replace(".pdf)", ".png)")

BREAK = '```{=openxml}\n<w:p><w:r><w:br w:type="page"/></w:r></w:p>\n```\n\n'

# Title page, then the abstract on its own page.
text = text.replace("\n# Abstract\n", "\n" + BREAK + "# Abstract\n", 1)

# Contents between the abstract and the introduction, each on a new page.
contents = "\n" + BREAK + "# Contents\n\nTOCPLACEHOLDER\n\n" + BREAK
text = text.replace("\n# Introduction\n", contents + "# Introduction\n", 1)

# The test-case table needs a wider ID column in Word than in the PDF, because
# Word will not break "TC-DUP-001" at the hyphen inside a narrow cell.
text = text.replace(
    "|----------|---------------|-------------|-------------------|"
    "-----------------------|--------|",
    "|------------|--------------|----------|----------------|"
    "----------------------|------|")

# Word has no hyphenation point inside an identifier, so it breaks
# "DuplicatePaymentFlagsBothSides" at whatever character meets the cell edge.
# A zero-width space at each lower-to-upper boundary gives it somewhere sensible
# to wrap. This mirrors the \\- insertions made for the LaTeX tables.
ZWSP = "\u200b"


def _break_identifier(match):
    word = match.group(0)
    out = [word[0]]
    for prev, ch in zip(word, word[1:]):
        if prev.islower() and ch.isupper():
            out.append(ZWSP)
        out.append(ch)
    return "".join(out)


lines = []
for line in text.splitlines():
    if line.startswith("|"):
        line = re.sub(r"[A-Za-z]{16,}", _break_identifier, line)
    lines.append(line)
text = "\n".join(lines)

pathlib.Path("master_docx.md").write_text(text)
print("master_docx.md written")
PY

# --- Word-metric font for the page-number render -----------------------------
# Page numbers in the contents are read back out of the render, so the render has
# to be laid out the way Word will lay it out. This host has no real Tahoma, only
# Wine's substitute, and that substitute declares USE_TYPO_METRICS with a 1.0 em
# typographic line. Every genuine Microsoft font here (Arial, Times, Courier,
# Verdana) leaves that flag clear and measures 1.207 em, so Word sets 1.5 spacing
# 20 % looser than LibreOffice does with the substitute. Building a metric-
# corrected copy and pointing fontconfig at it for the render only makes the
# numbers describe Word's pagination; the .docx keeps "1.5 lines" either way.
WM="$TMP/wm"
FONTCONFIG_EXTRA=""
PROF="$(mktemp -d "$TMP/prof.XXXXXX")"
if python3 make_word_metrics_font.py "$WM" >/dev/null 2>&1; then
    FONTCONFIG_EXTRA="$WM/fonts.conf"
else
    echo "warning: metric-corrected font unavailable; page numbers follow the substitute" >&2
fi

# --- one build round --------------------------------------------------------
round() {
    pandoc master_docx.md \
        -o FTAD_Report.docx \
        --resource-path=.:figures/png

    FTAD_MARGIN_TWIPS=1134 \
        python3 ../../documentation/apply_report_style.py FTAD_Report.docx

    python3 fix_catalogue_table.py FTAD_Report.docx
    python3 set_figure_widths.py FTAD_Report.docx master_docx.md
    python3 make_toc.py inject FTAD_Report.docx "$1"

    # Ask Word to refresh page fields when the file is opened.
    python3 - <<'PY'
import zipfile
path = "FTAD_Report.docx"
with zipfile.ZipFile(path) as src:
    items = {n: src.read(n) for n in src.namelist()}
s = items["word/settings.xml"].decode("utf-8")
if "updateFields" not in s:
    s = s.replace("</w:settings>", '<w:updateFields w:val="true"/></w:settings>')
items["word/settings.xml"] = s.encode("utf-8")
with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as out:
    for name, data in items.items():
        out.writestr(name, data)
PY

    rm -f "$TMP/FTAD_Report.pdf"
    if [ -n "$FONTCONFIG_EXTRA" ]; then
        FONTCONFIG_FILE="$FONTCONFIG_EXTRA" soffice --headless \
            -env:UserInstallation="file://$PROF" \
            --convert-to pdf --outdir "$TMP" FTAD_Report.docx >"$TMP/lo.log" 2>&1
    else
        soffice --headless --convert-to pdf --outdir "$TMP" FTAD_Report.docx \
            >"$TMP/lo.log" 2>&1
    fi
    pdftotext -layout "$TMP/FTAD_Report.pdf" "$TMP/render.txt"
}

python3 make_toc.py headings master_docx.md "$TMP/headings.json"
echo '[]' > "$TMP/toc.json"

for i in 1 2 3 4; do
    round "$TMP/toc.json"
    python3 make_toc.py extract "$TMP/headings.json" "$TMP/render.txt" "$TMP/new.json"

    if python3 toc_compare.py "$TMP/toc.json" "$TMP/new.json"; then
        echo "contents settled after $i round(s)"
        break
    fi
    cp "$TMP/new.json" "$TMP/toc.json"
done

echo "FTAD_Report.docx written: $(stat -c%s FTAD_Report.docx) bytes"
echo "rendered: $TMP/FTAD_Report.pdf ($(pdfinfo "$TMP/FTAD_Report.pdf" | awk '/^Pages/{print $2}') pages)"
