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
