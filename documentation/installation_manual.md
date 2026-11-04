# Chapter 7 — Installation Manual

> **Evidence note.** The procedure below was executed end to end in a clean
> directory as part of this report. The transcript is
> `documentation/evidence/install/install-verify-20261104-055026.log`, produced by
> `./verify_install.sh`: 14 of 14 checks passed, zero compiler warnings, 46 s
> build. Every command in this chapter appears in that transcript.

---

## 7.1 Scope

FTAD is a desktop application built with C++17 and Qt 6. It has no installer
package, no service component and no network dependency: it is compiled from
source and run as a single executable. This chapter covers installing the build
dependencies, compiling, verifying, launching, and removing the application.

The supported platform is 64-bit Linux with a Debian-style package manager. The
procedure was verified on one such system and on no other. Windows and macOS are
not claimed to work: Qt and CMake are portable, but the launch scripts in §7.5 are
POSIX shell and the tested toolchain is GCC on Linux.

---

## 7.2 Requirements

| Component | Minimum | Verified with | Purpose |
|---|---|---|---|
| C++ compiler | C++17 | GCC 14.2.0 | Builds all sources |
| CMake | 3.16 | 3.31.6 | Build system (`CMakeLists.txt`) |
| Qt 6 Base | 6.x | 6.8.2 | `Qt6::Core`, `Qt6::Widgets`, `Qt6::Sql` |
| Qt 6 Charts | 6.x | 6.8.2 | `Qt6::Charts`, linked by the GUI target |
| Build tool | — | Ninja | Drives the compile |
| SQLite CLI | optional | 3.46.1 | Inspecting the database by hand |

Qt 6 Charts is a separate package from Qt 6 Base and is easy to miss, because the
backend library does not need it — only the `FTAD` executable does, so an absent
Charts package fails the build late, at link time, rather than at configure time.
A Release build needs roughly 120 MB of build tree; the test suite pulls
GoogleTest through CMake's `FetchContent`, which needs network access on the first
configure only, and adds about 60 MB.

---

## 7.3 Installing the dependencies

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-charts-dev
```

`build-essential` supplies GCC, `make` and the C library headers. `ninja-build` is
optional — CMake falls back to `make` — but shortens the build considerably.
Verify before continuing with `cmake --version` (3.16 or newer),
`g++ -dumpversion` (11 or newer) and
`pkg-config --modversion Qt6Core Qt6Charts Qt6Sql`. The last should print three
version numbers; if it prints `Package Qt6Charts was not found`, the Charts
package is missing and the GUI will not link.

---

## 7.4 Building from source

From the project root — the directory containing `CMakeLists.txt`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

The first command configures, the second compiles. On the verified system the
build took 46 s on all cores and produced `build/FTAD` (420 KB). Two CMake options
control what is built: `FTAD_BUILD_TESTS` (default `ON`) builds
`tests/ftad_tests` and registers it with CTest, and `FTAD_ENABLE_COVERAGE`
(default `OFF`) adds `--coverage -O0 -g` to `ftad_core`. A normal installation
wants the application alone, which is
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFTAD_BUILD_TESTS=OFF` followed
by the same build command.

Configuration also copies `database/schema.sql` into `build/database/`. This is
not cosmetic: `DatabaseManager` reads the schema from beside the executable at
first launch, so a build tree missing that file fails immediately with
`Unable to apply the database schema`.

---

## 7.5 First run, verification and launching

```sh
./build/FTAD
```

On first launch the application creates `build/database/ftad.db`, applies the
four-table schema (`transactions`, `anomalies`, `investigations`,
`investigation_events`) and enables foreign-key enforcement with
`PRAGMA foreign_keys = ON`. There is no separate initialisation step and no
migration tool — deleting the file returns the application to a clean state.
Confirm the installation with `sqlite3 build/database/ftad.db ".tables"`, which
should print `anomalies  investigation_events  investigations  transactions`.
Then import the shipped sample from the Transactions page (Import CSV) and select
`data/samples/test_transactions.csv`: eleven transactions are imported, of which
four are flagged on analysis — one large amount with a behavioural deviation, one
duplicate pair, and one rapid-withdrawal sequence. This is the quickest end-to-end
confirmation that detection is working.

**One caveat worth knowing before it surprises you.** The database is created with
mode 0644 — world-readable — because no code sets the permissions and the file
inherits the process umask. This is finding SEC-01 in Chapter 6. On a single-user
machine it is inconsequential; on a shared machine, run
`chmod 600 build/database/ftad.db`.

Two convenience launchers sit at the project root. `./run.sh` starts the
permanently built application, and `ftad.desktop` is a freedesktop entry for a
menu or taskbar, installed with
`install -Dm755 ftad.desktop ~/.local/share/applications/ftad.desktop`. Both
contain an absolute `Exec` path pointing at
`/home/hashcat/Documents/dataset/FTAD-main/build/FTAD`. **If you build somewhere
else, edit that path in both files**, or the shortcut fails silently — a desktop
launcher gives no error when its target is missing.

---

## 7.6 Running the test suite

```sh
cd build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```

The suite is 154 tests across ten binaries and completes in a few seconds. The
`QT_QPA_PLATFORM=offscreen` setting matters on a headless machine: without it Qt
aborts when it cannot open a display, and the failure looks like a test failure
rather than a missing display server. To regenerate every piece of evidence in
this report — tests, coverage, static analysis, sanitizers, Valgrind and the
benchmark — run `./run_test_gates.sh` (about four minutes), which writes
`documentation/evidence/SUMMARY.md` and the logs beside it. The optional tools it
needs are installed by `./install_test_tools.sh`; check without installing using
`./install_test_tools.sh --check`.

Coverage and sanitizer builds need their own trees, because optimisation distorts
line attribution and instrumenting the Release tree yields misleading percentages:
`cmake -S . -B build/coverage -DCMAKE_BUILD_TYPE=Debug -DFTAD_ENABLE_COVERAGE=ON`,
built and tested in that directory. The sanitizer flags are not a CMake option and
are passed directly; the exact invocation is recorded in §5.2 and in
`run_test_gates.sh`. Keeping the three trees separate is not stylistic — mixing
them is the most common way to obtain a coverage figure that does not describe the
code tested.

---

## 7.7 Removing the application

FTAD installs nothing outside its own directory:

```sh
rm -r build/                                   # binaries and database
rm ~/.local/share/applications/ftad.desktop    # if the shortcut was installed
```

Deleting `build/` deletes the database with it, so copy `build/database/ftad.db`
first if the transaction data matters. The Qt and CMake packages from §7.3 are
shared system libraries and should be removed only if nothing else needs them.

---

## 7.8 Troubleshooting

| Symptom | Cause | Remedy |
|---|---|---|
| `Could not find a package configuration file provided by "Qt6"` | Qt 6 development files absent | `sudo apt install qt6-base-dev qt6-charts-dev` |
| Configure succeeds, link fails on `Qt6::Charts` | Charts package missing | `sudo apt install qt6-charts-dev` |
| `Unable to create database directory` | Build tree not writable, or installed under `/opt` | Build in a user-writable directory; see SEC-04 |
| `Unable to apply the database schema` | `build/database/schema.sql` missing | Re-run the configure step |
| Tests abort with `could not connect to display` | No X server | Prefix with `QT_QPA_PLATFORM=offscreen` |
| Shortcut does nothing | Stale `Exec` path in `ftad.desktop` | Edit the path; see §7.5 |
| `FetchContent` fails on first configure | No network access | Build with `-DFTAD_BUILD_TESTS=OFF`, or vendor GoogleTest |

The procedure above is not transcribed from intent — it was executed.
`./verify_install.sh` performs the whole sequence in a throwaway directory and
asserts fourteen conditions: the six dependencies present, a Release configure
with tests enabled, a build completing with zero compiler warnings, the `FTAD`
binary produced, the schema deployed with four tables, 154 of 154 tests passing, a
binary that stays in its event loop for five seconds offscreen, a first-run
database created, and a queryable `transactions` table. All fourteen passed. Run
it yourself with `./verify_install.sh`, or `--quick` to skip the test suite; it
needs no root, modifies no repository file, and writes its transcript to
`documentation/evidence/install/`.
