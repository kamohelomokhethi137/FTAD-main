# Appendix F — Security Findings in Detail

**Companion to:** Chapter 6, §6.4
**Status:** Reference material — excluded from the body page count

---

### F.1 SEC-02 and SEC-03 — no authentication, no encryption

There is no login, no
user accounts and no roles: whoever can launch the application can read and modify
everything, and anyone holding the database file can read it without the
application at all. For a course project this is defensible, and the brief does not
require authentication; what would not be defensible is silence about it. For real
financial data both would be blocking, and neither is an incremental fix:
authentication implies a user store, a session model and an authorisation policy on
every repository method, while encryption at rest implies SQLCipher or a
key-management story the application does not have. Both are recorded as
**accepted risks with a stated deployment constraint**: FTAD suits single-user,
single-machine use on data the operator already controls, and does not suit shared
or regulated environments.

**SEC-04 — database beside the executable.** `DatabaseManager` resolves the
database to `applicationDirPath() + "/database"`. Installed system-wide under
`/opt` or `/usr/local`, the process either cannot write there — first launch fails
for an ordinary user — or must run elevated, creating a root-owned world-readable
database. The conventional location is `QStandardPaths::AppDataLocation`, which is
per-user, platform-permissioned and survives reinstallation. This is a robustness
finding with a security consequence.

**SEC-05 — no tamper detection.** Nothing detects direct edits to the database: an
attacker with file access can change an amount, delete an anomaly or rewrite a note
with `sqlite3`, and the application presents the result as its own. The schema
enforces *internal* consistency — `amount >= 0`, `risk_score` between 0 and 100,
`risk_level` restricted to three values, foreign keys enabled — but consistency is
not authenticity, and detection requires a hash chain or signature, which is a
substantial feature and is accepted rather than recommended.

**SEC-07 — CSV formula injection.** Recorded because it is cheap to note now and
expensive to discover later. `ReportsPage` exports nothing, so the risk is currently
unreal. When export is implemented, a transaction field beginning with `=`, `+`, `-`
or `@` will be interpreted as a formula by spreadsheet software, so anyone who can
influence imported CSV content could plant a payload that executes when a reviewer
opens the report; the mitigation is to prefix such fields with a single quote on
export. The vulnerability does not exist yet, but the code path that will create it
is already planned.
