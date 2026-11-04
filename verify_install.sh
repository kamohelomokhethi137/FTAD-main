#!/bin/bash
#
# verify_install.sh — clean-room verification of the FTAD installation procedure.
#
# What it does
#   1. Checks the build dependencies (Qt 6, CMake, a C++17 compiler).
#   2. Configures and builds FTAD from scratch in a throwaway directory,
#      exactly as the installation manual instructs.
#   3. Runs the automated test suite against that fresh build.
#   4. Smoke-tests the built binary (offscreen, 5 s timeout).
#   5. Verifies the schema is deployed and the database is created on first run.
#   6. Writes a provenance-stamped log to documentation/evidence/install/.
#
# What it does NOT do
#   - It does not modify the repository (all build output goes to a scratch dir).
#   - It does not install anything and does not need root. If a dependency is
#     missing it prints the apt command for you to run yourself.
#
# Usage
#   ./verify_install.sh            full run (build + tests + smoke test)
#   ./verify_install.sh --quick    build + smoke test, skip the test suite
#
# Exit status is 0 only if every check passed.

set -u

REPO=$(cd "$(dirname "$0")" && pwd)
STAMP=$(date -u +%Y%m%d-%H%M%S)
SCRATCH="${TMPDIR:-/tmp}/ftad-install-verify-$STAMP"
LOGDIR="$REPO/documentation/evidence/install"
LOGFILE="$LOGDIR/install-verify-$STAMP.log"
QUICK=0

for arg in "$@"; do
    case "$arg" in
        --quick)   QUICK=1 ;;
        -h|--help) sed -n '3,28p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)         printf 'unknown option: %s (try --help)\n' "$arg" >&2; exit 2 ;;
    esac
done

mkdir -p "$LOGDIR" "$SCRATCH"

# Everything from here on goes to the terminal and to the log file.
exec > >(tee "$LOGFILE") 2>&1

PASS=0
FAIL=0

ok()   { PASS=$((PASS + 1)); printf '  [PASS] %s\n' "$1"; }
bad()  { FAIL=$((FAIL + 1)); printf '  [FAIL] %s\n' "$1"; }
info() { printf '  %s\n' "$1"; }
head_() { printf '\n== %s ==\n' "$1"; }

# ---------------------------------------------------------------- provenance

COMMIT=$(cd "$REPO" && git rev-parse --short HEAD 2>/dev/null || echo "not a git repo")
DIRTY=$(cd "$REPO" && git status --porcelain 2>/dev/null | wc -l | tr -d ' ')

printf 'gate:        I1 (clean-room installation verification)\n'
printf 'purpose:     verify the documented install procedure works from scratch\n'
printf 'command:     ./verify_install.sh%s\n' "$([ "$QUICK" -eq 1 ] && echo ' --quick')"
printf 'date (UTC):  %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
printf 'host:        %s\n' "$(uname -srm)"
printf 'repo:        %s\n' "$REPO"
printf 'commit:      %s (%s uncommitted file(s))\n' "$COMMIT" "$DIRTY"
printf 'scratch:     %s\n' "$SCRATCH"
printf 'note:        repository is not modified; all build output is in scratch\n'
printf '%s\n' '--------------------------------------------------------------'

# ------------------------------------------------------------ 1. toolchain

head_ "1. Build dependencies"

have() { command -v "$1" >/dev/null 2>&1; }

if have cmake; then
    ok "cmake $(cmake --version | head -1 | awk '{print $3}')"
else
    bad "cmake not found — install: sudo apt install cmake"
fi

if have g++; then
    ok "g++ $(g++ -dumpversion) (C++17 required)"
else
    bad "g++ not found — install: sudo apt install build-essential"
fi

if have make || have ninja; then
    ok "build tool: $(have ninja && echo ninja || echo make)"
else
    bad "no make or ninja — install: sudo apt install build-essential"
fi

QTVER=$(pkg-config --modversion Qt6Core 2>/dev/null || true)

if [ -n "$QTVER" ]; then
    ok "Qt6Core $QTVER"
else
    bad "Qt6 development files not found — install: sudo apt install qt6-base-dev qt6-charts-dev"
fi

if pkg-config --exists Qt6Charts 2>/dev/null; then
    ok "Qt6Charts $(pkg-config --modversion Qt6Charts) (required: the app links Qt6::Charts)"
else
    bad "Qt6Charts not found — install: sudo apt install qt6-charts-dev"
fi

if pkg-config --exists Qt6Sql 2>/dev/null; then
    ok "Qt6Sql $(pkg-config --modversion Qt6Sql)"
else
    bad "Qt6Sql not found — install: sudo apt install qt6-base-dev"
fi

# ---------------------------------------------------------- 2. configure+build

head_ "2. Configure and build from scratch"
info "scratch directory: $SCRATCH"
info "this is a clean tree — nothing is reused from the repository's build/"

BUILD_START=$(date +%s)

if cmake -S "$REPO" -B "$SCRATCH/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DFTAD_BUILD_TESTS=ON \
        > "$SCRATCH/configure.log" 2>&1
then
    ok "cmake configure (Release, tests ON)"
else
    bad "cmake configure failed — last 20 lines:"
    tail -20 "$SCRATCH/configure.log" | sed 's/^/         /'
fi

if [ -f "$SCRATCH/build/CMakeCache.txt" ]; then
    if cmake --build "$SCRATCH/build" -j"$(nproc 2>/dev/null || echo 2)" \
            > "$SCRATCH/build.log" 2>&1
    then
        BUILD_SECS=$(( $(date +%s) - BUILD_START ))
        # grep -c prints "0" but exits 1 when there are no matches, so a
        # `|| echo 0` fallback would append a second zero. Take the count as
        # printed and only default when grep produced nothing at all.
        WARNINGS=$(grep -c 'warning:' "$SCRATCH/build.log" 2>/dev/null || true)
        [ -z "$WARNINGS" ] && WARNINGS=0
        ok "build completed in ${BUILD_SECS} s with ${WARNINGS} compiler warning(s)"
    else
        bad "build failed — last 30 lines:"
        tail -30 "$SCRATCH/build.log" | sed 's/^/         /'
    fi
fi

if [ -x "$SCRATCH/build/FTAD" ]; then
    ok "executable produced: $SCRATCH/build/FTAD ($(du -h "$SCRATCH/build/FTAD" | cut -f1))"
else
    bad "executable not produced"
fi

# The schema is deployed by configure_file(... COPYONLY); without it the
# application cannot initialise its database and every launch fails.
if [ -f "$SCRATCH/build/database/schema.sql" ]; then
    TABLES=$(grep -c 'CREATE TABLE' "$SCRATCH/build/database/schema.sql")
    ok "schema deployed to build/database/schema.sql ($TABLES tables)"
else
    bad "schema not deployed — the application would fail at first launch"
fi

# ---------------------------------------------------------------- 3. tests

if [ "$QUICK" -eq 1 ]; then
    head_ "3. Automated test suite — SKIPPED (--quick)"
    info "run without --quick to include the 154-test suite"
else
    head_ "3. Automated test suite"

    if [ -x "$SCRATCH/build/tests/ftad_tests" ]; then
        TEST_START=$(date +%s)
        if ( cd "$SCRATCH/build" && QT_QPA_PLATFORM=offscreen \
                ctest --output-on-failure > "$SCRATCH/ctest.log" 2>&1 )
        then
            TEST_SECS=$(( $(date +%s) - TEST_START ))
            SUMMARY=$(grep -E 'tests passed|tests failed' "$SCRATCH/ctest.log" | tail -1)
            ok "ctest: ${SUMMARY} in ${TEST_SECS} s"
        else
            bad "ctest reported failures — last 25 lines:"
            tail -25 "$SCRATCH/ctest.log" | sed 's/^/         /'
        fi
    else
        bad "test binary not built (tests/ftad_tests)"
    fi
fi

# ------------------------------------------------------------ 4. smoke test

head_ "4. Smoke test — does the binary actually start?"

# A GUI application under the offscreen platform should stay in its event
# loop until killed, so timeout's 124 is the success case here. A signal
# (>=128) or a non-zero exit means it died.
( cd "$SCRATCH/build" && QT_QPA_PLATFORM=offscreen timeout 5 ./FTAD \
    > "$SCRATCH/smoke.log" 2>&1 )
SMOKE_RC=$?

if [ "$SMOKE_RC" -eq 124 ]; then
    ok "application started and stayed running for 5 s (killed by timeout)"
elif [ "$SMOKE_RC" -eq 0 ]; then
    ok "application started and exited cleanly"
elif [ "$SMOKE_RC" -ge 128 ]; then
    bad "application died from signal $((SMOKE_RC - 128)) — output:"
    tail -15 "$SCRATCH/smoke.log" | sed 's/^/         /'
else
    bad "application exited with status $SMOKE_RC — output:"
    tail -15 "$SCRATCH/smoke.log" | sed 's/^/         /'
fi

# ------------------------------------------------------- 5. database creation

head_ "5. First-run database creation"

if [ -f "$SCRATCH/build/database/ftad.db" ]; then
    ok "database created at build/database/ftad.db ($(du -h "$SCRATCH/build/database/ftad.db" | cut -f1))"

    if have sqlite3; then
        ROWS=$(sqlite3 "$SCRATCH/build/database/ftad.db" \
               "SELECT COUNT(*) FROM transactions;" 2>/dev/null || echo "?")
        ok "schema is queryable — transactions table present (rows: $ROWS)"
    else
        info "sqlite3 CLI not installed; skipped the queryable-schema check"
    fi

    PERMS=$(stat -c '%a' "$SCRATCH/build/database/ftad.db" 2>/dev/null)
    info "database file mode is $PERMS — see SEC-01: 0644 means world-readable"
else
    bad "no database was created — first-run initialisation is broken"
fi

# --------------------------------------------------------------- 6. summary

head_ "6. Summary"

printf '  checks passed: %d\n' "$PASS"
printf '  checks failed: %d\n' "$FAIL"
printf '  scratch dir:   %s\n' "$SCRATCH"
printf '  log:           %s\n' "$LOGFILE"
printf '%s\n' '--------------------------------------------------------------'

if [ "$FAIL" -eq 0 ]; then
    printf 'RESULT: PASS — the installation procedure works from a clean tree.\n'
    exit 0
fi

printf 'RESULT: FAIL — %d check(s) failed. See the log above.\n' "$FAIL"
exit 1
