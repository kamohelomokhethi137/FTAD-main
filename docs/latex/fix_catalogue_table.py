#!/usr/bin/env python3
"""Give the Appendix C case catalogue a workable layout in Word.

The catalogue is a six-column table: identifier, objective, technique, expected
result, implementing test, status. At 10 pt on A4 that does not fit — Word broke
the Status heading ("Statu / s"), split identifiers at the hyphen ("TC- /
LARGE-001") and cut ordinary words mid-syllable ("Equivalenc / e").

This sets that one table to 9 pt and rewrites its column grid to widths taken
from the longest unbreakable token in each column. The other tables are left
alone: they are already legible and shrinking them would only cost space.

Usage:
    python3 fix_catalogue_table.py FILE.docx
"""

import re
import sys
import zipfile

# Twips across an A4 text block with 20 mm margins.
TEXT_WIDTH = 9638

# Longest unbreakable token per column, at 9 pt: "TC-LARGE-001" (Courier),
# "partitioning" (Tahoma), "duplicate_payment" (Courier), a CamelCase segment
# such as "ToleranceIsNot" (Courier), and "Status" (Tahoma).
COLUMNS = [1450, 1550, 1400, 2350, 1988, 900]

# Word pads every cell by 0.08 in on each side by default, which is a sixth of
# the Status column. Halving it buys the width the headings need.
CELL_MARGIN = 72

SMALL = "18"          # half-points: 9 pt
NORMAL = "20"         # what the styler set


def fix_table(xml):
    tables = list(re.finditer(r"<w:tbl>.*?</w:tbl>", xml, flags=re.S))
    for match in tables:
        if "TC-DUP-001" not in match.group(0):
            continue

        table = match.group(0)

        # Fixed layout, full text width.
        table = re.sub(r"<w:tblW [^/]*/>",
                       '<w:tblW w:w="%d" w:type="dxa"/>' % TEXT_WIDTH, table, count=1)
        if "<w:tblLayout" in table:
            table = re.sub(r"<w:tblLayout [^/]*/>",
                           '<w:tblLayout w:type="fixed"/>', table, count=1)
        else:
            table = table.replace("<w:tblPr>", '<w:tblPr><w:tblLayout w:type="fixed"/>', 1)

        # Tighter cell padding.
        margin = "".join('<w:%s w:w="%d" w:type="dxa"/>' % (side, CELL_MARGIN)
                         for side in ("top", "left", "bottom", "right"))
        # CT_TblPr is a sequence: tblCellMar precedes tblLook, so it cannot be
        # appended at the end of the block.
        if "<w:tblCellMar>" in table:
            table = re.sub(r"<w:tblCellMar>.*?</w:tblCellMar>",
                           "<w:tblCellMar>%s</w:tblCellMar>" % margin, table, flags=re.S)
        elif "<w:tblLook" in table:
            table = table.replace("<w:tblLook",
                                  "<w:tblCellMar>%s</w:tblCellMar><w:tblLook" % margin, 1)
        else:
            table = table.replace("</w:tblPr>",
                                  "<w:tblCellMar>%s</w:tblCellMar></w:tblPr>" % margin, 1)

        # Column grid.
        grid = "".join('<w:gridCol w:w="%d"/>' % w for w in COLUMNS)
        table = re.sub(r"<w:tblGrid>.*?</w:tblGrid>",
                       "<w:tblGrid>%s</w:tblGrid>" % grid, table, flags=re.S)

        # Cell widths, in column order within each row.
        def fix_row(row_match):
            row = row_match.group(0)
            index = [0]

            def fix_cell(cell_match):
                i = index[0]
                index[0] += 1
                cell = cell_match.group(0)
                if i < len(COLUMNS):
                    cell = re.sub(r'<w:tcW [^/]*/>',
                                  '<w:tcW w:w="%d" w:type="dxa"/>' % COLUMNS[i],
                                  cell, count=1)
                return cell

            return re.sub(r"<w:tc>.*?</w:tc>", fix_cell, row, flags=re.S)

        table = re.sub(r"<w:tr[ >].*?</w:tr>", fix_row, table, flags=re.S)

        # One point smaller, so the widths above hold.
        table = table.replace('<w:sz w:val="%s"/>' % NORMAL,
                              '<w:sz w:val="%s"/>' % SMALL)
        table = table.replace('<w:szCs w:val="%s"/>' % NORMAL,
                              '<w:szCs w:val="%s"/>' % SMALL)

        return xml[:match.start()] + table + xml[match.end():]

    raise SystemExit("fix_catalogue_table: catalogue table not found")


def main(path):
    with zipfile.ZipFile(path) as src:
        items = {n: src.read(n) for n in src.namelist()}

    doc = items["word/document.xml"].decode("utf-8")
    items["word/document.xml"] = fix_table(doc).encode("utf-8")

    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as out:
        for name, data in items.items():
            out.writestr(name, data)
    print("catalogue table: 9 pt, columns rebalanced")


if __name__ == "__main__":
    main(sys.argv[1])
