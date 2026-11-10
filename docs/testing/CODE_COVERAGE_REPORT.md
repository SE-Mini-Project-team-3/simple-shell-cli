# Code Coverage Report — Linux (10 November 2026)

## 1. Environment
- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- gcov: GCC 13.3.0
- gcovr: 7.0
- Instrumentation: GCC `--coverage`
- Scope: Production source files under `src/`, excluding `src/test_executor.c`.

## 2. Execution
The complete test suite was rebuilt with coverage instrumentation and executed.
All test groups passed.

Detailed output: `logs/10_linux_coverage_run_20261110.txt`

## 3. Results

| Source file | Lines | Executed | Coverage |
|---|---:|---:|---:|
| src/env_manager.c | 21 | 21 | 100% |
| src/executor.c | 76 | 30 | 39% |
| src/history.c | 30 | 30 | 100% |
| src/main.c | 282 | 212 | 75% |
| src/parser.c | 113 | 103 | 91% |
| src/pipeline.c | 134 | 44 | 32% |
| **Total** | **656** | **440** | **67.1%** |

- Function coverage: 27/29 (93.1%)
- Branch coverage: 313/510 (61.4%)

## 4. Interpretation
Parser, history and environment-manager code achieved strong line coverage.
Executor and pipeline modules have substantial uncovered paths and require
additional tests to improve coverage.

The aggregate line coverage is 67.1%. This is below a 70% aggregate target.
Earlier module-specific coverage figures in `TEST_REPORT.md` use a different
measurement scope and should not be confused with this aggregate run.

## 5. Evidence
- `coverage/code_coverage_20261110.txt`
- `logs/10_linux_coverage_run_20261110.txt`
