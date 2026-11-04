# Test gate evidence summary

Generated: 2026-11-04T05:54:36Z  
Host: Linux 7.1.13+parrot7-amd64 x86_64  
Project: /home/hashcat/Documents/dataset/FTAD-main

| Gate | Result | Evidence |
|---|---|---|
| G1 | **PASS** | ctest/ctest-20261104.log |
| G2 | **PASS** | coverage/lcov-20261104.log + coverage/html/ |
| G3 | **PASS** | static/cppcheck-20261104.log (app scope: 0 finding(s)) |
| G4 | **PASS** | static/clang-tidy-20261104.log (6 diagnostic(s)) |
| G5 | **PASS** | dynamic/asan-ubsan-20261104.log (0 findings) |
| G6 | **PASS** | dynamic/valgrind-20261104.log (0 error(s)) |
| G7 | **PASS** | performance/benchmark-20261104.log |
| G8 | **PASS** | functional/sample-dataset-20261104.log |
| G9 | **PASS** | install/install-verify-20261104-055814.log |

A SKIPPED gate means the tool was not installed; it is not a pass.
A FAIL gate means the command ran and reported a problem; the log
records the exact output.
