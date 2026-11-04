#!/usr/bin/env python3
"""Exit 0 when two TOC files carry the same page numbers.

Used by build_docx.sh to decide whether the contents page has settled: the
page numbers injected in one round are compared with the pages the render
actually used.
"""

import json
import sys

a = [h.get("page") for h in json.load(open(sys.argv[1]))]
b = [h.get("page") for h in json.load(open(sys.argv[2]))]

moved = [(x, y) for x, y in zip(a, b) if x != y]
print("  %d entries, %d moved" % (len(b), len(moved)))
for x, y in moved[:4]:
    print("    %s -> %s" % (x, y))

sys.exit(0 if a == b else 1)
