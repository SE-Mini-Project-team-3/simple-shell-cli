# System Validation Report — Linux (10 November 2026)

## 1. Validation Environment

- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- Build system: GNU Make 4.3
- Language standard: C11
- Validation evidence: `logs/10_linux_coverage_run_20261110.txt`
- Final test-suite evidence: `logs/11_final_validation_20261110.txt`
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
| Command history | FR-HE-01, FR-HE-02, FR-HE-03 | History storage, retrieval, numbering, capacity and eviction behavior were tested. | Pass |
| Environment management | FR-HE-04, FR-HE-05, FR-HE-06, FR-HE-07 | Export, overwrite, unset, validation and child-process visibility were tested. | Pass |
| Security: environment exposure | SEC-03 | Positive control and 11 negative scenarios passed, with 37 assertions and scenario-completion checks. | Pass |
| Security: input and memory safety | SEC-02, SEC-06 | Boundary and malformed-input tests passed; full AddressSanitizer/LeakSanitizer run completed without errors. | Pass |
| Security: shell interpretation and file operations | SEC-01, SEC-05 | Quoted metacharacter input remained literal, no injected marker file was created, non-writable-directory output was denied, and explicit redirection tests passed. | Pass |
| Performance and usability | NFR-01, NFR-02, NFR-03, NFR-04 | Three-run average startup was 1.773 ms and foreground `pwd`-to-next-prompt latency was 0.332 ms; both meet the 1-second limits. Ten-run pipeline throughput and first-time-user usability checks remain incomplete. | Partial |

## 3. Validation Summary

- Fully passed workflows: 12
- Partially validated workflows: 1
- Not executed workflows: 0

A partial result means that some relevant behavior has supporting test evidence, but the complete acceptance criteria have not been demonstrated.

## 4. Coverage Results

Aggregate production-source coverage:

- Line coverage: 67.1% (440/656)
- Function coverage: 93.1% (27/29)
- Branch coverage: 61.4% (313/510)

Detailed results:
- `CODE_COVERAGE_REPORT.md`
- `BRANCH_COVERAGE_REPORT.md`

These aggregate figures cover production source files under `src/` and exclude `src/test_executor.c`.

## 5. Known Validation Gaps

The remaining incomplete checks are performance/usability items, not security gaps:

1. Perform ten-run pipeline throughput and first-time-user built-in discoverability checks (NFR-03/NFR-04).

## 6. Evidence References

- `logs/10_linux_coverage_run_20261110.txt`
- `logs/08_sec03_hardened_validation_20261110.txt`
- `logs/09_sec03_hardened_asan_20261110.txt`
- `coverage/code_coverage_20261110.txt`
- `coverage/branch_coverage_20261110.txt`
- `TEST_CASE_EXECUTION_RESULTS.md`
- `UNIT_TEST_REPORT.md`
