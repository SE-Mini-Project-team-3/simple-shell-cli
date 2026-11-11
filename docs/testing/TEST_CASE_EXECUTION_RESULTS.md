# Test Case Execution Results — Linux (11 November 2026)

## 1. Environment and Evidence

- Environment: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- Build system: GNU Make
- Coverage instrumentation: GCC gcov
- Main test execution log: `logs/11_linux_full_test_run_20261111.txt`
- Full coverage execution log: `logs/13_linux_full_coverage_run_20261111.txt`
- Focused F2/F3 coverage log: `logs/12_f2_f3_focused_coverage_20261111.txt`
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
| TC-NFR-01 | NFR-01, NFR-02 | Three-run average startup was 1.773 ms; average foreground `pwd` completion-to-next-prompt time was 0.332 ms. Both are below the 1-second limits. | Pass |
| TC-NFR-02 | NFR-03, NFR-04 | Maximum five-stage pipeline test passed. Ten-run pipeline throughput and first-time-user built-in discoverability checks are not evidenced. | Partial |

## 3. Execution Summary

- Total planned test cases: 12
- Passed: 11
- Partially verified: 1
- Not executed: 0

A partially verified test case has supporting evidence for some, but not all, of its specified steps. It is not counted as fully passed.

The automated validation also includes nine test groups, all of which passed in the final Linux test run:

1. Parser unit tests
2. History unit tests
3. Environment manager tests
4. Executor tests
5. Environment integration tests
6. Pipeline tests
7. Parser specification tests
8. Cross-module integration tests
9. SEC-03 environment exposure tests

## 4. Important Observations

### History boundary

The history integration test executes 55 distinct echo commands and then invokes `history`. Since the `history` invocation is itself recorded, the resulting 50 entries contain 49 echo commands and the history command. Therefore, six earlier echo commands are evicted. This behavior is documented in `TEST_REPORT.md` and is consistent with the implementation's 50-entry limit.

### Coverage

Final aggregate production-source coverage:

- Line coverage: 67.7% (444/656)
- Function coverage: 93.1% (27/29)
- Branch coverage: 62.4% (318/510)

Final module-specific line coverage:

| Module | Line Coverage |
|---|---:|
| F1 | 79.7% |
| F2 | 73.7% |
| F3 (pipeline) | 64.9% |
| F4 | 100% |

F3 remains below the 70% line-coverage target. The pipeline module contains process-management and operating-system-dependent error paths that were not fully exercised by the available deterministic tests. Additional experimental failure-path tests were attempted but discarded because they did not produce reliable coverage improvement. The remaining gap is documented in the coverage reports.

The aggregate and focused module-specific measurements use different measurement scopes and should not be treated as interchangeable.

### Performance

Three-run measurements recorded:

- Average shell startup time: 1.773 ms
- Average foreground `pwd` completion-to-next-prompt time: 0.332 ms

Both are below the 1-second limits.

Ten-run pipeline throughput and first-time-user built-in discoverability checks remain partially unverified.

### Security and memory safety

- Shell-metacharacter injection validation passed.
- Explicit redirection behavior was tested.
- The full AddressSanitizer suite completed without sanitizer or LeakSanitizer errors.
- Valgrind was not run.

### Remaining validation

The remaining evidence gap is limited to the following TC-NFR-02 checks:

- Ten-run pipeline throughput measurement.
- First-time-user built-in discoverability/usability check.

These are recorded as partially verified rather than completed.

## 5. Evidence References

- `logs/11_linux_full_test_run_20261111.txt`
- `logs/13_linux_full_coverage_run_20261111.txt`
- `logs/12_f2_f3_focused_coverage_20261111.txt`
- `logs/08_sec03_hardened_validation_20261110.txt`
- `logs/09_sec03_hardened_asan_20261110.txt`
- `logs/12_performance_20261110.txt`
- `logs/13_prompt_latency_20261110.txt`
- `TEST_REPORT.md`
