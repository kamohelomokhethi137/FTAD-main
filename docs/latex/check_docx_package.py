#!/usr/bin/env python3
"""Check a .docx package against the OPC rules and the OOXML child sequences.

Word refuses to open, or offers to repair, a package that violates any of these.
LibreOffice is far more forgiving, so a file that renders correctly in LibreOffice
can still be rejected by Word. This checks the properties that are independent of
any layout engine:

    1. the zip is intact and holds no duplicate part names
    2. every relationship target resolves to a real part
    3. every part is covered by [Content_Types].xml
    4. the children of pPr, rPr, trPr, tcPr, tblPr and sectPr appear in the order
       the schema declares (they are xsd:sequence, not xsd:all)
    5. no element appears twice inside one property block

Usage:
    python3 check_docx_package.py FILE.docx
"""

import re
import sys
import zipfile
import xml.etree.ElementTree as ET

W = "{http://schemas.openxmlformats.org/wordprocessingml/2006/main}"

# ECMA-376 Part 1, the xsd:sequence inside each property type.
SEQUENCES = {
    "pPr": ["pStyle", "keepNext", "keepLines", "pageBreakBefore", "framePr",
            "widowControl", "numPr", "suppressLineNumbers", "pBdr", "shd", "tabs",
            "suppressAutoHyphens", "kinsoku", "wordWrap", "overflowPunct",
            "topLinePunct", "autoSpaceDE", "autoSpaceDN", "bidi", "adjustRightInd",
            "snapToGrid", "spacing", "ind", "contextualSpacing", "mirrorIndents",
            "suppressOverlap", "jc", "textDirection", "textAlignment",
            "textboxTightWrap", "outlineLvl", "divId", "cnfStyle", "rPr", "sectPr",
            "pPrChange"],
    "rPr": ["rStyle", "rFonts", "b", "bCs", "i", "iCs", "caps", "smallCaps",
            "strike", "dstrike", "outline", "shadow", "emboss", "imprint",
            "noProof", "snapToGrid", "vanish", "webHidden", "color", "spacing",
            "w", "kern", "position", "sz", "szCs", "highlight", "u", "effect",
            "bdr", "shd", "fitText", "vertAlign", "rtl", "cs", "em", "lang",
            "eastAsianLayout", "specVanish", "oMath"],
    "trPr": ["cnfStyle", "divId", "gridBefore", "gridAfter", "wBefore", "wAfter",
             "cantSplit", "trHeight", "tblHeader", "tblCellSpacing", "jc",
             "hidden", "ins", "del", "trPrChange"],
    "tcPr": ["cnfStyle", "tcW", "gridSpan", "hMerge", "vMerge", "tcBorders", "shd",
             "noWrap", "tcMar", "textDirection", "tcFitText", "vAlign", "hideMark",
             "headers", "cellIns", "cellDel", "cellMerge", "tcPrChange"],
    "tblPr": ["tblStyle", "tblpPr", "tblOverlap", "bidiVisual",
              "tblStyleRowBandSize", "tblStyleColBandSize", "tblW", "jc",
              "tblCellSpacing", "tblInd", "tblBorders", "shd", "tblLayout",
              "tblCellMar", "tblLook", "tblCaption", "tblDescription",
              "tblPrChange"],
    "sectPr": ["footnotePr", "endnotePr", "type", "pgSz", "pgMar", "paperSrc",
               "pgBorders", "lnNumType", "pgNumType", "cols", "formProt", "vAlign",
               "noEndnote", "titlePg", "textDirection", "bidi", "rtlGutter",
               "docGrid", "printerSettings", "sectPrChange"],
}


def local(tag):
    return tag.split("}")[-1] if "}" in tag else tag


def check_sequences(xml_bytes, part):
    """Report children that are out of order or repeated inside one block."""
    problems = []
    root = ET.fromstring(xml_bytes)
    for block, order in SEQUENCES.items():
        index = {name: i for i, name in enumerate(order)}
        for elem in root.iter(W + block):
            seen = []
            for child in elem:
                name = local(child.tag)
                if name not in index:
                    continue
                seen.append((name, index[name]))
            # order
            for (a, ia), (b, ib) in zip(seen, seen[1:]):
                if ib < ia:
                    problems.append("%s: <%s> follows <%s>" % (block, b, a))
            # duplicates
            names = [n for n, _ in seen]
            dupes = {n for n in names if names.count(n) > 1}
            for d in sorted(dupes):
                problems.append("%s: <%s> appears %d times" % (block, d, names.count(d)))
    return problems


def main(path):
    fail = 0
    with zipfile.ZipFile(path) as z:
        bad = z.testzip()
        print("zip integrity:", "OK" if bad is None else "CORRUPT at %s" % bad)
        fail += bad is not None

        names = z.namelist()
        dupes = {n for n in names if names.count(n) > 1}
        print("duplicate parts:", dupes if dupes else "none")
        fail += bool(dupes)

        # 2. relationships resolve
        unresolved = []
        for name in names:
            if not name.endswith(".rels"):
                continue
            base = name.rsplit("_rels/", 1)[0]
            rels = ET.fromstring(z.read(name))
            for rel in rels:
                if rel.get("TargetMode") == "External":
                    continue
                target = rel.get("Target").replace("\\", "/")
                if target.startswith("/"):
                    resolved = target.lstrip("/")
                else:
                    resolved = (base + target).replace("/./", "/")
                # normalise ../ segments
                parts = []
                for seg in resolved.split("/"):
                    if seg == "..":
                        if parts:
                            parts.pop()
                    elif seg not in ("", "."):
                        parts.append(seg)
                resolved = "/".join(parts)
                if resolved not in names:
                    unresolved.append("%s -> %s" % (name, resolved))
        print("unresolved relationships:", unresolved if unresolved else "none")
        fail += bool(unresolved)

        # 3. content types cover every part
        ct = z.read("[Content_Types].xml").decode("utf-8")
        defaults = set(re.findall(r'<Default Extension="([^"]+)"', ct))
        overrides = set(re.findall(r'<Override PartName="([^"]+)"', ct))
        uncovered = []
        for name in names:
            if name == "[Content_Types].xml" or name.endswith("/"):
                continue
            if "/" + name in overrides:
                continue
            ext = name.rsplit(".", 1)[-1].lower()
            if ext in defaults:
                continue
            uncovered.append(name)
        print("parts without a content type:", uncovered if uncovered else "none")
        fail += bool(uncovered)

        # 4/5. schema sequences
        total = []
        for name in names:
            if name.startswith("word/") and name.endswith(".xml"):
                total += ["%s %s" % (name, p)
                          for p in check_sequences(z.read(name), name)]
        print("schema-order violations:", len(total))
        for t in total[:10]:
            print("   ", t)
        fail += bool(total)

    print("\nRESULT:", "PASS" if not fail else "FAIL")
    return 0 if not fail else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
