#!/usr/bin/env python3
"""Build a real table of contents for the DOCX edition.

Pandoc's --toc writes a Word TOC field with no cached entries. Word fills it on
demand, but LibreOffice and most PDF converters render it as nothing at all, so
the delivered file would show an empty contents page. Rather than depend on the
reader, this writes the entries as ordinary paragraphs with a right tab stop and
a dot leader, exactly as a typeset contents page looks.

Page numbers cannot be known before the document is laid out, so the build runs
in rounds: inject, render, read the page each heading actually landed on, and
inject again until the numbers stop moving.

Subcommands:
    headings MASTER.md OUT.json        list the H1/H2 headings in document order
    extract  TEXT.txt OUT.json         page number of each heading in a render
    inject   FILE.docx TOC.json        replace the placeholder with TOC entries
"""

import json
import pathlib
import re
import sys
import zipfile

PLACEHOLDER = "TOCPLACEHOLDER"

# A4 minus two 20 mm margins, in twips: the right edge for the leader tab.
TEXT_WIDTH = 11906 - 2 * 1134

# A contents entry is set smaller than body text so the list stays compact.
ENTRY_SZ = "21"          # half-points: 10.5 pt
H1_INDENT = "0"
H2_INDENT = "340"

# Pages whose text is a contents page must not be searched for headings. The
# injected leader is a run of dots from a tab stop, so it is contiguous.
TOC_PAGE = re.compile(r"\.{5,}")


def clean_heading(text):
    text = text.lstrip("#").strip()
    text = text.replace("`", "").replace("**", "").replace("*", "")
    return re.sub(r"\s+", " ", text)


def headings(master):
    out = []
    for line in pathlib.Path(master).read_text().splitlines():
        if line.startswith("## "):
            out.append((2, clean_heading(line)))
        elif line.startswith("# "):
            out.append((1, clean_heading(line)))
    # "References" is an H2 in the source but a top-level division in the PDF.
    # The contents page does not list itself.
    return [{"level": 1 if t == "References" else lv, "text": t}
            for lv, t in out if t != "Contents"]


def extract(pages, heads):
    """Find the page each heading starts on, searching forward only."""
    flat = [re.sub(r"\s+", " ", p) for p in pages]
    cursor = 0
    for h in heads:
        needle = h["text"][:38]
        h["page"] = None
        for i in range(cursor, len(flat)):
            if TOC_PAGE.search(pages[i]):
                continue
            if needle and needle in flat[i]:
                h["page"] = i + 1          # printed numbers start at 1
                cursor = i
                break
        if h["page"] is None:              # fall back to a shorter prefix
            short = h["text"][:20]
            for i in range(cursor, len(flat)):
                if short and short in flat[i]:
                    h["page"] = i + 1
                    cursor = i
                    break
    return heads


def paragraph(text, page, indent):
    def esc(s):
        return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))

    rpr = '<w:rPr><w:sz w:val="%s"/><w:szCs w:val="%s"/></w:rPr>' % (ENTRY_SZ, ENTRY_SZ)
    return (
        "<w:p><w:pPr>"
        '<w:tabs><w:tab w:val="right" w:leader="dot" w:pos="%d"/></w:tabs>'
        '<w:spacing w:before="0" w:after="40" w:line="240" w:lineRule="auto"/>'
        '<w:ind w:left="%s"/>'
        "</w:pPr>"
        "<w:r>%s<w:t xml:space=\"preserve\">%s</w:t></w:r>"
        "<w:r>%s<w:tab/></w:r>"
        "<w:r>%s<w:t>%s</w:t></w:r>"
        "</w:p>"
        % (TEXT_WIDTH, indent, rpr, esc(text), rpr, rpr, page if page else "—")
    )


def inject(docx, toc):
    with zipfile.ZipFile(docx) as src:
        items = {n: src.read(n) for n in src.namelist()}

    document = items["word/document.xml"].decode("utf-8")
    if PLACEHOLDER not in document:
        raise SystemExit("inject: placeholder not found in %s" % docx)

    body = "".join(
        paragraph(h["text"], h["page"], H1_INDENT if h["level"] == 1 else H2_INDENT)
        for h in toc
    )
    document = re.sub(
        r"<w:p>(?:(?!</w:p>).)*%s(?:(?!</w:p>).)*</w:p>" % PLACEHOLDER,
        body,
        document,
        count=1,
        flags=re.S,
    )
    items["word/document.xml"] = document.encode("utf-8")

    with zipfile.ZipFile(docx, "w", zipfile.ZIP_DEFLATED) as out:
        for name, data in items.items():
            out.writestr(name, data)
    print("inject: %d entries" % len(toc))


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    cmd = argv[0]

    if cmd == "headings":
        json.dump(headings(argv[1]), open(argv[2], "w"), indent=1)
        print("headings: %s" % argv[2])
        return 0

    if cmd == "extract":
        heads = json.load(open(argv[1]))
        pages = pathlib.Path(argv[2]).read_text(
            encoding="utf-8", errors="replace").split("\f")
        heads = extract(pages, heads)
        json.dump(heads, open(argv[3], "w"), indent=1)
        missing = [h["text"] for h in heads if not h["page"]]
        print("extract: %d headings, %d not found %s"
              % (len(heads), len(missing), missing[:3]))
        return 0

    if cmd == "inject":
        inject(argv[1], json.load(open(argv[2])))
        return 0

    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
