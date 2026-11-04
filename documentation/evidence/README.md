# Evidence directory

Raw tool output captured during testing. Each subdirectory corresponds to a gate
in the test plan (Chapter 4, §4.7.2 and §4.10.4).

| Directory | Contents | Gate |
|---|---|---|
| `ctest/` | CTest run log | X1 |
| `coverage/` | lcov summary log, `coverage.info`, HTML report | X5 |
| `static/` | Cppcheck (full tree and application scope) and Clang-Tidy logs | X4 |
| `dynamic/` | AddressSanitizer + UBSan and Valgrind logs | X3 |
| `performance/` | Benchmark output | X8 |
| `functional/` | End-to-end run over the sample dataset, plus its harness source | F1 |
| `install/` | Clean-room installation transcript from `verify_install.sh` | I1 |

Every file records the exact command that produced it, the UTC date, the host
kernel and the tool version, so a reader can re-run it.

## How this was produced

Two scripts at the project root, both committed alongside the source:

| Script | Purpose |
|---|---|
| `install_test_tools.sh` | Installs lcov, Cppcheck, Clang-Tidy and Valgrind. Supports `--check` to report presence without changing anything. Run once, by the operator, because it needs `sudo`. |
| `run_test_gates.sh` | Runs gates G1–G7 and writes every log here, plus `SUMMARY.md`. Supports `--list` and running individual gates, e.g. `./run_test_gates.sh G1 G5`. |

Reproduce the whole evidence set with:

```
./run_test_gates.sh
```

## Reading the numbers

- **Static analysis has two scopes.** `cppcheck-*.log` covers the whole tree and
  includes the vendored GoogleTest in `build/_deps`; `cppcheck-app-*.log` covers
  application source only and is the figure the report quotes. Third-party
  findings are not FTAD's to remediate.
- **A SKIPPED gate is not a pass.** It means the tool was absent.
- **Valgrind's "still reachable" blocks** are Qt platform-plugin and global test
  registry allocations. `definitely lost` and `indirectly lost` are both zero,
  which is what the gate asserts.
- **Coverage is measured on an unoptimised build** (`-O0 --coverage`) because
  optimisation merges statements and distorts line attribution. It is not
  comparable with the Release build used for the benchmark.

## Summary

`SUMMARY.md` is regenerated on every run and lists each gate with its result and
the evidence file that supports it.
