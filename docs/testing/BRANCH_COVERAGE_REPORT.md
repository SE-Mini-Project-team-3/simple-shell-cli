# Branch Coverage Report — Linux (11 November 2026)

## 1. Environment
- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- gcov: GCC 13.3.0
- gcovr: 7.0
- Instrumentation: GCC `--coverage`
- Scope: Production source files under `src/`, excluding `src/test_executor.c`.

## 2. Latest full-suite results

| Metric | Covered | Total | Coverage |
|---|---:|---:|---:|
| Branches | 320 | 510 | 62.7% |

The latest full-suite run also measured 450/656 lines (68.6%) and
27/29 functions (93.1%).

Detailed output: `logs/15_updated_full_coverage_20261111.txt`

## 3. Focused process-module branch measurements

The focused coverage-aware measurements collected child-process coverage
using GCC fork/exec hooks and an exit wrapper.

| Feature | Branch coverage |
|---|---:|
| F2 — Process Executor | 68.0% |
| F3 — I/O and Pipeline Manager | 64.0% |

These are focused module measurements, not the aggregate full-suite result.

## 4. Comparison with 10 November 2026

The 10 November full-suite run recorded 313/510 branches (61.4%).
The 11 November full-suite run recorded 320/510 branches (62.7%).

Aggregate branch coverage increased by 1.3 percentage points.
The November 10 report remains historical evidence.

## 5. Interpretation
The latest aggregate branch coverage is 62.7%. Branch coverage is
reported separately from NFR-05, which specifies a 70% line-coverage
target per module.

F2 and F3 focused branch results are included as supplementary evidence.
Their focused measurements use child-process coverage collection and
must not be confused with the aggregate full-suite measurement.

## 6. Evidence
- `logs/15_updated_full_coverage_20261111.txt` — latest aggregate run.
- `logs/12_f2_f3_focused_coverage_20261111.txt` — focused F2/F3 results.
- `coverage/branch_coverage_20261110.txt` — historical detailed branch report.
- `logs/10_linux_coverage_run_20261110.txt` — historical full-suite run.
