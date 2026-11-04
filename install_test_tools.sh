#!/usr/bin/env bash
#
# install_test_tools.sh
#
# Installs the analysis tools that the FTAD test plan (Chapter 4, section 4.7)
# requires but that are missing from this machine:
#
#   lcov        coverage report generation   (gate X5)
#   cppcheck    static analysis              (gate X4)
#   clang-tidy  static analysis              (gate X4)
#   valgrind    dynamic memory analysis      (gate X3)
#
# This script must be run by the operator. It calls apt-get and therefore
# needs sudo. It does not modify the FTAD source tree in any way.
#
# Usage:
#   ./install_test_tools.sh              install everything (asks to confirm)
#   ./install_test_tools.sh --yes        install without the confirmation prompt
#   ./install_test_tools.sh --check      only report what is present/missing
#   ./install_test_tools.sh --no-valgrind
#                                        skip valgrind (it is 76% of the
#                                        download: 14.7 MB of 19.3 MB)
#   ./install_test_tools.sh --update     run apt-get update first
#
# Estimated download: 19.3 MB total, 4.6 MB without valgrind.
# Estimated disk:     106.8 MB total, 25.2 MB without valgrind.

set -euo pipefail

PACKAGES=(lcov cppcheck clang-tidy valgrind)
ASSUME_YES=0
CHECK_ONLY=0
DO_UPDATE=0

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
for arg in "$@"; do
    case "$arg" in
        --yes|-y)
            ASSUME_YES=1
            ;;
        --check)
            CHECK_ONLY=1
            ;;
        --no-valgrind)
            PACKAGES=(lcov cppcheck clang-tidy)
            ;;
        --update)
            DO_UPDATE=1
            ;;
        --help|-h)
            sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Try --help" >&2
            exit 2
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
say()
{
    printf '%s\n' "$*"
}

have()
{
    command -v "$1" >/dev/null 2>&1
}

report_presence()
{
    say "Current state of the required tools:"
    say ""

    local missing=0

    for package in "${PACKAGES[@]}"; do
        # cppcheck and clang-tidy are invoked by name; lcov and valgrind too.
        if have "$package"; then
            local version
            version="$("$package" --version 2>&1 | head -1)"
            printf '  [present] %-12s %s\n' "$package" "$version"
        else
            printf '  [MISSING] %-12s\n' "$package"
            missing=$((missing + 1))
        fi
    done

    say ""
    say "Missing: $missing of ${#PACKAGES[@]}"
    return "$missing"
}

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
if ! have apt-get; then
    say "ERROR: apt-get not found. This script targets Debian/Ubuntu systems." >&2
    exit 1
fi

if [ "$(id -u)" -ne 0 ] && ! have sudo; then
    say "ERROR: not root and sudo is unavailable." >&2
    exit 1
fi

say "=============================================================="
say " FTAD test tool installation"
say "=============================================================="
say ""

# report_presence returns non-zero when tools are missing; that is not an
# error for --check, so the exit status is captured rather than propagated.
report_presence || true
say ""

if [ "$CHECK_ONLY" -eq 1 ]; then
    say "Check-only mode: nothing was installed."
    exit 0
fi

# ---------------------------------------------------------------------------
# Confirmation
# ---------------------------------------------------------------------------
say "The following packages will be installed via apt-get:"
say "  ${PACKAGES[*]}"
say ""

if [ "${#PACKAGES[@]}" -eq 4 ]; then
    say "Estimated download: 19.3 MB   Estimated disk: 106.8 MB"
else
    say "Estimated download:  4.6 MB   Estimated disk:  25.2 MB"
fi

say ""

if [ "$ASSUME_YES" -ne 1 ]; then
    printf 'Proceed? [y/N] '
    read -r reply
    case "$reply" in
        [yY]|[yY][eE][sS])
            ;;
        *)
            say "Aborted. Nothing was changed."
            exit 0
            ;;
    esac
fi

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
if [ "$DO_UPDATE" -eq 1 ]; then
    say ""
    say "--> apt-get update"
    sudo apt-get update
fi

say ""
say "--> apt-get install ${PACKAGES[*]}"
sudo apt-get install -y "${PACKAGES[@]}"

# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------
say ""
say "=============================================================="
say " Verification"
say "=============================================================="
say ""

failed=0

verify()
{
    local name="$1"
    local command="$2"

    if eval "$command" >/dev/null 2>&1; then
        local version
        version="$(eval "$command" 2>&1 | head -1)"
        printf '  [ok]   %-12s %s\n' "$name" "$version"
    else
        printf '  [FAIL] %-12s not runnable\n' "$name"
        failed=$((failed + 1))
    fi
}

for package in "${PACKAGES[@]}"; do
    case "$package" in
        cppcheck)
            verify "cppcheck" "cppcheck --version"
            ;;
        clang-tidy)
            verify "clang-tidy" "clang-tidy --version"
            ;;
        lcov)
            verify "lcov" "lcov --version"
            ;;
        valgrind)
            verify "valgrind" "valgrind --version"
            ;;
    esac
done

say ""

if [ "$failed" -ne 0 ]; then
    say "WARNING: $failed tool(s) did not verify. Check the output above."
    exit 1
fi

say "All requested tools installed and runnable."
say ""
say "Next step: tell the assistant 'installed', and it will re-run all seven"
say "gates and capture the output into documentation/evidence/."
