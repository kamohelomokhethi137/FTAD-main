#!/usr/bin/env python3
"""Apply the markdown image widths to a pandoc-generated DOCX.

Pandoc's docx writer ignores `{width=100%}` and sizes an image from its pixel
dimensions at 96 dpi, which made every figure 5 in wide regardless of what the
source asked for. This reads the requested percentages back out of the markdown
and rewrites the drawing extents, keeping each image's aspect ratio.

The images appear in the document in source order and pandoc renames the media
files (rId132.png and so on), so the fractions are matched by position rather
than by filename.

Usage:
    python3 set_figure_widths.py FILE.docx MASTER.md
"""

import re
import sys
import zipfile

# A4 minus two 20 mm margins, in EMU (1 twip = 635 EMU).
TEXT_WIDTH = 9638 * 635


def widths(master):
    text = open(master).read()
    return [int(m.group(1)) / 100.0
            for m in re.finditer(r"\(figures/png/[^)]+\)\{width=(\d+)%", text)]


def fix_docx(path, fractions):
    with zipfile.ZipFile(path) as src:
        items = {n: src.read(n) for n in src.namelist()}

    doc = items["word/document.xml"].decode("utf-8")
    blocks = list(re.finditer(r"<wp:inline[^>]*>.*?</wp:inline>", doc, flags=re.S))

    if len(blocks) != len(fractions):
        raise SystemExit(
            "set_figure_widths: %d images in the document, %d widths in the "
            "markdown" % (len(blocks), len(fractions)))

    out = []
    cursor = 0
    for block, frac in zip(blocks, fractions):
        text = block.group(0)
        ext = re.search(r'<wp:extent cx="(\d+)" cy="(\d+)"\s*/>', text)
        if not ext:
            raise SystemExit("set_figure_widths: no extent in an image block")

        cx, cy = int(ext.group(1)), int(ext.group(2))
        new_cx = int(TEXT_WIDTH * frac)
        new_cy = int(new_cx * cy / cx)

        text = text.replace(ext.group(0),
                            '<wp:extent cx="%d" cy="%d"/>' % (new_cx, new_cy))
        text = re.sub(r'<a:ext cx="%d" cy="%d"\s*/>' % (cx, cy),
                      '<a:ext cx="%d" cy="%d"/>' % (new_cx, new_cy), text)

        out.append(doc[cursor:block.start()])
        out.append(text)
        cursor = block.end()
    out.append(doc[cursor:])

    items["word/document.xml"] = "".join(out).encode("utf-8")
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as dst:
        for name, data in items.items():
            dst.writestr(name, data)
    print("figures: %d widths applied" % len(blocks))


if __name__ == "__main__":
    fix_docx(sys.argv[1], widths(sys.argv[2]))
