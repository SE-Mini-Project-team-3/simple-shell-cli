# System Validation Report — Linux (11 November 2026)

## 1. Validation Environment

- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- Build system: GNU Make 4.3
- Language standard: C11
- Full test-suite evidence: `logs/11_linux_full_test_run_20261111.txt`
- Full coverage evidence: `logs/15_updated_full_coverage_20261111.txt`
- Focused F2/F3 coverage evidence: `logs/12_f2_f3_focused_coverage_20261111.txt`
- Static analysis evidence: `logs/14_cppcheck_full_analysis_20261111.txt`
- Performance evidence: `logs/12_performance_20261110.txt`
- Three-run prompt latency: `logs/13_prompt_latency_20261110.txt`
- Security evidence: `logs/08_sec03_hardened_validation_20261110.txt`
- Memory-safety evidence: `logs/09_sec03_hardened_asan_20261110.txt`

## 2. End-to-End Workflow Validation

| Workflow | Related Requirements | Validation Evidence | Result |
|---|---|---|---|
| Command input and parsing | FR-CI-02, FR-CI-06, SEC-02 | Parser specification tests cover empty input, whitespace, maximum input, oversized input, excessive arguments and malformed syntax. | Pass |
| Shell prompt and startup | FR-CI-01, FR-CI-04 | A pseudo-terminal test verified initial prompt display and prompt redisplay after `pwd` across three runs. | Pass |
| Built-in command handling | FR-CI-04, FR-HE-04, FR-HE-05, FR-HE-06 | Built-in parsing, environment operations, history integration and `cd` edge cases were tested. | Pass |
| Foreground process execution | FR-PE-01, FR-PE-02, FR-PE-03 | Executor tests cover successful execution, invalid commands, abnormal termination and signal handling. | Pass |
| Background process execution | FR-PE-04, FR-PE-06 | Executor tests verify background execution and process-group isolation. After completion, `reap_background_jobs()` is called and `waitpid(-1, ..., WNOHANG)` returns `-1` with `errno == ECHILD`, confirming no unreaped child remains. | Pass |
| Input/output redirection | FR-IO-01, FR-IO-02, FR-IO-03 | Input, output and append redirection tests passed, including environment output redirection. | Pass |
| Pipeline execution | FR-IO-04, FR-IO-05, FR-IO-07 | Two-stage, three-stage and maximum five-stage pipelines passed. Invalid stages, exit status and descriptor-leak checks were tested. | Pass |
| Command history | FR-HE-01, FR-HE-02, FR-HE-03 | History storage, retrieval, numbering, capacity and eviction behavior were tested. The `history` invocation itself consumes one history slot. | Pass |
| Environment management | FR-HE-04, FR-HE-05, FR-HE-06, FR-HE-07 | Export, overwrite, unset, validation and child-process visibility were tested. | Pass |
| Security: environment exposure | SEC-03 | Positive control and 11 negative scenarios passed, with 37 assertions and scenario-completion checks. | Pass |
| Security: input and memory safety | SEC-02, SEC-06 | Boundary and malformed-input tests passed; the full AddressSanitizer/LeakSanitizer run completed without errors. Valgrind was not run. | Pass |
| Security: shell interpretation and file operations | SEC-01, SEC-05 | Quoted metacharacter input remained literal, no injected marker file was created, non-writable-directory output was denied, and explicit redirection tests passed. | Pass |
| Performance and usability | NFR-01, NFR-02, NFR-03, NFR-04 | Three-run average startup was 1.773 ms and foreground `pwd`-to-next-prompt latency was 0.332 ms; both meet the 1-second limits. The maximum five-stage pipeline test passed, but ten-run pipeline throughput and first-time-user usability checks remain incomplete. | Partial |

## 3. Validation Summary

- Fully passed workflows: 12
- Partially validated workflows: 1
- Not executed workflows: 0

A partial result means that some relevant behavior has supporting test evidence, but the complete acceptance criteria have not been demonstrated.

The final automated Linux test run also passed all nine test groups:

1. Parser unit tests
2. History unit tests
3. Environment manager tests
4. Executor tests
5. Environment integration tests
6. Pipeline tests
7. Parser specification tests
8. Cross-module integration tests
9. SEC-03 environment exposure tests

## 4. Coverage Results

Aggregate production-source coverage:

- Line coverage: 68.6% (450/656)
- Function coverage: 93.1% (27/29)
- Branch coverage: 62.7% (320/510)

Module-specific line coverage:

| Module | Line Coverage | NFR-05 Target |
|---|---:|---|
| F1 — Command Interpreter | 79.7% | Met |
| F2 — Process Executor | 73.7% | Met |
| F3 — I/O & Pipeline Manager | 64.9% | Not met |
| F4 — History & Environment Manager | 100% | Met |

NFR-05 requires at least 70% line coverage in each functional module. F3 remains below this threshold at 64.9%; therefore, NFR-05 is not fully met.

The pipeline module contains process-management and operating-system-dependent error paths that were not fully exercised by the available deterministic tests. Additional experimental failure-path tests were attempted but discarded because they did not produce reliable coverage improvement. The remaining gap is documented in the coverage reports.

The aggregate and focused module-specific measurements use different measurement scopes and should not be treated as interchangeable. Aggregate figures cover production source files under `src/` and exclude `src/test_executor.c`.

Detailed results:
- `CODE_COVERAGE_REPORT.md`
- `BRANCH_COVERAGE_REPORT.md`

## 5. Known Validation Gaps

The remaining gaps are:

1. Complete the ten-run pipeline throughput measurement and first-time-user built-in discoverability check (NFR-03/NFR-04). TC-NFR-02 is therefore partially verified.
2. F3 line coverage is 64.9%, below the 70% per-module target in NFR-05. The other three modules meet the target.

These gaps are explicitly recorded rather than reported as completed.

## 6. Evidence References

- `logs/11_linux_full_test_run_20261111.txt`
- `logs/15_updated_full_coverage_20261111.txt`
- `logs/12_f2_f3_focused_coverage_20261111.txt`
- `logs/08_sec03_hardened_validation_20261110.txt`
- `logs/09_sec03_hardened_asan_20261110.txt`
- `logs/12_performance_20261110.txt`
- `logs/13_prompt_latency_20261110.txt`
- `TEST_CASE_EXECUTION_RESULTS.md`
- `UNIT_TEST_REPORT.md`
- `CODE_COVERAGE_REPORT.md`
- `BRANCH_COVERAGE_REPORT.md`
