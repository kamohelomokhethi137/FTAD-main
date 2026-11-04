#!/usr/bin/env python3
"""Build a Tahoma whose metrics match the real font, for render measurements.

The report is set in Tahoma. This host has no real Tahoma; it has Wine's
metric-compatible substitute, which differs in one decisive way: its OS/2 table
sets USE_TYPO_METRICS (fsSelection bit 7) and declares a typographic line of
exactly 1.0 em, so LibreOffice sets "single" spacing to 12 pt and 1.5 lines to
18 pt. Every genuine Microsoft font checked on this host (Arial, Times New Roman,
Courier New, Verdana) clears that bit, so an engine reading real Tahoma takes the
line from the win metrics instead: (2049 + 423) / 2048 = 1.207 em, i.e. 14.48 pt
single and 21.73 pt at 1.5.

That difference is invisible in the document but changes pagination by 20 %, which
is enough to invalidate page numbers read from the render. This script produces a
copy of the substitute with bit 7 cleared and the family renamed, so LibreOffice
can be pointed at it for measurement while the system font and the document are
left alone.

Usage:
    python3 make_word_metrics_font.py OUTDIR

Writes OUTDIR/fonts/TahomaWin.ttf and OUTDIR/fonts.conf.
"""

import os
import pathlib
import struct
import sys

SRC = "/usr/share/fonts/truetype/wine-tahoma/tahoma.ttf"
FAMILY = "TahomaWin"
# nameIDs that carry the family, so the patched copy is addressable separately.
RENAME = {1: FAMILY, 3: FAMILY + " Regular", 4: FAMILY + " Regular",
          6: FAMILY + "-Regular", 16: FAMILY, 18: FAMILY + " Regular"}


def read_dir(d):
    n = struct.unpack(">H", d[4:6])[0]
    tables = {}
    for i in range(n):
        off = 12 + 16 * i
        tag = d[off:off + 4].decode("latin1")
        tables[tag] = struct.unpack(">II", d[off + 8:off + 16])
    return tables


def clear_typo_flag(d, tables):
    """Rewrite OS/2 in place: clear fsSelection bit 7."""
    off, length = tables["OS/2"]
    os2 = bytearray(d[off:off + length])
    fs = struct.unpack(">H", os2[62:64])[0]
    struct.pack_into(">H", os2, 62, fs & ~0x0080)
    return bytes(os2), fs


def rename_table(d, tables):
    """Rebuild the name table with the family replaced, appended at EOF."""
    off, length = tables["name"]
    name = d[off:off + length]
    fmt, count, str_off = struct.unpack(">HHH", name[:6])

    records = []
    for i in range(count):
        base = 6 + 12 * i
        pid, eid, lid, nid, ln, so = struct.unpack(">HHHHHH", name[base:base + 12])
        records.append([pid, eid, lid, nid, name[str_off + so:str_off + so + ln]])

    for rec in records:
        if rec[3] in RENAME:
            rec[4] = RENAME[rec[3]].encode("utf-16-be")

    storage = b""
    offsets = []
    for rec in records:
        offsets.append(len(storage))
        storage += rec[4]

    header = struct.pack(">HHH", fmt, len(records), 6 + 12 * len(records))
    body = b""
    for rec, so in zip(records, offsets):
        body += struct.pack(">HHHHHH", rec[0], rec[1], rec[2], rec[3],
                            len(rec[4]), so)
    return header + body + storage


def main(outdir):
    out = pathlib.Path(outdir)
    (out / "fonts").mkdir(parents=True, exist_ok=True)

    d = bytearray(pathlib.Path(SRC).read_bytes())
    tables = read_dir(d)

    os2, old_fs = clear_typo_flag(d, tables)
    new_name = rename_table(d, tables)

    # Append both tables at EOF and repoint the directory entries: offsets are
    # absolute, so appending avoids re-laying out every other table.
    def append(data):
        off = len(d)
        d.extend(data)
        return off, len(data)

    for tag, data in (("OS/2", os2), ("name", new_name)):
        off, length = append(data)
        for i in range(struct.unpack(">H", d[4:6])[0]):
            e = 12 + 16 * i
            if d[e:e + 4].decode("latin1") == tag:
                struct.pack_into(">II", d, e + 8, off, length)
                break

    dest = out / "fonts" / "TahomaWin.ttf"
    dest.write_bytes(bytes(d))

    conf = out / "fonts.conf"
    conf.write_text("""<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <dir>%s</dir>
  <include ignore_missing="yes">/etc/fonts/fonts.conf</include>
  <!-- LibreOffice enumerates families and matches by name, so an alias alone is
       not enough: the substitute has to be hidden before the corrected copy can
       answer to "Tahoma". -->
  <selectfont>
    <rejectfont>
      <glob>%s</glob>
    </rejectfont>
  </selectfont>
  <match target="pattern">
    <test qual="any" name="family"><string>Tahoma</string></test>
    <edit name="family" mode="assign" binding="same"><string>%s</string></edit>
  </match>
</fontconfig>
""" % ((out / "fonts").as_posix(), os.path.dirname(SRC) + "/*", FAMILY))

    print("wrote %s (%d bytes)" % (dest, len(d)))
    print("fsSelection 0x%04X -> 0x%04X" % (old_fs, old_fs & ~0x0080))
    print("wrote %s" % conf)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "/tmp/opencode/wm"))
