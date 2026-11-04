# Checkpoint 9 — Final Quality Assurance Record

**Project:** FTAD — Financial Transaction Anomaly Detector (C++17, Qt 6, SQLite)
**Document type:** Quality assurance record for the delivered report
**Executed:** 4 November 2026
**Baseline:** `docs/latex/FTAD_Report.pdf` (77 pages) and `docs/latex/FTAD_Report.docx` (76 pages)
**Companion to:** Chapters 1–8 and Appendices A–G

## 9.1 Purpose and Scope

This record closes the report against the submission specification. It states what
was checked, how it was checked, what the measured result was, and — with equal
care — what could not be checked on this host. Three kinds of evidence are kept
apart throughout, because they carry different weight:

* **Automated checks.** Scripts that read the OOXML package, the LaTeX log, or the
  rendered text layer and assert a property. These either pass or fail.
* **Render inspection.** The DOCX laid out by LibreOffice and examined as page
  images. This catches defects no structural check sees (overlapping rows, broken
  wrapping), but it is a *proxy* for Microsoft Word, not a substitute.
* **Reading.** The rendered text layer of every page was read for content
  correctness — stale markers, placeholder text, numbering, reference counts.

No claim is made about how the file looks in Microsoft Word. Word is not installed
on this host, and Section 9.7 records the consequences of that limitation rather
than papering over them.

## 9.2 Gates Executed

| Gate | Question asked | Method | Result |
|---|---|---|---|
| G1 | Does the PDF build without error? | Three XeLaTeX passes; `!` scan of `pass3.log` | 0 errors |
| G2 | Are cross-references resolved? | Scan for undefined reference / citation warnings | 0 undefined, no rerun requested |
| G3 | Is the body within the page budget? | Page range Introduction → Chapter 8 | 39 pages (PDF) |
| G4 | Are the typeface and spacing as specified? | `styles.xml` defaults; embedded font list | Tahoma, 12 pt, 1.5 |
| G5 | Is the footer present on every page? | `Page X of Y` search across all rendered pages | 76/76 pages (DOCX) |
| G6 | Does the contents page agree with the body? | TOC numbers compared against actual heading pages | 86/86 entries (DOCX) |
| G7 | Are all references present and distinct? | Numbered-entry extraction | 52 distinct, 1–52 |
| G8 | Are the architectural views present? | Embedded drawing count | 6 figures |
| G9 | Has any placeholder survived to the deliverable? | Search for `NOT YET VERIFIED`, `NOT EXECUTED`, `INFORMATION REQUIRED`, `TOCPLACEHOLDER` | 0 occurrences |
| G10 | Is the package well formed for Word? | Zip integrity, relationship resolution, content-type coverage, `pPr`/`rPr`/`trPr`/`tcPr`/`tblPr`/`sectPr` order | 0 violations, both files |

## 9.3 Specification Compliance

| Requirement | Target | PDF | DOCX | Status |
|---|---|---|---|---|
| Body length | 20–40 pages | 39 | 40 | Met |
| Abstract | One paragraph | 1 paragraph | 1 paragraph | Met |
| Introduction | Three paragraphs | 3 | 3 | Met |
| Conclusion | Three paragraphs | 3 | 3 | Met |
| References | ≥ 30, ≥ 80 % journals/books | 52; 44/52 = 84.6 % | 52; 84.6 % | Met |
| Citation style | IEEE numeric | `[n]` | `[n]` | Met |
| Page numbering | "Page X of Y" | footer, every page | footer, every page | Met |
| Body typeface | Tahoma 12 pt | Tahoma, embedded | Tahoma, `w:sz` 24 | Met |
| Line spacing | 1.5 | `\linespread{1.5}` | `w:line` 360 | Met |
| Installation manual | Included | Chapter 7 | Chapter 7 | Met |
| LaTeX softcopy | Buildable sources | `build.sh`, 3 passes | — | Met |
| DOCX deliverable | Native `.docx` | — | 76 pages | Met |
| Architectural views | Diagrams included | 6 (Appendix G) | 6 (Appendix G) | Met |

The reference split behind the 84.6 % figure is 8 books and 36 journal articles,
with 4 standards and 4 tool-documentation entries making up the remainder. All 48
books, articles and tool pages were verified against Crossref, Open Library or the
publisher's own record; the 4 standards are cited by number only and are marked
unverified in Appendix A because `iso.org` refuses automated retrieval.

## 9.4 PDF Edition

| Property | Measured |
|---|---|
| Pages | 77 total; body 39 (Introduction → Chapter 8), 40 including the abstract |
| Build errors | 0 |
| Overfull boxes | 39, largest 30.8 pt |
| Undefined references | 0 |
| Undefined citations | 0 |
| Rerun warnings | 0 |
| Embedded typefaces | Tahoma (regular, bold), Courier New, DejaVu Serif fallback |
| Figures | 6, Appendix G, vector |
| References | 52 |

An overfull box means a line exceeded its measure by up to 30.8 pt — under a
twelfth of the text width — and is the residual cost of setting long identifiers
and URLs without hyphenating them mid-token. No text is clipped; the excess
projects into the margin. This was accepted rather than forced, because the
alternative (aggressive hyphenation) breaks identifiers that a reader may need to
copy.

## 9.5 DOCX Edition

| Property | Measured |
|---|---|
| Pages | 76 total; body 40 (Introduction → References), 41 including the abstract |
| Body font and size | Tahoma, `w:sz` 24 (12 pt) |
| Line spacing | `w:line` 360 (1.5) |
| Page geometry | A4, 20 mm margins — matched to the PDF so pagination is comparable |
| Footer | "Page X of Y" on 76 of 76 pages, live `PAGE`/`NUMPAGES` fields |
| Contents | 86 entries, real paragraphs with dot leaders |
| Figures | 6, 300 dpi PNG, trimmed, scaled to the widths the source requests |
| References | 52 distinct, 1–52 |
| Stale markers | 0 |
| Rows held on one page | 382 (`w:cantSplit`) |

The DOCX is generated from the same chapter sources as the PDF, not converted from
it. Pandoc's docx writer produces the text; a styling pass rewrites the package so
the defaults are Tahoma 12 pt on 1.5 spacing with A4 geometry; a contents pass
writes the entries as ordinary paragraphs and re-derives their page numbers from
the render until they stop moving (three rounds).

A Word table-of-contents *field* was rejected deliberately. LibreOffice renders
such a field as an empty page, and the field is empty until the reader updates it,
so the delivered file would show a blank contents page to anyone opening it in
anything other than Word. The static entries render correctly everywhere. Section
9.7 states the cost of that choice.

## 9.6 Defects Found in the Deliverables and Their Disposition

Every defect below was found during this QA pass, in the delivered artefacts
rather than in the application, and every one is closed. Each was re-checked in a
fresh render after the fix.

| ID | Symptom | Cause | Disposition |
|---|---|---|---|
| QA-01 | Contents page rendered blank | LibreOffice ignores `<w:updateFields>`; a TOC field has no cached result | Contents written as real paragraphs (`make_toc.py`) |
| QA-02 | Title page ran into the abstract | No page break after the title block | Explicit page break injected |
| QA-03 | Headings printed in Word's blue accent | Theme colour inherited from pandoc's reference document | Heading and title styles forced to black |
| QA-04 | Every figure 5 in wide regardless of source | Pandoc drops `{width=…%}` and sizes from pixels at 96 dpi | Extents rewritten from the markdown percentages |
| QA-05 | Figures sat inside a large white border | Graphviz page margins rasterised along with the drawing | `-trim` applied after rasterisation |
| QA-06 | Catalogue names broke mid-word: `DuplicatePayment` split before `FlagsBothSides` | Word has no break opportunity inside an identifier | Zero-width space at CamelCase boundaries in table rows |
| QA-07 | `Status` split as `Statu`/`s`; IDs split at the hyphen | Six columns would not fit at 10 pt with default cell padding | Catalogue set to 9 pt, fixed layout, halved cell margins, rebalanced grid |
| QA-08 | `duplicate_payment` split one character from the end | Expected-result column ~30 twips narrower than the token | Column widened by 200 twips, taken from Objective and Test |
| QA-09 | Row content stacked at two heights across a page break | A table row was allowed to split between pages | `w:cantSplit` on all 382 rows |
| QA-10 | `pPr` had `<w:spacing>` before `<w:pStyle>` | The styling pass inserted spacing immediately after `<w:pPr>` | Insertion moved after the optional `pStyle` |
| QA-11 | Adjacent rows were indistinguishable | Row pitch equalled line pitch: no rules, no padding | 2 pt after each cell paragraph; rows now separate at 12 pt pitch |
| QA-12 | 419 `rPr` blocks had `<w:sz>` ahead of `<w:rStyle>` or `<w:b>` | Table size inserted at the front of the block | Inserted at the schema position, ahead of `highlight`/`u` |
| QA-13 | `<w:tblCellMar>` followed `<w:tblLook>` | Appended to the end of `tblPr` | Moved ahead of `tblLook` |
| QA-14 | Footer `pPr` had `<w:jc>` before `<w:spacing>` | Hand-written footer template | Order corrected |
| QA-15 | `behaviour_​deviation` split before its final `n` in three Expected-result cells | The column fits 18 of the token's 19 characters at 9 pt Courier | Column widened 2150 → 2350 twips, taken from Objective and Test |

QA-15 came after those: a final page-by-page inspection at the new pagination
found the 19-character `behaviour_deviation` one character short of the
Expected-result column, the same failure as QA-08 with a longer token.

QA-12 to QA-14 were found by the package check in G10, which was written for this
pass: the earlier scan covered `pPr` only, so the same class of ordering error in
`rPr`, `tblPr` and the footer went unseen. Vanilla pandoc output scores zero
violations, so all three were introduced by the styling pass, not inherited.

QA-09 and QA-11 are worth separating, because the first masked the second. The
original report of "overlapping rows" on page 54 was a row split across a page
boundary; fixing that exposed that the remaining rows were merely contiguous, with
no visual break between them. Measuring baseline positions rather than judging by
eye distinguished the two: a split row puts two words at the same `y`, whereas
contiguous rows put them at successive `y` values one line pitch apart.

## 9.7 Residual Limitations

These are stated as facts about what was and was not verified, not as defects to
be fixed later.

**No Microsoft Word on this host.** The DOCX was verified by laying it out with
LibreOffice, inspecting page images, and reading the OOXML directly. Word's own
layout engine was not observed. The package was checked against the OPC rules and
the OOXML child sequences Word enforces — zip integrity, relationship resolution,
content-type coverage, and the `xsd:sequence` order of `pPr`, `rPr`, `trPr`,
`tcPr`, `tblPr` and `sectPr` — with no violations. That is evidence the file is
well formed; it is not evidence about how Word lays it out.

**The contents page numbers were read from a layout matched to Word's metrics.**
This was the one defect this pass actually had to fix, rather than record. The
first renders put the body on an 18.00 pt pitch, exactly 12 pt x 1.5, because
`wine-tahoma` declares `USE_TYPO_METRICS` and a typographic line of 1.0 em. Every
genuine Microsoft font on this host — Arial, Times New Roman, Courier New and
Verdana, Tahoma's sibling — instead carries `fsSelection` 0x0040 without that
flag, so an engine reading real Tahoma takes the single line from the win metrics
(1.207 em) and sets 1.5 spacing at 21.73 pt, within 0.6 % of the 21.60 pt measured
in the PDF. Numbers read from the 18 pt layout would therefore have been wrong in
Word by several pages.

The build now generates a copy of the substitute with that flag cleared, hides the
original from fontconfig, and renders through the corrected font when it reads the
page numbers back. The measured pitch of that render is 21.8 pt, so the contents
numbers describe Word's pagination rather than this host's substitute. The `.docx`
itself is untouched: it still declares Tahoma at `w:line` 360 with `lineRule`
"auto", i.e. 1.5 lines, which is what Word expects.

One consequence is recorded rather than hidden: opened in a LibreOffice that still
has the Wine substitute, the same file lays out at 18 pt and the contents numbers
would not match. That is a property of the substitute font, not of the file, and it
does not affect Word or any host with real Tahoma.

**One glyph is substituted in the render.** `wine-tahoma` has no U+2192, so the
eleven arrows in Tahoma runs are drawn from DejaVu Serif. Whether real Tahoma
carries U+2192 was not established, and the genuine Microsoft fonts on this host
disagree: Arial, Times New Roman and Courier New have the glyph, while Verdana —
designed by the same hand in the same period — does not. Word performs automatic
glyph fallback, so the arrows display either way; only the typeface they are drawn
in is not guaranteed. The runs specify Tahoma and request no fallback, so this is
a font-coverage matter rather than a defect in the file.

**The title page is unnumbered.** The printed total therefore trails the physical
page count by one in the PDF — "Page 76 of 76" sits on physical page 77. This is
conventional for a title page and was kept.

**Tables are set smaller than body text.** Body prose is 12 pt on 1.5 spacing as
specified; tables are 10 pt on single spacing, and the case catalogue 9 pt, because
a six-column table at body size wraps into several pages and stops functioning as
a table. No content is omitted to achieve this.

## 9.8 Traceability to Checkpoints

| Checkpoint | Deliverable | Verified by |
|---|---|---|
| 1 — Project audit | `project_audit.md` | SLOC and feature inventory cross-checked against the tree |
| 2 — Architecture and diagrams | `architecture.md`, 6 views | Views rendered from Graphviz sources; embedded in Appendix G |
| 3 — Literature and references | `literature_review.md`, `references.bib`, Appendix A | 48 of 52 verified against Crossref / Open Library / publisher |
| 4 — Test plan | `test_plan.md`, Appendix B | Levels, techniques and types kept distinct; traceability to FR/NFR |
| 5 — Test cases and execution | `test_cases.md`, `test_execution.md` | 154/154 tests pass; coverage measured, not estimated |
| 6 — Bugs, performance, security | `security_report.md`, Appendices E and F | Defects recorded with status; timings reported as a range |
| 7 — Installation manual | `installation_manual.md` | `verify_install.sh`, 14/14 checks |
| 8 — Report | `FTAD_Report.pdf`, LaTeX sources | Gates G1–G4, G7–G10 |
| 9 — Final QA | This record, `FTAD_Report.docx` | Gates G5–G10 |

## 9.9 Conclusion

The report meets the specification on every countable requirement: 39 body pages
in the PDF and 40 in the DOCX, against a 20–40 page budget; 52 references of which 84.6 % are books or journal
articles, one-paragraph abstract, three-paragraph introduction and conclusion,
"Page X of Y" throughout, Tahoma at 12 pt on 1.5 spacing, an installation manual,
and buildable LaTeX sources. Both editions were produced from one set of chapter
sources, so the two cannot disagree about content.

Fifteen defects were found in the deliverables during this pass and all fifteen
are closed, each re-checked in a fresh render. They fell into two families. The
first was invisible to structural checks — a table row splitting across a page
break, rows with no visual separation, and three identifier tokens one character
short of their column — and was found by looking at rendered pages, then diagnosed
by measuring baseline positions rather than by eye. The
second was invisible to render inspection, because LibreOffice tolerates property
blocks that Word is entitled to reject, and was found only by validating the
package against the OPC rules and the schema sequences.

Two limitations remain open and are recorded rather than resolved. Word's own
layout was never observed: the pagination was matched to Word's metrics by
measurement, not by running Word. And one arrow glyph may be drawn from a fallback
font in either engine, because Tahoma's coverage of U+2192 could not be
established. Neither affects the correctness of the content, and neither is
claimed as verified.
