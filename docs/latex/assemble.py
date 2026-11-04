#!/usr/bin/env python3
"""Assemble the FTAD report master Markdown from the chapter sources.

The chapter files under documentation/ are also delivered as standalone
documents, so each carries its own heading and a small metadata block
("**Project:**", "**Document type:**", ...). In the bound report that metadata
would repeat nine times, and two chapters are missing a chapter number in their
heading. This script normalises both without editing the sources.

Outputs master.md in the current directory.
"""

import pathlib
import re

HERE = pathlib.Path(__file__).resolve().parent
DOC = HERE.parent.parent / "documentation"

# Chapter heading overrides where the source heading is not already numbered.
H1 = {
    "project_audit.md": "# Chapter 1 — Project Audit",
    "architecture.md": "# Chapter 3 — System Architecture",
    "references_verification.md": "# Appendix A — Reference Verification Record",
}

# Metadata lines that belong to a standalone deliverable, not the bound report.
DROP = re.compile(
    r"^\*\*(Project|Document type|Executed|Assessed|Verified|Baseline|"
    r"Companion to|Status|Evidence):\*\*"
)


def clean(text, name):
    lines = text.splitlines()

    # Normalise the first H1.
    if name in H1:
        for i, line in enumerate(lines):
            if line.startswith("# "):
                lines[i] = H1[name]
                break

    # Drop the standalone-document metadata block.
    out = [ln for ln in lines if not DROP.match(ln)]

    # Drop the decorative horizontal rules that separated sections in the
    # standalone documents. In the bound report the headings already separate
    # them, and each rule costs about 1.5 rendered lines. Table separator rows
    # (|---|---|) are left alone: only a bare "---" that is not adjacent to a
    # table row is removed.
    kept = []
    for i, ln in enumerate(out):
        if ln.strip() == "---":
            prev = next((p for p in reversed(out[:i]) if p.strip()), "")
            nxt = next((n for n in out[i + 1:] if n.strip()), "")
            if prev.lstrip().startswith("|") or nxt.lstrip().startswith("|"):
                kept.append(ln)
                continue
            continue
        kept.append(ln)
    out = kept

    # Collapse runs of blank lines left behind.
    text = "\n".join(out)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip() + "\n"


def split_literature(text):
    """Return (body, references). The verification appendix becomes 2.12."""
    marker_refs = "\n## References\n"
    marker_app = "\n## Appendix — Reference verification summary\n"

    if marker_refs not in text:
        raise SystemExit("literature_review.md: no '## References' section")

    body, rest = text.split(marker_refs, 1)
    refs = "## References\n" + rest

    if marker_app in refs:
        refs, app = refs.split(marker_app, 1)
        app = "## 2.12 Reference quality and verification\n" + app
        body = body.rstrip() + "\n\n" + app.strip() + "\n"

    return body.strip() + "\n", refs.strip() + "\n"


def read(name):
    return (DOC / name).read_text()


def main():
    front = HERE / "front"

    lit_body, lit_refs = split_literature(clean(read("literature_review.md"),
                                                "literature_review.md"))

    blocks = [
        (front / "abstract.md").read_text().strip(),
        (front / "introduction.md").read_text().strip(),
        clean(read("project_audit.md"), "project_audit.md").strip(),
        lit_body.strip(),
        clean(read("architecture.md"), "architecture.md").strip(),
        clean(read("test_plan.md"), "test_plan.md").strip(),
        clean(read("test_execution.md"), "test_execution.md").strip(),
        clean(read("security_report.md"), "security_report.md").strip(),
        clean(read("installation_manual.md"), "installation_manual.md").strip(),
        (front / "conclusion.md").read_text().strip(),
        lit_refs.strip(),
        clean(read("references_verification.md"),
              "references_verification.md").strip(),
        clean(read("appendix_b_test_design.md"),
              "appendix_b_test_design.md").strip(),
        clean(read("test_cases.md"), "test_cases.md").strip(),
        clean(read("appendix_d_tooling.md"), "appendix_d_tooling.md").strip(),
        clean(read("appendix_e_performance.md"),
              "appendix_e_performance.md").strip(),
        clean(read("appendix_f_security_detail.md"),
              "appendix_f_security_detail.md").strip(),
        clean(read("appendix_g_views.md"), "appendix_g_views.md").strip(),
    ]

    # Back matter starts on a fresh page even though chapters now flow.
    BREAK = "\n```{=latex}\n\\clearpage\n```\n\n"
    blocks = [b if not (b.startswith("## References") or
                        b.startswith("# Appendix")) else BREAK + b
              for b in blocks]

    yaml = (
        "---\n"
        'title: "Verification of FTAD: A Financial Transaction '
        'Anomaly Detector"\n'
        'author: "Bongeni Hashcat"\n'
        'date: "BIST4110 Software Testing and Reliability\\\\'
        '6 November 2026"\n'
        "---\n\n"
    )

    (HERE / "master.md").write_text(yaml + "\n\n".join(blocks) + "\n")

    words = sum(len(b.split()) for b in blocks)
    print("master.md: %d blocks, ~%d words" % (len(blocks), words))


if __name__ == "__main__":
    main()
