#!/usr/bin/env python3
"""Post-process the generated LaTeX before compilation.

Two typesetting defects come out of pandoc that neither the preamble nor the
source markdown can fix.

1. Unbreakable identifiers. The appendix tables hold GoogleTest names such as
   `InsufficientHistoryProducesNoBehaviouralFinding` in narrow p-columns. TeX
   finds no hyphenation point inside such a token, so the cell overflows its
   column by up to 230 pt and runs into the neighbouring column. This inserts a
   discretionary hyphen (\\-) at each lower-to-upper boundary in letter runs of
   22 characters or more. The identifiers are unchanged when the text is read or
   copied, because \\- only prints a hyphen if the line actually breaks there.

2. Unbreakable URLs. Pandoc emits reference URLs as ordinary characters, so the
   longest entry ran 161 pt into the margin. Bare URLs are wrapped in \\url{},
   which supplies break points; pandoc's template already sets \\urlstyle{same},
   so the surrounding font is kept.

Verbatim bodies are excluded from both passes: a \\- or \\url inside one would
print literally and corrupt the listing.
"""

import pathlib
import re

TEX = pathlib.Path(__file__).with_name("FTAD_Report.tex")

MIN_RUN = 16

# Regions that must be left byte-for-byte alone. Control-sequence names are
# included because pandoc's template contains \DefineVerbatimEnvironment, and a
# \- anywhere inside that name is fatal.
PROTECTED = re.compile(
    r"(\\begin\{verbatim\}.*?\\end\{verbatim\}"
    r"|\\begin\{Verbatim\}.*?\\end\{Verbatim\}"
    r"|\\url\{[^{}]*\}"
    r"|\\[A-Za-z]+)",
    re.S,
)

# Verbatim bodies only, for passes that must not touch listings.
VERBATIM_ONLY = re.compile(
    r"(\\begin\{verbatim\}.*?\\end\{verbatim\}"
    r"|\\begin\{Verbatim\}.*?\\end\{Verbatim\})",
    re.S,
)

# A \texttt argument that is a path: it contains a slash and therefore has no
# break point, so a long one runs off the page (one reached 160 pt).
TT_PATH = re.compile(r"\\texttt\{([^{}]*/[^{}]*)\}")

# Bare URL not already inside \url{...} or \href{...}.
BARE_URL = re.compile(r"(?<![{\w])https?://[^\s{}\\]*[^\s{}\\.,;:)]")

# Letter runs of MIN_RUN+ characters. Control-sequence names are already
# excluded by PROTECTED: pandoc's template contains \DefineVerbatimEnvironment,
# and a \- inside that name is fatal.
LONG_RUN = re.compile(r"(?<!\\)[A-Za-z]{%d,}" % MIN_RUN)


def split_protected(text: str) -> list[str]:
    """Alternate unprotected/protected pieces; odd indices are protected."""
    return PROTECTED.split(text)


def wrap_urls(text: str) -> tuple[str, int]:
    pieces = split_protected(text)
    count = 0
    for i, piece in enumerate(pieces):
        if i % 2:
            continue
        pieces[i], n = BARE_URL.subn(lambda m: "\\url{%s}" % m.group(0), piece)
        count += n
    return "".join(pieces), count


def break_paths(text: str) -> tuple[str, int]:
    """Allow a line break after each slash inside a \\texttt path."""
    pieces = VERBATIM_ONLY.split(text)
    count = 0
    for i, piece in enumerate(pieces):
        if i % 2:
            continue
        pieces[i], n = TT_PATH.subn(
            lambda m: "\\texttt{%s}" % m.group(1).replace("/", "/\\allowbreak{}"),
            piece,
        )
        count += n
    return "".join(pieces), count


def add_breaks(match: "re.Match[str]") -> str:
    word = match.group(0)
    out = [word[0]]
    for prev, ch in zip(word, word[1:]):
        if prev.islower() and ch.isupper():
            out.append("\\-")
        out.append(ch)
    return "".join(out)


def break_identifiers(text: str) -> tuple[str, int]:
    pieces = split_protected(text)
    count = 0
    for i, piece in enumerate(pieces):
        if i % 2:
            continue
        pieces[i], n = LONG_RUN.subn(add_breaks, piece)
        count += n
    return "".join(pieces), count


def main() -> None:
    text = TEX.read_text()
    before = text.count("\\-")

    # URLs first, so the \url{...} they create is protected from pass two.
    text, urls = wrap_urls(text)
    text, paths = break_paths(text)

    # Identifiers document-wide: the narrow table columns that hold them are in
    # Chapter 7 as well as the appendices.
    text, _ = break_identifiers(text)

    added = text.count("\\-") - before

    TEX.write_text(text)
    print("fix_tables: %d break points, %d URLs wrapped, %d paths split"
          % (added, urls, paths))


if __name__ == "__main__":
    main()
