#!/usr/bin/env python3
"""Apply the FTAD report style to a pandoc-generated .docx.

Pandoc emits a document whose fonts come from the OOXML theme (Calibri and
Cambria) at single spacing with no page geometry. This script rewrites the
package so the file matches the report specification:

    body font      Tahoma 12 pt
    line spacing   1.5
    page           A4, 25 mm margins
    footer         "Page X of Y", centred

It edits the OOXML directly with the standard library, because python-docx is
not installed on the target host and adding a dependency for a cosmetic pass
would be disproportionate.

Usage:
    python3 apply_report_style.py FILE.docx [FILE.docx ...]
"""

import os
import re
import sys
import zipfile

FONT = "Tahoma"
MONO = "Courier New"

# Half-points: 24 = 12 pt.
BODY_SZ = "24"
# Twips: 360 = 1.5 lines (240 would be single), 120 = 6 pt after.
LINE_TWIPS = "360"
AFTER_TWIPS = "120"

# A4 in twips, and the page margin. 1417 twips = 25 mm; the bound report uses
# 20 mm (1134) so that the DOCX paginates like the PDF, which is what the page
# budget was measured on. Override with FTAD_MARGIN_TWIPS.
PAGE_W, PAGE_H = "11906", "16838"
MARGIN = os.environ.get("FTAD_MARGIN_TWIPS", "1417")
HEADER_FOOTER = "709"

# Styles that must stay monospaced; everything else becomes Tahoma.
MONO_STYLES = {"VerbatimChar", "SourceCode", "HTMLPreformatted", "Code"}

RPR_TAHOMA = (
    '<w:rFonts w:ascii="{f}" w:hAnsi="{f}" w:cs="{f}" w:eastAsia="{f}"/>'
    '<w:sz w:val="{s}"/><w:szCs w:val="{s}"/>'
).format(f=FONT, s=BODY_SZ)

FOOTER_XML = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:pPr>
      <w:spacing w:before="0" w:after="0" w:line="240" w:lineRule="auto"/>
      <w:jc w:val="center"/>
    </w:pPr>
    <w:r>
      <w:rPr><w:rFonts w:ascii="%(font)s" w:hAnsi="%(font)s"/><w:sz w:val="20"/></w:rPr>
      <w:t xml:space="preserve">Page </w:t>
    </w:r>
    <w:fldSimple w:instr=" PAGE ">
      <w:r>
        <w:rPr><w:rFonts w:ascii="%(font)s" w:hAnsi="%(font)s"/><w:sz w:val="20"/></w:rPr>
        <w:t>1</w:t>
      </w:r>
    </w:fldSimple>
    <w:r>
      <w:rPr><w:rFonts w:ascii="%(font)s" w:hAnsi="%(font)s"/><w:sz w:val="20"/></w:rPr>
      <w:t xml:space="preserve"> of </w:t>
    </w:r>
    <w:fldSimple w:instr=" NUMPAGES ">
      <w:r>
        <w:rPr><w:rFonts w:ascii="%(font)s" w:hAnsi="%(font)s"/><w:sz w:val="20"/></w:rPr>
        <w:t>1</w:t>
      </w:r>
    </w:fldSimple>
  </w:p>
</w:ftr>
""" % {"font": FONT}


def restyle_defaults(styles):
    """Rewrite docDefaults: explicit Tahoma, 12 pt, 1.5 spacing."""
    # rPrDefault: replace the theme-derived font (minorHAnsi -> Cambria) with
    # an explicit Tahoma at 12 pt.
    styles = re.sub(
        r"(<w:rPrDefault>\s*<w:rPr>).*?(</w:rPr>)",
        lambda m: m.group(1) + RPR_TAHOMA + m.group(2),
        styles,
        count=1,
        flags=re.S,
    )

    # pPrDefault: spacing after 6 pt, line 1.5
    styles = re.sub(
        r"(<w:pPrDefault>\s*<w:pPr>).*?(</w:pPr>)",
        r'\1<w:spacing w:after="%s" w:line="%s" w:lineRule="auto"/>\2'
        % (AFTER_TWIPS, LINE_TWIPS),
        styles,
        count=1,
        flags=re.S,
    )
    return styles


def _insert_in_seq(inner, snippet, before_names):
    """Insert snippet ahead of the first of before_names, else append.

    The property blocks in OOXML are xsd:sequence, so a child inserted at the
    front of the block is out of order unless it happens to be first in the
    sequence. LibreOffice tolerates that; Word is entitled not to.
    """
    for name in before_names:
        m = re.search(r"<w:%s[ />]" % name, inner)
        if m:
            return inner[:m.start()] + snippet + inner[m.start():]
    return inner + snippet


# CT_RPr children that follow <w:color> / <w:sz>, in schema order.
AFTER_COLOR = ("spacing", "w", "kern", "position", "sz", "szCs", "highlight",
               "u", "effect", "bdr", "shd", "fitText", "vertAlign", "rtl", "cs",
               "em", "lang", "eastAsianLayout", "specVanish", "oMath")
AFTER_SZ = ("highlight", "u", "effect", "bdr", "shd", "fitText", "vertAlign",
            "rtl", "cs", "em", "lang", "eastAsianLayout", "specVanish", "oMath")


def restyle_block(block):
    """Force Tahoma (or monospace for code) inside one <w:style> block."""
    m = re.search(r'w:styleId="([^"]+)"', block)
    style_id = m.group(1) if m else ""

    font = MONO if style_id in MONO_STYLES else FONT

    if "<w:rPr>" in block:
        # Replace any existing font declaration with an explicit one.
        block = re.sub(
            r"<w:rFonts[^>]*/>",
            '<w:rFonts w:ascii="%s" w:hAnsi="%s" w:cs="%s" w:eastAsia="%s"/>'
            % (font, font, font, font),
            block,
            count=1,
        )
        # Headings keep their own sizes; body styles get 12 pt.
        if style_id in MONO_STYLES or style_id.startswith("Heading") \
                or style_id in {"Title", "Subtitle"}:
            # Word's default headings are blue. The report is set in black, so
            # the theme accent colour is replaced with an explicit black.
            if style_id.startswith("Heading") or style_id in {"Title", "Subtitle"}:
                block = re.sub(r"<w:color[^>]*/>", "", block)
                block = re.sub(
                    r"<w:rPr>(.*?)</w:rPr>",
                    lambda m: "<w:rPr>%s</w:rPr>" % _insert_in_seq(
                        m.group(1), '<w:color w:val="000000"/>', AFTER_COLOR),
                    block,
                    count=1,
                    flags=re.S,
                )
        else:
            if "<w:sz " not in block:
                block = re.sub(
                    r"<w:rPr>(.*?)</w:rPr>",
                    lambda m: "<w:rPr>%s</w:rPr>" % _insert_in_seq(
                        m.group(1), '<w:sz w:val="%s"/>' % BODY_SZ, AFTER_SZ),
                    block,
                    count=1,
                    flags=re.S,
                )
    return block


def compact_tables(xml):
    """Set table text to 10 pt on single spacing.

    Body prose stays at 12 pt / 1.5 as specified; tables are set tighter
    because at 12 pt / 1.5 a wide table wraps into many pages and stops being
    readable as a table. This is conventional in technical reports and does not
    reduce any content.
    """
    # CT_RPr is an xsd:sequence. <w:sz> and <w:szCs> sit after the character
    # formatting and before highlight/underline and the rest, so a size inserted
    # at the front of the block (ahead of <w:rStyle> or <w:b>) is out of schema
    # order even though LibreOffice renders it.
    after_sz = ("highlight", "u", "effect", "bdr", "shd", "fitText", "vertAlign",
                "rtl", "cs", "em", "lang", "eastAsianLayout", "specVanish",
                "oMath")

    def set_size(inner, size):
        inner = re.sub(r"<w:sz [^/]*/>|<w:szCs [^/]*/>", "", inner)
        ins = '<w:sz w:val="%s"/><w:szCs w:val="%s"/>' % (size, size)
        for name in after_sz:
            m = re.search(r"<w:%s[ />]" % name, inner)
            if m:
                return inner[:m.start()] + ins + inner[m.start():]
        return inner + ins

    def fix_tbl(match):
        t = match.group(0)
        # Runs with no rPr at all.
        t = re.sub(
            r"<w:r>(?!<w:rPr>)",
            '<w:r><w:rPr><w:sz w:val="20"/><w:szCs w:val="20"/></w:rPr>',
            t,
        )
        t = t.replace("<w:rPr/>", '<w:rPr><w:sz w:val="20"/><w:szCs w:val="20"/></w:rPr>')
        # Runs that already have an rPr: place the size where the schema wants it.
        t = re.sub(
            r"<w:rPr>(.*?)</w:rPr>",
            lambda m: "<w:rPr>%s</w:rPr>" % set_size(m.group(1), "20"),
            t,
            flags=re.S,
        )
        # Single spacing inside cells, no extra space before, and 2 pt after so
        # one row is distinguishable from the next: with no row rules and no
        # gap, the row pitch equals the line pitch and the table reads as one
        # block of lines. CT_PPr is a sequence, and <w:pStyle> has to come
        # first; inserting the spacing straight after <w:pPr> would put it ahead
        # of the style reference and leave the file out of schema order.
        t = re.sub(
            r"<w:pPr>(<w:pStyle [^/]*/>)?",
            lambda m: '<w:pPr>%s<w:spacing w:before="0" w:after="40" '
                      'w:line="240" w:lineRule="auto"/>' % (m.group(1) or ""),
            t,
        )
        # Keep a row on one page. A row that splits leaves its cells stacked at
        # different heights on the two pages, which reads as overlapping text.
        # No row in this report is a page tall, so this costs nothing.
        def fix_row(m):
            row = m.group(0)
            if "<w:trPr>" in row:
                return row.replace("<w:trPr>", "<w:trPr><w:cantSplit/>", 1)
            return "<w:tr><w:trPr><w:cantSplit/></w:trPr>" + row[row.index(">") + 1:]

        t = re.sub(r"<w:tr[ >].*?</w:tr>", fix_row, t, flags=re.S)
        return t

    return re.sub(r"<w:tbl>.*?</w:tbl>", fix_tbl, xml, flags=re.S)


def restyle_styles(xml):
    xml = restyle_defaults(xml)

    def repl(match):
        return restyle_block(match.group(0))

    return re.sub(r"<w:style [^>]*>.*?</w:style>", repl, xml, flags=re.S)


def restyle_section(xml, rid_footer):
    """Give the body A4 geometry and a footer reference."""
    sect = (
        '<w:sectPr>'
        '<w:footerReference w:type="default" r:id="%s"/>' % rid_footer
        + '<w:pgSz w:w="%s" w:h="%s"/>' % (PAGE_W, PAGE_H)
        + '<w:pgMar w:top="%s" w:right="%s" w:bottom="%s" w:left="%s" '
          'w:header="%s" w:footer="%s" w:gutter="0"/>'
          % (MARGIN, MARGIN, MARGIN, MARGIN, HEADER_FOOTER, HEADER_FOOTER)
        + "</w:sectPr>"
    )
    if "<w:sectPr" in xml:
        return re.sub(r"<w:sectPr[^>]*/>|<w:sectPr[^>]*>.*?</w:sectPr>",
                      sect, xml, count=1, flags=re.S)
    # No section properties at all: append one before </w:body>.
    return xml.replace("</w:body>", sect + "</w:body>")


def add_footer_parts(content_types, rels):
    """Register footer1.xml in [Content_Types].xml and the relationships."""
    if "footer1.xml" not in content_types:
        content_types = content_types.replace(
            "</Types>",
            '<Override PartName="/word/footer1.xml" '
            'ContentType="application/vnd.openxmlformats-officedocument'
            '.wordprocessingml.footer+xml"/></Types>',
        )

    ids = [int(x) for x in re.findall(r'Id="rId(\d+)"', rels)]
    rid = "rId%d" % (max(ids) + 1 if ids else 1)

    if "footer1.xml" not in rels:
        rels = rels.replace(
            "</Relationships>",
            '<Relationship Id="%s" Type="http://schemas.openxmlformats.org'
            '/officeDocument/2006/relationships/footer" '
            'Target="footer1.xml"/></Relationships>' % rid,
        )
    return content_types, rels, rid


def process(path):
    with zipfile.ZipFile(path) as src:
        items = {name: src.read(name) for name in src.namelist()}

    styles = items["word/styles.xml"].decode("utf-8")
    document = items["word/document.xml"].decode("utf-8")
    content_types = items["[Content_Types].xml"].decode("utf-8")
    rels = items["word/_rels/document.xml.rels"].decode("utf-8")

    styles = restyle_styles(styles)
    content_types, rels, rid = add_footer_parts(content_types, rels)
    document = restyle_section(document, rid)
    document = compact_tables(document)

    # The document body uses r: for relationship ids; make sure the namespace
    # is declared, which pandoc's output already does.
    items["word/styles.xml"] = styles.encode("utf-8")
    items["word/document.xml"] = document.encode("utf-8")
    items["[Content_Types].xml"] = content_types.encode("utf-8")
    items["word/_rels/document.xml.rels"] = rels.encode("utf-8")
    items["word/footer1.xml"] = FOOTER_XML.encode("utf-8")

    tmp = path + ".tmp"
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as out:
        for name, data in items.items():
            out.writestr(name, data)
    os.replace(tmp, path)


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    for path in argv:
        if not path.endswith(".docx") or not os.path.exists(path):
            print("skip (not a .docx): %s" % path)
            continue
        process(path)
        print("styled: %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
