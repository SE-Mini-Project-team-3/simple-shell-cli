# Branch Coverage Report — Linux (10 November 2026)

## 1. Environment
- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- gcov: GCC 13.3.0
- gcovr: 7.0
- Instrumentation: GCC `--coverage`
- Scope: Production source files under `src/`, excluding `src/test_executor.c`.

## 2. Results

| Source file | Branches | Taken | Coverage |
|---|---:|---:|---:|
| src/env_manager.c | 22 | 18 | 81% |
| src/executor.c | 50 | 15 | 30% |
| src/history.c | 16 | 16 | 100% |
| src/main.c | 204 | 139 | 68% |
| src/parser.c | 104 | 88 | 84% |
| src/pipeline.c | 114 | 37 | 32% |
| **Total** | **510** | **313** | **61.4%** |

## 3. Interpretation
History has complete branch coverage in this run. Parser and environment
manager also have relatively high branch coverage.

Executor, pipeline and main retain uncovered branches. Additional tests
should target error handling, process creation/failure paths, pipeline
validation and less frequently executed shell workflows.

## 4. Evidence
- `coverage/branch_coverage_20261110.txt`
- `logs/10_linux_coverage_run_20261110.txt`
