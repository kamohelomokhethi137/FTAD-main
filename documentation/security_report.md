# Chapter 6 — Security, Performance and Defect Analysis

> **Evidence note.** Every statement here was produced by reading the source or
> by executing a command on 4 November 2026. Where a control was *not* assessed,
> that is stated as a gap. No penetration test was performed and none is claimed.
> The full performance measurement record is in **Appendix E** and
> `Reports/performance_report.md`; this chapter states only what it establishes.

---

## 6.1 Scope and Threat Model

FTAD is a single-user desktop application with no network layer, no server
component and no multi-user access control. Stating that first prevents the
chapter from grading the application against controls it was never built to have.

**Assets:** transaction records (account identifiers, amounts, timestamps) and
investigation records (case status, assignee, reviewer notes).

**Adversaries considered:** another local user with filesystem access; anyone who
obtains the database file by backup, shared folder or stolen disk; and malicious
input supplied through CSV import.

**Out of scope:** remote network attackers (no network layer exists), physical
access to an unlocked session, and supply-chain compromise of Qt or SQLite. An
application with no network listener cannot be attacked remotely, and reporting
"no firewall" would be noise rather than a finding.

---

## 6.2 What Was Examined

| Area | Method | Result |
|---|---|---|
| SQL construction | Read all 12 `prepare()` sites; counted bindings | 36 `addBindValue`, 0 interpolated queries |
| OS command execution | Search `QProcess`, `system`, `popen`, `exec*` | None |
| Network surface | Search socket and `QNetworkAccessManager` classes | None |
| Secrets in source | Search `password`, `secret`, `api_key`, `token` | None |
| Sensitive logging | Count `qDebug`/`qWarning`/`qCritical` in backend | 0 |
| Database constraints | Read `database/schema.sql` | `CHECK`, `NOT NULL`, `UNIQUE`, `FOREIGN KEY` present |
| File permissions | Inspect the created database file | **0644 — world-readable** |
| Memory safety | ASan + UBSan, Valgrind (Chapter 5) | 0 errors |
| Input validation | Read validators; run negative tests | Rejects malformed identifiers, amounts, timestamps |

## 6.3 Findings

| ID | Finding | Severity | Evidence | Status |
|---|---|---|---|---|
| SEC-01 | Database created world-readable (0644) | **Medium** | `ls -l`; no `setPermissions` in source | Open |
| SEC-02 | No authentication or authorisation | Medium (shared) / Low (single user) | Source review | Accepted by design |
| SEC-03 | No encryption at rest; SQLite stores plaintext | **Medium** | `QSQLITE` driver, no SQLCipher | Accepted risk |
| SEC-04 | Database stored beside the executable | Low | `DatabaseManager.cpp` | Open |
| SEC-05 | No tamper detection on stored records | Low | Schema has no signature or hash chain | Accepted |
| SEC-06 | Dependency vulnerability scanning not performed | Unknown | — | **Gap, declared** |
| SEC-07 | CSV formula injection unguarded (latent) | Low | No export exists yet | Forward risk |

---

## 6.4 Detailed Findings

### 6.4.1 SEC-01 — world-readable database

```
-rw-r--r-- 1 hashcat hashcat 57344 Nov  4 01:39 ftad.db
```

Any local user can read every transaction and every investigation note. On a
single-user laptop this is inconsequential; on a shared machine it is a
confidentiality breach requiring no exploit, only `cat`.

No code sets permissions — the file inherits the process umask, 022 on most Linux
systems. The intent was clearly to keep the data private and a default silently
did not. Remediation, two lines after the connection opens:

```cpp
QFile::setPermissions(databaseFolder + "/ftad.db",
    QFile::ReadOwner | QFile::WriteOwner);
```

This is the only finding in the chapter that is a defect rather than a design
consequence.

### 6.4.2 Accepted risks and forward risks

**SEC-02, SEC-03, SEC-04, SEC-05 and SEC-07 are recorded in full in
Appendix F**, because each is a design consequence or a forward risk rather than a
defect, and the detail is consulted rather than read. In summary: there is no
authentication, no authorisation model and no encryption at rest, so whoever can
launch the application can read and modify everything and anyone holding the
database file can read it without the application; the database is resolved beside
the executable rather than under `QStandardPaths::AppDataLocation`, which fails or
requires elevation when installed system-wide; nothing detects direct edits to the
database, since the schema enforces internal consistency but consistency is not
authenticity; and CSV formula injection is unguarded, which is currently unreal
because `ReportsPage` exports nothing but will become real the moment export exists.

---

## 6.5 What Was Not Found, and Why That Is Credible

A report listing only findings invites the question of whether the absences were
tested or assumed. Each absence below was actively checked, and each is a search
that could have returned a hit.

**SQL injection — not present.** Every query uses `prepare()` with `?` placeholders
and `addBindValue()`: twelve prepared statements, 36 bind calls, and a search for
`exec(... .arg(...))` returns nothing. This is the control most often broken in Qt
applications and here it is correct throughout. **OS command injection — not
present.** No `QProcess`, `system()`, `popen()` or `exec*()` appears in `backend/`
or `frontend/`; there is no shell to inject into. **Memory safety — verified
clean.** ASan, UBSan and Valgrind report zero errors across 154 tests (§5.4), with 0
bytes definitely or indirectly lost. **Secrets in source — none.** No password, API
key, token or credential string appears in backend, frontend or schema. **Sensitive
logging — none.** The backend contains zero `qDebug`, `qWarning` or `qCritical`
calls, so transaction data cannot leak into a log, terminal or crash report through
application logging. **Network attack surface — none exists.** No socket or network
class is used; this is not a control that was implemented but an architectural
property that removes an entire class of risk. **Input validation — present and
tested.** Identifiers are normalised and rejected when malformed, amounts must be
finite and non-negative, timestamps must parse, and CSV rows with missing,
duplicated or ill-typed columns are rejected with per-row errors — verified by 16
`CsvImportTest` and 18 `TransactionRepositoryTest` cases, not asserted from reading
the code.

**SEC-06 is a genuine gap, declared as one.** No dependency vulnerability scanning
was performed: Qt 6.8.2, SQLite 3.46.1 and GoogleTest 1.16.0 were not checked
against CVE databases. Claiming "no known vulnerabilities" would require that scan,
so no such claim is made.

---

## 6.6 Performance Analysis

Measured with `ftad_benchmark` at 1,000–20,000 transactions, median of five
repetitions; the per-size table, the constructor comparison and the full scaling
discussion are in **Appendix E**.

### 6.6.1 The defect that dominated everything

Timestamps were parsed with `QDateTime::fromString` and stored as **local** time.
Constructing a local `QDateTime` resolves the system timezone per record, and at
20,000 records that dominated the entire analysis: the same date and time
constructed as `Qt::LocalTime` cost 2,901 ms, against **6 ms** for `Qt::UTC` — a
factor of roughly 500 on the constructor alone.

The fix introduced `backend/include/TransactionTime.h`, which builds the instant
explicitly in UTC. End-to-end analysis at 20,000 transactions fell from **40.50 s
to 27.6 ms**, roughly **1,466×**. This is the clearest example in the project of a
performance defect invisible in code review: the call is idiomatic, the result is
correct, and only measurement reveals what it costs.

### 6.6.2 Current baseline and its noise

At 20,000 transactions the current build analyses in **31.7–39.9 ms**, median
34.8 ms, with `build()` taking 12.87 ms. Those figures must be read as a range:
three runs of the same binary on the same data spanned roughly 25 %, because no
CPU frequency scaling was pinned and other processes were running. The report
therefore quotes the range and the median and draws no conclusion from differences
smaller than the spread; the historical 27.6 ms belongs to its own run and is not
blended with these.

Per-transaction cost stays between 1.7 and 2.0 µs across the whole range, so the
algorithm is effectively linear. The measured scaling factor at 20,000 is 23.5×
against a linear ideal of 20×; the excess is consistent with cache effects and
with the noise above, and the per-transaction column — which would rise steadily
under quadratic behaviour — does not rise.

---

## 6.7 Defect Summary

| ID | Defect | Severity | Found by | Status |
|---|---|---|---|---|
| BUG-01 | Behaviour baseline included future transactions | High | `BehaviorAnalyzer` tests | Fixed |
| BUG-02 | Investigation cases destroyed by re-analysis | High | `DetectionPipelineTest` | Fixed |
| BUG-03 | Uncommitted SQLite transaction on no-op save | High | `InvestigationTest` | Fixed |
| BUG-04 | Duplicate and malformed identifiers accepted | High | `TransactionRepositoryTest`, `CsvImportTest` | Fixed |
| BUG-05 | Sample dataset did not exercise the detection rules | Medium | Manual review + detection tests | Fixed, verified end to end |
| BUG-06 | Analysis runtime dominated by local-time construction | Medium | `ftad_benchmark` | Fixed (~1,466×) |
| LAT-01 | Implicit `int` multiplication widened to `qint64` | Low | Clang-Tidy | Open, unreachable |
| LAT-02 | `minimumHistory` read as `>=` and `>` in two methods | Low | Boundary analysis | Open, dead code |
| SEC-01 | Database file world-readable | Medium | File inspection | Open |
| SEC-03 | No encryption at rest | Medium | Driver and schema review | Accepted risk |

Six defects found and fixed; four issues open and documented with their
reachability and remedies. None of the open items affects detection correctness
in the shipped configuration.

---

## 6.8 Recommendations

Ordered by value per unit of effort. Setting the database permissions to 0600 after
creation addresses SEC-01 in two lines. Adding upper bounds to both window settings
in `validate()` and casting the window arithmetic to `qint64` address LAT-01 in five
lines between them. Making `hasBaseline()` and `profile()` agree with `evaluate()` on
`minimumHistory` is a one-comparison change and closes LAT-02. Those four together
are under ten lines and close four of the five open items. Moving the database to
`QStandardPaths::AppDataLocation` addresses SEC-04 and is a small change; prefixing
formula-leading characters on CSV export addresses SEC-07 and is small but only
becomes necessary once export exists; running dependency vulnerability scanning in
CI addresses SEC-06 and is an ongoing commitment rather than a change; and encrypting
at rest addresses SEC-03 and is a large piece of work that should be undertaken only
if the application is deployed with real financial data.

Recommendations 1–4 were not applied because doing so would have invalidated the
measurements in Chapter 5, which were taken against the current commit. Applying them
and re-running the gates is the correct next action, not a late edit to a measured
baseline.

---

## 6.9 Conclusion

The security posture is what the architecture implies: a single-user local
application with no network surface, correct SQL parameterisation, clean memory
behaviour, no secrets and no sensitive logging — and no authentication, no
encryption, and a database any local user can read.

The two findings that matter are SEC-01 and SEC-03, and they differ in kind.
SEC-01 is a defect: the intent was privacy and a default silently overrode it.
SEC-03 is a scope decision: encryption was never attempted, and the application is
honest about that only if the report is.

The performance work produced the largest measurable improvement in the project —
40.50 s to 27.6 ms — from a defect no code review would have caught. That is the
argument for measurement as a testing activity rather than an optional extra.

What this chapter does not establish is that FTAD is secure in an absolute sense.
It establishes that specific named properties were checked, that most hold, and
that the ones that do not are listed with their consequences.
