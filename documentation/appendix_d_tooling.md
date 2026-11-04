# Appendix D — Test Environment and Tooling

**Companion to:** Chapter 4, §4.7 and §4.8
**Status:** Reference material — excluded from the body page count

---

### D.1 Tool inventory

| # | Tool | Purpose | Version | Gate |
|---|---|---|---|---|
| 1 | GCC/G++ | Compilation and diagnostics | 14.2.0 | build |
| 2 | GoogleTest | Unit and integration framework | 1.16.0 (FetchContent, pinned) | X1 |
| 3 | CTest | Test runner and reporting | CMake 3.31.6 | X1 |
| 4 | gcov | Structural coverage | 14.2.0 | X5 |
| 5 | lcov | Coverage report generation | 2.3.1-1 | X5 |
| 6 | Cppcheck | Static analysis | 2.17.1 | X4 |
| 7 | Clang-Tidy | Static analysis and code quality | 19.1.7 | X4 |
| 8 | AddressSanitizer + UBSan | Dynamic analysis | GCC 14.2.0 | X3 |
| 9 | Valgrind (Memcheck) | Dynamic analysis | 3.24.0 | X3 |
| 10 | `ftad_benchmark` | Performance measurement | project-local | X8 |

All ten were executed on 4 November 2026. The lcov version string is inconsistent
and is recorded rather than corrected: the Debian package is 2.3.1-1 according to
`dpkg -s`, but the binary prints `LCOV version 2.0-1`. The package version is
authoritative, and anyone reproducing the run will see the same output.

Per-gate commands and log files are listed in
`documentation/evidence/README.md`; the consolidated result is
`documentation/evidence/SUMMARY.md`.

### D.2 Environment specification

| Component | Value |
|---|---|
| Operating system | Parrot Security 7.4 (Debian 13 base) |
| Kernel | 7.1.13+parrot7-amd64 |
| Architecture | x86_64 |
| Compiler | GCC/G++ 14.2.0 (Debian 14.2.0-19) |
| Build system | CMake 3.31.6, Ninja 1.12.1 |
| GUI toolkit | Qt 6.8.2 (Widgets, Sql, Charts) |
| Database | SQLite 3.46.1 via Qt `QSQLITE` |
| Scripting | Python 3.13.5 |
| C++ standard | C++17 |

The Release build used for test execution and benchmarking:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFTAD_BUILD_TESTS=ON
cmake --build build -j2
```

Coverage reconfigures the same tree with `-DFTAD_ENABLE_COVERAGE=ON`, adding
`--coverage -O0 -g` to `ftad_core`. Sanitizer runs add
`-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g"`.

The environment is a rolling, security-oriented Linux distribution. It is
representative of the target platform but is **not** a controlled benchmark
platform: no CPU frequency scaling was pinned and no other processes were
excluded during measurement. Timing figures in Chapter 6 must be read with that
caveat.
