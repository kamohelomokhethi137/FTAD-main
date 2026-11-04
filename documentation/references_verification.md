# Reference Verification Record

**Project:** FTAD — BIST4110 Software Testing & Reliability
**Verification date:** 4 November 2026
**Sources of truth:**

- the Crossref REST API (`api.crossref.org`), for journal articles and books;
- the Open Library catalogue (`openlibrary.org`), for ISBNs and classic
  textbooks;
- a direct HTTP fetch of each tool documentation URL.

## Method

For each journal article and book, the DOI (or ISBN) was submitted to the
Crossref registration record or the Open Library catalogue and the returned
metadata — title, author list, container, year, volume, issue, pages — was
transcribed into `references.bib`. Nothing was written from memory. Where a DOI
was unavailable (classic textbooks), the ISBN was verified through Open Library.

For tool documentation, each URL was fetched and required to return HTTP 200.

For ISO/IEC and IEEE standards, `iso.org` returns HTTP 403 to automated clients,
so the standards are cited by standard number only and are flagged below as
**not machine-verified**.

## Summary

| Category | Entries | Verified | Unverified |
|---|---|---|---|
| Books | 8 | 8 | 0 |
| Journal articles | 36 | 36 | 0 |
| Standards | 4 | 0 | 4 |
| Tool documentation | 4 | 4 | 0 |
| **Total** | **52** | **48** | **4** |

Academic share (books + journals): **44 / 52 = 84.6 %**.

## Books

| Key | Title | Year | DOI / ISBN | Verified via |
|---|---|---|---|---|
| myers2012art | The Art of Software Testing (3rd ed.) | 2012 | 10.1002/9781119202486 | Crossref |
| ammann2016introduction | Introduction to Software Testing (2nd ed.) | 2016 | 10.1017/9781316771273 | Crossref |
| jorgensen2013software | Software Testing: A Craftsman's Approach (4th ed.) | 2013 | 10.1201/9781439889503 | Crossref |
| laporte2017sqa | Software Quality Assurance | 2017 | 10.1002/9781119312451 | Crossref |
| forgacs2024modern | Modern Software Testing Techniques | 2024 | 10.1007/978-1-4842-9893-0 | Crossref |
| kan2003metrics | Metrics and Models in Software Quality Engineering (2nd ed.) | 2003 | 9780201729153 | Open Library |
| beizer1990software | Software Testing Techniques (2nd ed.) | 1990 | 9780442206727 | Open Library |
| pressman2020software | Software Engineering: A Practitioner's Approach (9th ed.) | 2020 | 9781259872976 | Open Library |

## Journal articles

| # | Key | Journal | Year | DOI |
|---|---|---|---|---|
| 1 | mondal2022colosseum | IEEE TSE | 2022 | 10.1109/TSE.2021.3111169 |
| 2 | liu2022automatic | IEEE TSE | 2022 | 10.1109/TSE.2020.2999884 |
| 3 | wu2020empirical | IEEE TSE | 2020 | 10.1109/TSE.2018.2852744 |
| 4 | rothermel2025prioritizing | IEEE TSE | 2025 | 10.1109/TSE.2025.3538490 |
| 5 | bertolino2021adaptive | IEEE TSE | 2021 | 10.1109/TSE.2019.2906187 |
| 6 | fatima2024flakyfix | IEEE TSE | 2024 | 10.1109/TSE.2024.3472476 |
| 7 | maurina2025babelrts | IEEE TSE | 2025 | 10.1109/TSE.2025.3554403 |
| 8 | kessel2022diversity | J. Syst. Softw. | 2022 | 10.1016/j.jss.2022.111442 |
| 9 | menendez2021diversifying | ACM TOSEM | 2021 | 10.1145/3447265 |
| 10 | greca2023state | ACM CSUR | 2023 | 10.1145/3579851 |
| 11 | bluemke2021software | ACM CSUR | 2021 | 10.1145/3442694 |
| 12 | aghamohammadi2021statement | Inf. Softw. Technol. | 2021 | 10.1016/j.infsof.2020.106426 |
| 13 | lee2020test | Inf. Softw. Technol. | 2020 | 10.1016/j.infsof.2020.106272 |
| 14 | hora2022excluding | Empir. Softw. Eng. | 2022 | 10.1007/s10664-022-10259-7 |
| 15 | derakhshanfar2022basic | Empir. Softw. Eng. | 2022 | 10.1007/s10664-022-10155-0 |
| 16 | chen2023source | IEEE TSE | 2023 | 10.1109/TSE.2022.3210580 |
| 17 | arcuri2020handling | ACM TOSEM | 2020 | 10.1145/3391533 |
| 18 | herbold2021costs | IEEE TSE | 2021 | 10.1109/TSE.2019.2957794 |
| 19 | cabral2023towards | IEEE TSE | 2023 | 10.1109/TSE.2022.3175789 |
| 20 | zhao2023systematic | ACM CSUR | 2023 | 10.1145/3567550 |
| 21 | yao2021impact | Inf. Softw. Technol. | 2021 | 10.1016/j.infsof.2021.106664 |
| 22 | stradowski2023machine | Inf. Softw. Technol. | 2023 | 10.1016/j.infsof.2022.107128 |
| 23 | jahanshahi2022sdabt | Inf. Softw. Technol. | 2022 | 10.1016/j.infsof.2022.107025 |
| 24 | eisty2022testing | Empir. Softw. Eng. | 2022 | 10.1007/s10664-022-10184-9 |
| 25 | murazvu2024survey | Software | 2024 | 10.3390/software3010001 |
| 26 | stradowski2023exploring | Inf. Softw. Technol. | 2023 | 10.1016/j.infsof.2022.107067 |
| 27 | imran2025code | Empir. Softw. Eng. | 2025 | 10.1007/s10664-025-10712-3 |
| 28 | traini2023delag | IEEE TSE | 2023 | 10.1109/TSE.2023.3266041 |
| 29 | leithner2021hydra | Inf. Softw. Technol. | 2021 | 10.1016/j.infsof.2021.106703 |
| 30 | lachkov2022vulnerability | J. Web Eng. | 2022 | 10.13052/jwe1540-9589.2178 |
| 31 | hilal2022financial | Expert Syst. Appl. | 2022 | 10.1016/j.eswa.2021.116429 |
| 32 | pourhabibi2020fraud | Decis. Support Syst. | 2020 | 10.1016/j.dss.2020.113303 |
| 33 | baesens2021data | Decis. Support Syst. | 2021 | 10.1016/j.dss.2021.113492 |
| 34 | motie2024financial | Expert Syst. Appl. | 2024 | 10.1016/j.eswa.2023.122156 |
| 35 | lin2022model | Expert Syst. Appl. | 2022 | 10.1016/j.eswa.2022.118354 |
| 36 | stefani2023insights | J. Softw. Eng. Appl. | 2023 | 10.4236/jsea.2023.1611032 |

## Tool documentation (HTTP 200 on 2026-11-04)

| Key | URL | Status |
|---|---|---|
| googletest | https://google.github.io/googletest/ | 200 |
| cppcheck | https://cppcheck.sourceforge.io/manual.pdf | 200 |
| valgrind | https://valgrind.org/docs/manual/manual.html | 200 |
| clangsanitizers | https://clang.llvm.org/docs/AddressSanitizer.html | 200 |

## Standards — NOT machine-verified

`iso.org` returns HTTP 403 to automated clients. These four entries are cited by
standard number and **must be checked manually against the university library
catalogue or the ISO/IEC web store before submission**. Their existence and
numbering are widely attested, but this record does not claim to have verified
them.

| Key | Standard |
|---|---|
| iso29119_2 | ISO/IEC/IEEE 29119-2:2021 — Software testing, Part 2: Test processes |
| iso29119_3 | ISO/IEC/IEEE 29119-3:2021 — Software testing, Part 3: Test documentation |
| iso25010 | ISO/IEC 25010:2023 — SQuaRE, Product quality model |
| ieee1012 | IEEE Std 1012-2016 — System, Software, and Hardware V&V |

## Caveats

1. **Year attribution.** Crossref's `issued` date is the registration date and
   can precede the print issue by several months for journals that publish
   online-first. Years in `references.bib` follow the Crossref record. Where an
   article appeared online in one year and in print in the next, the difference
   is at most one year and does not affect the recency analysis materially.

2. **Missing page numbers.** Four Empirical Software Engineering entries carry no
   page range because the publisher assigns article numbers rather than page
   ranges and Crossref returned none. Pages were not invented to fill the gap.

3. **Recency interpretation.** The lecturer's "three-year requirement" for
   journal references is ambiguous. This project interprets it as a preference
   for recent work and reports the actual distribution (24/36 published 2022 or
   later) rather than asserting compliance with an unclear rule.
