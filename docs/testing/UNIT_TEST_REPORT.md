# Unit Test Report — Linux (10 November 2026)

## 1. Test Environment
- OS: Ubuntu 24.04 (WSL)
- Compiler: GCC 13.3.0
- Build system: GNU Make 4.3
- Language standard: C11
- Instrumentation: GCC `--coverage`
- Main evidence: `logs/10_linux_coverage_run_20261110.txt`

## 2. Unit and Component Test Results

| Test suite | Component | Expected result | Actual result | Status |
|---|---|---|---|---|
| `tests/test_parser` | Parser | Valid commands parse correctly and invalid input is handled safely. | All parser tests passed. | Pass |
| `tests/test_parser_spec` | Parser specification | Tokenization, argument limits, input boundaries and malformed syntax are handled correctly. | All specification checks passed, including exact maximum input, oversized input, excess arguments and malformed syntax. | Pass |
| `tests/test_history` | History | Entries are stored, numbered, retrieved and evicted correctly. | 15/15 checks passed, including negative and out-of-range access. | Pass |
| `tests/test_env_manager` | Environment manager | Set, overwrite, unset, validation and child inheritance work correctly. | All environment manager tests passed. | Pass |
| `test_executor` | Process executor | Foreground/background execution, error handling, signals and redirection behave correctly. | 12/12 executor tests passed. | Pass |
| `tests/test_pipeline` | Pipeline manager | Pipelines execute correctly, enforce stage limits and avoid descriptor leaks. | 10/10 pipeline tests passed. | Pass |
| `tests/test_env_integration` | Environment integration | Exported values are visible to env and child processes; unset removes values. | All environment integration tests passed. | Pass |
| `tests/test_integration` | Cross-module integration | Combined shell workflows involving history, environment, execution and pipelines work correctly. | 38 CHECK assertions passed. | Pass |
| `tests/test_sec03_env_exposure` | Security / SEC-03 | Environment values are not disclosed through unintended output paths. | 37 assertions passed, including a positive control and 11 negative scenarios. | Pass |

## 3. Important Test Coverage

### Parser
- Built-in and external command classification.
- NULL, empty and whitespace-only input.
- Exact maximum input length and oversized input.
- Maximum argument limit.
- Missing redirection filename.
- Unterminated quotes.

### History
- Command storage and numbering.
- Maximum history size and eviction.
- Empty/null command handling.
- Negative and out-of-range retrieval.

### Environment manager
- Setting and overwriting variables.
- Unsetting variables.
- Invalid names and NULL arguments.
- Child-process environment inheritance.

### Process executor
- Valid foreground execution.
- Invalid command handling.
- Background execution.
- Abnormal child termination.
- Child SIGINT handling and shell SIGINT behavior.
- Input/output/append redirection.
- Redirection failures and permission-denied handling.

### Pipeline manager
- Two-stage and three-stage pipelines.
- Maximum five-stage pipeline.
- Final-stage exit status.
- Invalid commands and excessive stages.
- Input redirection.
- Repeated-pipeline file descriptor checks.
- Background pipeline process-group behavior.

### Integration and security
- History output and history boundary.
- Exported variables visible in pipeline stages and background processes.
- Environment output redirection.
- Malformed pipeline and built-in error handling.
- `cd` edge cases.
- SEC-03 positive control and 11 negative scenarios.

## 4. Memory-Safety Validation

The full test suite was also executed under AddressSanitizer.

An earlier run identified a 175-byte leak in the SEC-03 test helper. The helper was corrected to release `history_output` exactly once.

The subsequent full AddressSanitizer run completed without AddressSanitizer or LeakSanitizer errors.

Evidence: `logs/09_sec03_hardened_asan_20261110.txt`

Valgrind results are not claimed because Valgrind was not run.

## 5. Test Execution Evidence

- `logs/10_linux_coverage_run_20261110.txt` — clean coverage-instrumented build and full test execution.
- `logs/08_sec03_hardened_validation_20261110.txt` — hardened SEC-03 validation.
- `logs/09_sec03_hardened_asan_20261110.txt` — AddressSanitizer full-suite validation.

## 6. Result Summary

All nine listed automated test suites completed successfully in the recorded Linux coverage run. The executor suite passed 12/12 tests, the pipeline suite passed 10/10 tests, the history suite passed 15/15 checks, and the SEC-03 suite passed 37 assertions.

These automated results do not imply that every manual Test Plan case has been completed. See `TEST_CASE_EXECUTION_RESULTS.md` for the status of all 12 planned test cases.


### 6.1 Focused F2/F3 Linux remeasurement — 11 November 2026

A subsequent focused Linux run used GCC coverage-aware fork/exec hooks
and an exit wrapper to collect coverage from child processes.

- F2 `executor.c`: 56/76 lines covered (**73.7%**); 12/12 executor tests passed.
- F3 `pipeline.c`: 87/134 lines covered (**64.9%**); 16/16 pipeline tests passed.

F2 meets the NFR-05 70% line-coverage target. F3 remains below target.

Detailed focused coverage output is preserved in
`logs/12_f2_f3_focused_coverage_20261111.txt`. The subsequent complete
Linux test-suite transcript is preserved in
`logs/11_linux_full_test_run_20261111.txt`.
These focused results are separate from the earlier 10 November full-suite
execution evidence above.

---

## 7. Latest Linux revalidation — 11 November 2026

A fresh Linux full-suite run was completed using Ubuntu 24.04 (WSL),
GCC 13.3.0 and GNU Make 4.3.

All nine test groups passed:

| Test group | Latest result |
|---|---|
| Parser unit tests | Passed |
| History unit tests | Passed |
| Environment manager tests | Passed |
| Executor tests | 12/12 passed |
| Environment integration tests | Passed |
| Pipeline tests | 16/16 passed |
| Parser specification tests | Passed |
| Cross-module integration tests | Passed |
| SEC-03 environment exposure tests | Passed — 37 assertions |

The November 11 pipeline suite includes additional validation,
redirection-failure and signal-termination cases. This is why its
16/16 result differs from the earlier 10/10 result. The earlier result
is retained as historical evidence, not treated as a failure.

### 7.1 Focused process-module coverage

A focused coverage-aware run collected child-process execution using
GCC fork/exec hooks and an exit wrapper.

- Executor: 56/76 lines (**73.7%**); 12/12 tests passed.
- Pipeline: 87/134 lines (**64.9%**); 16/16 tests passed.

The ordinary full-suite coverage run reports lower executor and
pipeline line counts because it does not collect all forked/executed
child-process coverage. These are different measurement scopes.

### 7.2 Aggregate full-suite coverage

The fresh full-suite coverage run measured:

- Line coverage: **68.6% (450/656)**.
- Function coverage: **93.1% (27/29)**.
- Branch coverage: **62.7% (320/510)**.

The 10 November aggregate result was 67.1% line coverage and 61.4%
branch coverage. The fresh aggregate measurements increased.

### 7.3 Historical comparison and interpretation

Earlier dedicated-suite results and the November 11 focused results
were obtained with different toolchains, source-line denominators and
coverage collection scopes. In particular, the focused F2/F3 run
collects child-process coverage that the ordinary full-suite run
does not fully capture. These figures should therefore be reported
with their measurement scope rather than compared as direct
like-for-like regressions.

The November 10 reports and logs remain unchanged as historical
evidence. The November 11 logs are the latest evidence.

### 7.4 Evidence

- `logs/11_linux_full_test_run_20261111.txt`
- `logs/12_f2_f3_focused_coverage_20261111.txt`
- `logs/13_linux_full_coverage_run_20261111.txt`
