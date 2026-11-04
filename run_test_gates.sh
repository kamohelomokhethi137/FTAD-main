#!/usr/bin/env bash
#
# run_test_gates.sh
#
# Executes every gate defined in Chapter 4 of the FTAD report and captures the
# raw output into documentation/evidence/. This is the script that turns
# "[NOT YET VERIFIED]" into a captured log.
#
# It does not modify application source. It does create build trees
# (build/coverage, build/sanitize) and writes only inside documentation/evidence/.
#
# Gates
#   G1  ctest                  exit criterion X1
#   G2  gcov + lcov            exit criterion X5
#   G3  cppcheck               exit criterion X4
#   G4  clang-tidy             exit criterion X4
#   G5  ASan + UBSan           exit criterion X3
#   G6  valgrind memcheck      exit criterion X3
#   G7  benchmark              exit criterion X8
#   G8  functional: sample     gate F1 (all four detection rules fire)
#   G9  clean-room install     gate I1 (verify_install.sh)
#
# Usage
#   ./run_test_gates.sh              run every gate
#   ./run_test_gates.sh G1 G5        run selected gates only
#   ./run_test_gates.sh --list       list gates and exit
#
# Each gate is independent: a failing gate does not stop the others. A summary
# table is written to documentation/evidence/SUMMARY.md and printed at the end.
#
# Prerequisites: a configured build tree at ./build (Release, tests ON).
# Gates G2, G3, G4, G5 additionally need lcov, cppcheck, clang-tidy and the
# sanitizer runtime; if a tool is absent the gate is recorded as SKIPPED with
# the reason, never as PASS.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

EVIDENCE="$ROOT/documentation/evidence"
STAMP="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
DAY="$(date -u +%Y%m%d)"
JOBS="$(nproc 2>/dev/null || echo 2)"

GATE_ID=()
GATE_STATUS=()
GATE_NOTE=()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
say()
{
    printf '%s\n' "$*"
}

rule()
{
    say "--------------------------------------------------------------"
}

have()
{
    command -v "$1" >/dev/null 2>&1
}

record()
{
    GATE_ID+=("$1")
    GATE_STATUS+=("$2")
    GATE_NOTE+=("$3")
}

# Writes the provenance header that precedes every captured log, so that a
# reader can tell exactly which command, tool version and date produced it.
prologue()
{
    local gate="$1"
    local purpose="$2"
    local command="$3"

    echo "gate:        $gate"
    echo "purpose:     $purpose"
    echo "command:     $command"
    echo "date (UTC):  $STAMP"
    echo "host:        $(uname -srm)"
    echo "cwd:         $ROOT"
    echo "--------------------------------------------------------------"
}

ensure_build()
{
    if [ ! -f "$ROOT/build/CMakeCache.txt" ]; then
        say "ERROR: no configured build tree at ./build"
        say "Run: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFTAD_BUILD_TESTS=ON"
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# G1 - ctest
# ---------------------------------------------------------------------------
gate_g1()
{
    local out="$EVIDENCE/ctest/ctest-$DAY.log"
    local rc

    say ""
    say "=== G1  ctest (X1) ==="

    {
        prologue "G1" "X1 - all automated tests pass" \
                 "cd build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure"
        ( cd "$ROOT/build" && QT_QPA_PLATFORM=offscreen ctest --output-on-failure ) 2>&1
    } | tee "$out"

    rc=${PIPESTATUS[0]}

    if [ "$rc" -eq 0 ]; then
        record "G1" "PASS" "ctest/ctest-$DAY.log"
    else
        record "G1" "FAIL" "ctest/ctest-$DAY.log (exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# G2 - coverage
# ---------------------------------------------------------------------------
gate_g2()
{
    local out="$EVIDENCE/coverage/lcov-$DAY.log"
    local raw="$EVIDENCE/coverage/coverage.info"
    local html="$EVIDENCE/coverage/html"
    local rc=0

    say ""
    say "=== G2  gcov + lcov (X5) ==="

    if ! have lcov; then
        say "lcov not installed - gate SKIPPED"
        {
            prologue "G2" "X5 - line coverage" "lcov --capture"
            echo "SKIPPED: lcov is not on PATH."
            echo "Install with: sudo apt-get install -y lcov"
        } > "$out"
        record "G2" "SKIPPED" "lcov not installed"
        return
    fi

    {
        prologue "G2" "X5 - line coverage of ftad_core" \
                 "cmake --fresh -B build/coverage -DFTAD_ENABLE_COVERAGE=ON ... && ctest && lcov --capture"
        echo "tool: $(lcov --version 2>&1 | head -1)"
        echo "gcov: $(gcov --version 2>&1 | head -1)"
        echo ""
        echo ">>> configure (unoptimised, --coverage)"
        cmake --fresh -S "$ROOT" -B "$ROOT/build/coverage" -G Ninja \
              -DCMAKE_BUILD_TYPE=Debug \
              -DFTAD_BUILD_TESTS=ON \
              -DFTAD_ENABLE_COVERAGE=ON 2>&1
        echo ""
        echo ">>> build"
        cmake --build "$ROOT/build/coverage" -j "$JOBS" 2>&1
        echo ""
        echo ">>> execute the suite to produce .gcda"
        ( cd "$ROOT/build/coverage" && QT_QPA_PLATFORM=offscreen ctest --output-on-failure ) 2>&1
        echo ""
        echo ">>> lcov --capture"
        lcov --capture \
             --directory "$ROOT/build/coverage" \
             --output-file "$raw" \
             --include "$ROOT/backend/*" \
             --rc branch_coverage=1 \
             --ignore-errors source,gcov,mismatch,negative,unused,empty 2>&1
        echo ""
        echo ">>> lcov summary"
        lcov --summary "$raw" \
             --rc branch_coverage=1 \
             --ignore-errors source,gcov,mismatch,negative,unused,empty 2>&1
        echo ""
        echo ">>> genhtml"
        genhtml "$raw" --output-directory "$html" \
                --rc branch_coverage=1 --rc genhtml_branch_coverage=1 \
                --ignore-errors source,gcov,mismatch,negative,unused,empty 2>&1
    } | tee "$out"

    rc=${PIPESTATUS[0]}

    if [ -f "$raw" ]; then
        record "G2" "PASS" "coverage/lcov-$DAY.log + coverage/html/"
    else
        record "G2" "FAIL" "no coverage.info produced (exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# G3 - cppcheck
# ---------------------------------------------------------------------------
gate_g3()
{
    local out="$EVIDENCE/static/cppcheck-$DAY.log"

    say ""
    say "=== G3  cppcheck (X4) ==="

    if ! have cppcheck; then
        say "cppcheck not installed - gate SKIPPED"
        {
            prologue "G3" "X4 - static analysis" "cppcheck --project=build/compile_commands.json"
            echo "SKIPPED: cppcheck is not on PATH."
            echo "Install with: sudo apt-get install -y cppcheck"
        } > "$out"
        record "G3" "SKIPPED" "cppcheck not installed"
        return
    fi

    # compile_commands.json is required for accurate include resolution.
    if [ ! -f "$ROOT/build/compile_commands.json" ]; then
        say "generating compile_commands.json ..."
        cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null 2>&1
    fi

    local appout="$EVIDENCE/static/cppcheck-app-$DAY.log"

    # Two passes, because they answer different questions.
    #
    #   PASS 1  application source only. This is the figure the report quotes:
    #           it is the code FTAD owns and must fix.
    #   PASS 2  the whole tree, including the test suites and the vendored
    #           GoogleTest that FetchContent places in build/_deps. Informational
    #           only - findings in third-party code are not FTAD's to remediate,
    #           and folding them into the headline number would misrepresent the
    #           quality of the application.
    cppcheck \
        --project="$ROOT/build/compile_commands.json" \
        --enable=warning,style,performance,portability \
        --check-level=exhaustive \
        --inline-suppr \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        --suppress=normalCheckLevelMaxBranches \
        --std=c++17 \
        --template='{file}:{line}: {severity}: {id}: {message}' \
        -i "$ROOT/build/_deps" \
        -i "$ROOT/tests" \
        > "$appout" 2>&1

    {
        prologue "G3" "X4 - static analysis" \
                 "cppcheck --project=build/compile_commands.json --enable=warning,style,performance,portability --check-level=exhaustive"
        echo "tool: $(cppcheck --version 2>&1 | head -1)"
        echo ""
        echo ">>> PASS 1 - application source only (backend/src + frontend/src)"
        echo "    Excludes tests/ and build/_deps (vendored GoogleTest)."
        echo "    This is the figure reported in the test execution report."
        echo ""
        sed "s|$ROOT/||" "$appout"
        echo ""
        echo ">>> PASS 2 - whole tree, including tests and vendored GoogleTest"
        echo "    Informational only. Third-party findings are not FTAD's to fix."
        echo ""
        cppcheck \
            --project="$ROOT/build/compile_commands.json" \
            --enable=warning,style,performance,portability \
            --check-level=exhaustive \
            --inline-suppr \
            --suppress=missingIncludeSystem \
            --suppress=unusedFunction \
            --suppress=normalCheckLevelMaxBranches \
            --std=c++17 \
            --template='{file}:{line}: {severity}: {id}: {message}' \
            2>&1 | sed "s|$ROOT/||" | grep -vE '^Checking|files checked|^$'
    } | tee "$out"

    local findings
    findings=$(grep -cE ':[0-9]+: (error|warning|style|performance|portability):' "$appout" 2>/dev/null || true)
    record "G3" "PASS" "static/cppcheck-$DAY.log (app scope: $findings finding(s))"
}

# ---------------------------------------------------------------------------
# G4 - clang-tidy
# ---------------------------------------------------------------------------
gate_g4()
{
    local out="$EVIDENCE/static/clang-tidy-$DAY.log"

    say ""
    say "=== G4  clang-tidy (X4) ==="

    if ! have clang-tidy; then
        say "clang-tidy not installed - gate SKIPPED"
        {
            prologue "G4" "X4 - static analysis" "clang-tidy -p build backend/src/*.cpp"
            echo "SKIPPED: clang-tidy is not on PATH."
            echo "Install with: sudo apt-get install -y clang-tidy"
        } > "$out"
        record "G4" "SKIPPED" "clang-tidy not installed"
        return
    fi

    if [ ! -f "$ROOT/build/compile_commands.json" ]; then
        say "generating compile_commands.json ..."
        cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null 2>&1
    fi

    {
        prologue "G4" "X4 - static analysis and code quality" \
                 "clang-tidy -p build backend/src/*.cpp --checks=-*,bugprone-*,clang-analyzer-*,performance-*,portability-*"
        echo "tool: $(clang-tidy --version 2>&1 | grep -m1 version)"
        echo ""
        for src in "$ROOT"/backend/src/*.cpp; do
            echo ">>> $(basename "$src")"
            clang-tidy -p "$ROOT/build" "$src" \
                --checks='-*,bugprone-*,clang-analyzer-*,performance-*,portability-*' \
                --header-filter='.*backend/include/.*' \
                --quiet 2>&1
            echo ""
        done
    } | tee "$out"

    local findings
    findings=$(grep -cE 'warning:|error:' "$out" 2>/dev/null || true)
    record "G4" "PASS" "static/clang-tidy-$DAY.log ($findings diagnostic(s))"
}

# ---------------------------------------------------------------------------
# G5 - ASan + UBSan
# ---------------------------------------------------------------------------
gate_g5()
{
    local out="$EVIDENCE/dynamic/asan-ubsan-$DAY.log"

    say ""
    say "=== G5  AddressSanitizer + UBSan (X3) ==="

    {
        prologue "G5" "X3 - memory safety and undefined behaviour" \
                 "cmake --fresh -B build/sanitize -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined ...' && ctest"
        echo "compiler: $(g++ --version 2>&1 | head -1)"
        echo ""
        echo ">>> configure"
        cmake --fresh -S "$ROOT" -B "$ROOT/build/sanitize" -G Ninja \
              -DCMAKE_BUILD_TYPE=Debug \
              -DFTAD_BUILD_TESTS=ON \
              -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
              -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" 2>&1
        echo ""
        echo ">>> build"
        cmake --build "$ROOT/build/sanitize" -j "$JOBS" 2>&1
        echo ""
        echo ">>> execute the suite under the sanitizers"
        # halt_on_error=1 makes a sanitizer finding fail the run rather than
        # being buried in the log. Leak detection stays on.
        (
            cd "$ROOT/build/sanitize" &&
            QT_QPA_PLATFORM=offscreen \
            ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=0 \
            UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
            ctest --output-on-failure
        ) 2>&1
    } | tee "$out"

    local rc=${PIPESTATUS[0]}
    local hits
    hits=$(grep -cE 'ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:' "$out" 2>/dev/null || true)

    if [ "$rc" -eq 0 ] && [ "$hits" -eq 0 ]; then
        record "G5" "PASS" "dynamic/asan-ubsan-$DAY.log (0 findings)"
    else
        record "G5" "FAIL" "dynamic/asan-ubsan-$DAY.log ($hits finding(s), exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# G6 - valgrind
# ---------------------------------------------------------------------------
gate_g6()
{
    local out="$EVIDENCE/dynamic/valgrind-$DAY.log"

    say ""
    say "=== G6  valgrind memcheck (X3) ==="

    if ! have valgrind; then
        say "valgrind not installed - gate SKIPPED"
        {
            prologue "G6" "X3 - dynamic memory analysis" "valgrind --leak-check=full ./ftad_tests"
            echo "SKIPPED: valgrind is not on PATH."
            echo "Install with: sudo apt-get install -y valgrind"
        } > "$out"
        record "G6" "SKIPPED" "valgrind not installed"
        return
    fi

    if [ ! -x "$ROOT/build/tests/ftad_tests" ]; then
        say "test binary missing - gate SKIPPED"
        record "G6" "SKIPPED" "build/tests/ftad_tests not built"
        return
    fi

    {
        prologue "G6" "X3 - dynamic memory analysis" \
                 "valgrind --leak-check=full --error-exitcode=99 ./ftad_tests"
        echo "tool: $(valgrind --version 2>&1 | head -1)"
        echo ""
        (
            cd "$ROOT/build/tests" &&
            QT_QPA_PLATFORM=offscreen \
            valgrind \
                --leak-check=full \
                --show-leak-kinds=definite,indirect \
                --errors-for-leak-kinds=definite,indirect \
                --track-origins=yes \
                --error-exitcode=99 \
                --num-callers=25 \
                ./ftad_tests
        ) 2>&1
    } | tee "$out"

    local rc=${PIPESTATUS[0]}
    local errs
    errs=$(grep -oE 'ERROR SUMMARY: [0-9]+' "$out" | tail -1 | grep -oE '[0-9]+' || echo "?")

    if [ "$rc" -eq 0 ]; then
        record "G6" "PASS" "dynamic/valgrind-$DAY.log ($errs error(s))"
    else
        record "G6" "FAIL" "dynamic/valgrind-$DAY.log ($errs error(s), exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# G7 - benchmark
# ---------------------------------------------------------------------------
gate_g7()
{
    local out="$EVIDENCE/performance/benchmark-$DAY.log"

    say ""
    say "=== G7  benchmark (X8) ==="

    if [ ! -x "$ROOT/build/tests/ftad_benchmark" ]; then
        say "benchmark binary missing - gate SKIPPED"
        record "G7" "SKIPPED" "build/tests/ftad_benchmark not built"
        return
    fi

    {
        prologue "G7" "X8 - performance baseline and scaling" \
                 "build/tests/ftad_benchmark (5 repetitions, median reported)"
        echo "build type: Release (-O2)"
        echo ""
        ( cd "$ROOT/build/tests" && ./ftad_benchmark ) 2>&1
    } | tee "$out"

    local rc=${PIPESTATUS[0]}

    if [ "$rc" -eq 0 ]; then
        record "G7" "PASS" "performance/benchmark-$DAY.log"
    else
        record "G7" "FAIL" "performance/benchmark-$DAY.log (exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# G8 - functional verification of the shipped sample dataset
# ---------------------------------------------------------------------------
gate_g8()
{
    local out="$EVIDENCE/functional/sample-dataset-$DAY.log"
    local harness="$EVIDENCE/functional/verify_sample.cpp"
    local scratch="$ROOT/build/functional-gate"
    local rc

    say ""
    say "=== G8  functional: sample dataset (F1) ==="

    if [ ! -f "$harness" ]; then
        say "harness source missing - gate SKIPPED"
        record "G8" "SKIPPED" "functional/verify_sample.cpp not found"
        return
    fi

    if [ ! -f "$ROOT/build/libftad_core.a" ]; then
        say "libftad_core.a missing - gate SKIPPED (configure and build first)"
        record "G8" "SKIPPED" "build/libftad_core.a not built"
        return
    fi

    # The harness links the production library and runs in its own directory
    # with its own database, so the repository's database is never touched.
    mkdir -p "$scratch/database"

    if ! cp "$ROOT/database/schema.sql" "$scratch/database/schema.sql" 2>/dev/null; then
        say "cannot stage schema.sql - gate SKIPPED"
        record "G8" "SKIPPED" "schema.sql could not be staged"
        return
    fi

    cp "$ROOT/data/samples/test_transactions.csv" "$scratch/test_transactions.csv"

    if ! g++ -std=c++17 -fPIC "$harness" \
            -I"$ROOT/backend/include" \
            $(pkg-config --cflags Qt6Core Qt6Sql) \
            "$ROOT/build/libftad_core.a" \
            $(pkg-config --libs Qt6Core Qt6Sql) \
            -o "$scratch/verify_sample" > "$scratch/build.log" 2>&1
    then
        say "harness failed to compile - see $scratch/build.log"
        record "G8" "FAIL" "functional harness did not compile"
        return
    fi

    {
        prologue "G8" "F1 - the shipped sample exercises all four detection rules" \
                 "verify_sample (links the production ftad_core)"
        ( cd "$scratch" && QT_QPA_PLATFORM=offscreen ./verify_sample ) 2>&1
    } | tee "$out"

    rc=${PIPESTATUS[0]}

    # Passing means all four rules fired, not merely that the run exited 0.
    if [ "$rc" -ne 0 ]; then
        record "G8" "FAIL" "functional/sample-dataset-$DAY.log (exit $rc)"
    elif grep -q "large_amount" "$out" \
         && grep -q "behaviour_deviation" "$out" \
         && grep -q "duplicate_payment" "$out" \
         && grep -q "rapid_withdrawals" "$out"
    then
        record "G8" "PASS" "functional/sample-dataset-$DAY.log"
    else
        record "G8" "FAIL" "functional/sample-dataset-$DAY.log (a rule did not fire)"
    fi
}

# ---------------------------------------------------------------------------
# G9 - clean-room installation verification
# ---------------------------------------------------------------------------
gate_g9()
{
    local rc
    local newest

    say ""
    say "=== G9  clean-room installation (I1) ==="

    if [ ! -x "$ROOT/verify_install.sh" ]; then
        say "verify_install.sh missing or not executable - gate SKIPPED"
        record "G9" "SKIPPED" "verify_install.sh not found"
        return
    fi

    # verify_install.sh writes its own provenance-stamped log, so its output is
    # indented here rather than tee'd a second time.
    "$ROOT/verify_install.sh" | sed 's/^/    /'
    rc=${PIPESTATUS[0]}

    newest="$(ls -1t "$EVIDENCE"/install/*.log 2>/dev/null | head -1)"

    if [ "$rc" -eq 0 ]; then
        record "G9" "PASS" "install/$(basename "${newest:-none}")"
    else
        record "G9" "FAIL" "install/$(basename "${newest:-none}") (exit $rc)"
    fi
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
write_summary()
{
    local summary="$EVIDENCE/SUMMARY.md"
    local i

    {
        echo "# Test gate evidence summary"
        echo ""
        echo "Generated: $STAMP  "
        echo "Host: $(uname -srm)  "
        echo "Project: $ROOT"
        echo ""
        echo "| Gate | Result | Evidence |"
        echo "|---|---|---|"
        for i in "${!GATE_ID[@]}"; do
            echo "| ${GATE_ID[$i]} | **${GATE_STATUS[$i]}** | ${GATE_NOTE[$i]} |"
        done
        echo ""
        echo "A SKIPPED gate means the tool was not installed; it is not a pass."
        echo "A FAIL gate means the command ran and reported a problem; the log"
        echo "records the exact output."
    } > "$summary"

    say ""
    rule
    say " SUMMARY"
    rule
    for i in "${!GATE_ID[@]}"; do
        printf '  %-3s %-8s %s\n' "${GATE_ID[$i]}" "${GATE_STATUS[$i]}" "${GATE_NOTE[$i]}"
    done
    rule
    say " written to: documentation/evidence/SUMMARY.md"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
usage()
{
    sed -n '2,34p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
}

if [ "${1:-}" = "--list" ] || [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
    usage
fi

ensure_build

mkdir -p "$EVIDENCE"/{ctest,coverage,static,dynamic,performance,functional,install}

if [ "$#" -eq 0 ]; then
    say "Running all nine gates. This takes several minutes (Valgrind is slow)."
    gate_g1
    gate_g2
    gate_g3
    gate_g4
    gate_g5
    gate_g6
    gate_g7
    gate_g8
    gate_g9
else
    for g in "$@"; do
        case "$g" in
            G1) gate_g1 ;;
            G2) gate_g2 ;;
            G3) gate_g3 ;;
            G4) gate_g4 ;;
            G5) gate_g5 ;;
            G6) gate_g6 ;;
            G7) gate_g7 ;;
            G8) gate_g8 ;;
            G9) gate_g9 ;;
            *)  say "Unknown gate: $g (valid: G1..G9)" >&2; exit 2 ;;
        esac
    done
fi

write_summary
