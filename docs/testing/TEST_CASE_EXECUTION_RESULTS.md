# Test Case Execution Results — Linux (10 November 2026)

## 1. Environment and Evidence
- Environment: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- Test execution: GCC coverage-instrumented build
- Main execution log: `logs/10_linux_coverage_run_20261110.txt`
- Final test-suite log: `logs/11_final_validation_20261110.txt`
- Performance benchmark: `logs/12_performance_20261110.txt`
- Three-run prompt latency: `logs/13_prompt_latency_20261110.txt`
- Security validation: `logs/08_sec03_hardened_validation_20261110.txt`
- Memory-safety validation: `logs/09_sec03_hardened_asan_20261110.txt`

## 2. Test Case Results

| Test Case | Requirements | Actual Result | Status |
|---|---|---|---|
| TC-CI-01 | FR-CI-01, FR-CI-04 | Built-in classification and parsing tests passed. A pseudo-terminal test verified initial prompt display and redisplay after `pwd` across three runs. | Pass |
| TC-CI-02 | FR-CI-02, FR-CI-06, SEC-02 | Exactly 1024 characters accepted; oversized input and excess arguments rejected. Malformed redirection and quotes rejected. | Pass |
| TC-PE-01 | FR-PE-01, FR-PE-02, FR-PE-03 | Foreground execution, invalid command handling, abnormal termination and child signal handling tests passed. | Pass |
| TC-PE-02 | FR-PE-04, FR-PE-06 | Background execution and process-group tests passed. The test waits for the background child to finish, calls `reap_background_jobs()`, then verifies `waitpid(-1, NULL, WNOHANG)` returns -1 with `errno == ECHILD`. | Pass |
| TC-IO-01 | FR-IO-01, FR-IO-02, FR-IO-03 | Output, append and input redirection tests passed; integration also verified redirected `env` output. | Pass |
| TC-IO-02 | FR-IO-04, FR-IO-05, FR-IO-07 | Two-stage, three-stage and maximum five-stage pipelines passed. Repeated-pipeline descriptor leak checks passed. | Pass |
| TC-HE-01 | FR-HE-01, FR-HE-02, FR-HE-03 | History recording, numbering, 50-entry limit and eviction verified through the real shell. The `history` command itself consumes a slot; six earlier entries are evicted in the 55-command scenario. | Pass |
| TC-HE-02 | FR-HE-04, FR-HE-05, FR-HE-06, FR-HE-07 | Export, env, unset and environment visibility in child processes verified. | Pass |
| TC-SEC-01 | SEC-01, SEC-05 | A quoted semicolon injection attempt was printed literally; no secondary command or marker file was created. A write to a non-writable directory was denied. Existing tests verify explicit output redirection. | Pass |
| TC-SEC-02 | SEC-02, SEC-06 | Input-boundary tests passed. Full AddressSanitizer suite completed without sanitizer or LeakSanitizer errors. Valgrind was not run. | Pass |
| TC-NFR-01 | NFR-01, NFR-02 | Three-run average startup was 1.773 ms; average foreground `pwd` completion to next prompt was 0.332 ms. Both are below the 1-second limits. | Pass |
| TC-NFR-02 | NFR-03, NFR-04 | Maximum five-stage pipeline test passed. Ten-run throughput and first-time-user built-in discoverability checks are not evidenced. | Partial |

## 3. Execution Summary

- Total planned test cases: 12
- Passed: 11
- Partially verified: 1
- Not executed: 0

A partially verified test case has supporting evidence for some, but not all, of its specified steps. It is not counted as fully passed.

## 4. Important Observations

### History boundary
The history integration test executes 55 distinct echo commands and then invokes `history`. Since the `history` invocation is itself recorded, the resulting 50 entries contain 49 echo commands and the history command. Therefore, six earlier echo commands are evicted. This behavior is documented in `TEST_REPORT.md` and is consistent with the implementation's 50-entry limit.

### Coverage
Aggregate production-source line coverage is 67.1% (440/656), with 61.4% branch coverage (313/510). These aggregate figures are distinct from the earlier module-specific coverage measurements in `TEST_REPORT.md`.

### Outstanding validation
- Explicit prompt display and redisplay check.
- Explicit zombie-process verification.
- SEC-01 shell-metacharacter injection test.
- SEC-05 explicit-redirection-only file-operation test.
- Startup and prompt-response latency measurements.
- Ten-run pipeline throughput and first-time-user usability check.

## 5. Evidence References
- `logs/10_linux_coverage_run_20261110.txt`
- `logs/08_sec03_hardened_validation_20261110.txt`
- `logs/09_sec03_hardened_asan_20261110.txt`
- `coverage/code_coverage_20261110.txt`
- `coverage/branch_coverage_20261110.txt`
- `TEST_REPORT.md`
