# Sprint 2 — Combined Testing & Security Report

**Scope:** Consolidated testing, integration validation, security verification, module-wise coverage analysis, defect documentation, and Linux revalidation for the Simple Shell CLI.

**Baseline:** `main` at `9b7cf66` ("docs: add sprint 2 closure report")

**Reference environment:** Ubuntu/GCC, as specified in STP §6. Earlier macOS results are retained for comparison, with platform-specific differences explicitly documented.

This report consolidates the Sprint 2 testing and security evidence across all modules. It records test coverage, execution results, security validation, defects identified, and outstanding requirements without attributing the combined results to individual team members.

Reported results are based on recorded test executions and supporting evidence under `docs/testing/logs/` and `docs/testing/coverage/`.

---

## 1. What was added this pass

| File | Purpose |
|---|---|
| `tests/test_history.c` (extended) | Added `test_out_of_range_access()`: asserts `history_get`/`history_get_number` reject negative and at-capacity positions. Closed a real unit-coverage gap (see §4). |
| `tests/test_integration.c` (new) | Cross-module integration tests. Drives the **compiled shell binary**, not isolated module calls, through sequences spanning History + Environment + Pipeline/Redirection + Process Execution together: `history` output format (FR-HE-02), export visible inside a pipeline stage (F3+F4), `env` output redirection (F3+F4), export visible to a background external process (F2+F4), and the 50-entry history boundary exercised through the real parser→dispatcher path (FR-HE-01/03), not direct `history_add()` calls. |
| `tests/test_sec03_env_exposure.c` (new) | SEC-03 validation: plants a secret environment value, exercises 11 different error/builtin paths, and asserts the secret never leaks except via the one sanctioned `env` disclosure path. Includes a positive control. |
| `Makefile` (extended) | Wired the two new test binaries into `make test` and `clean`. |

All new test code was built and run repeatedly during development; failures were investigated and root-caused before being accepted or fixed (see §3 for two genuine issues identified and resolved in the tests).

---

## 2. Full automated test suite — results

Command: `make clean && make && make <each test target> && <run each binary>` (see `docs/testing/logs/06_final_production_build_*.txt` for the complete, unedited transcript of this run, using the non-instrumented/production configuration).

| Test binary | Module(s) | Result | Notes |
|---|---|---|---|
| `tests/test_parser` | F1 | pass | |
| `tests/test_parser_spec` | F1 | pass | US-01 acceptance specification |
| `tests/test_history` | F4 | pass | 15/15, including 4 new boundary checks |
| `tests/test_env_manager` | F4 | pass | |
| `test_executor` | F2 | pass | |
| `tests/test_env_integration` | F4 (via real shell) | pass | |
| `tests/test_pipeline` | F3 | 7/9 | 2 failures — platform-specific, see §3 |
| `tests/test_integration` | F2+F3+F4 (new) | pass | 38 CHECK assertions |
| `tests/test_sec03_env_exposure` | F4 / security (new) | pass | 26 assertions |

**9 binaries run, 8 fully green, 1 with 2 known platform-specific failures out of 9 sub-checks.** Nothing here is marked passed without the corresponding test evidence.

The Linux revalidation results in §8 supersede these earlier platform-specific results wherever they differ.

---

## 3. Defects found

### 3.1 `tests/test_pipeline.c` — 2 failures on macOS, confirmed passing on the reference CI environment

```text
FAIL: file descriptor count changed (before=-1, after=-1)

FAIL: background pipeline process-group isolation (status=0, pid_files=1, group_check=0, reaped=2)
```

- **Root cause 1 (fd-count test):** `count_open_fds()` in `tests/test_pipeline.c` opens `/proc/self/fd`, which does not exist on macOS/BSD (it is Linux-specific). `opendir()` returns `NULL`, the function returns `-1` for both `before` and `after`, and the check `before >= 0 && before == after` fails on the `>= 0` guard — not because of an actual descriptor leak.

- **Root cause 2 (process-group test):** `getpgid()`-based process-group checks return `group_check_ok = 0` consistently (reproduced 3/3 runs, not flaky). This is a macOS versus Linux process-group/signal-semantics difference in how the test observes the child's process group, not a crash or hang.

- The affected implementation and test were not modified during this validation pass. Evidence was collected to distinguish an environment-specific test failure from a functional regression:
  - Local failure log: `docs/testing/logs/06_final_production_build_*.txt`
  - **Reference-environment (Ubuntu, GitHub Actions) log pulled from the project's CI, using the same baseline commit:** `docs/testing/logs/02_ci_reference_run_42102566649.txt` — shows `Pipeline tests: 9/9 passed`.

- **Recommendation:** treat this as a test-portability improvement. Consider using `/dev/fd` as a fallback, or skipping/guarding the fd-count check when `/proc/self/fd` is unavailable. It was not blocking in the reference environment, where the tests passed.

### 3.2 Bug found and fixed in the new integration test (`test_integration.c`) during development

The first version of the 55-command history-boundary test asserted the oldest surviving entry would be `cmd6`. It failed on the first run. Investigation showed the `history` command itself is a successfully parsed command line, so running it **also** consumes a history slot (recorded before it prints).

With 55 `echo` commands plus the `history` invocation itself, 56 entries are added against a 50-entry cap. The correct oldest surviving entry is `cmd7`, with `history`'s own invocation occupying the 50th (newest) slot.

This is correct FR-HE-01 behavior, not a shell bug. The test expectations were corrected to assert this precisely (see the comment in `test_history_boundary_through_real_shell()`), rather than loosening the assertion.

No other defects were found in the F4 module during this pass.

---

## 4. Coverage (NFR-05: ≥70% line coverage per module's own unit test suite)

Toolchain note: `make`'s recipe shell does not read the machine's `gcc` alias, so `make`-driven builds use Apple clang via `/usr/bin/gcc` — confirmed by `.gcno` format version. Coverage was measured with the matching `/usr/bin/gcov`.

Full build and run transcript: `docs/testing/logs/04_final_coverage_run_*.txt`; summary command output: `docs/testing/logs/05_coverage_summary_*.txt`.

### 4.1 Per-module dedicated unit test suite (the NFR-05 metric)

| Module | Suite | Lines covered | vs. 70% target |
|---|---|---|---|
| F4 `history.c` | `tests/test_history` | **80.56%** (29/36) | pass |
| F4 `env_manager.c` | `tests/test_env_manager` | **80.77%** (21/26) | pass |
| F1 `parser.c` | `tests/test_parser` | 65.00% (78/120) | below target |
| F1 `parser.c` | `tests/test_parser_spec` | 63.33% (76/120) | below target |
| F2 `executor.c` | `test_executor` | 45.68% (37/81) | below target |
| F3 `pipeline.c` | `tests/test_pipeline` | 57.24% (87/152) | below target |

**F4 clears the NFR-05 target in both files.** Annotated line-by-line evidence for both is saved at `docs/testing/coverage/history.c.gcov.txt` and `docs/testing/coverage/env_manager.c.gcov.txt`.

The uncovered lines in the earlier coverage run include the bodies of `history_print()` and `env_print()`. Their output is instead verified by the integration and SEC-03 tests below. This was a deliberate choice: output-formatting functions are also tested end-to-end rather than relying only on isolated unit tests.

**F1, F2 and F3 are below the 70% target in their earlier dedicated unit suites.** These were recorded as coverage gaps for the respective modules. The latest Linux revalidation figures are provided separately in §8 and supersede earlier figures wherever they differ.

### 4.2 Supplementary: whole compiled `simple-shell` binary, exercised end-to-end

This is extra context, not the NFR-05 metric itself. It shows how much of each file gets touched when the **actual compiled shell binary** is driven through realistic sessions (`test_env_integration` + `test_integration` + `test_sec03_env_exposure`, which spawn `./simple-shell` as a subprocess), as opposed to each module's isolated unit calls.

| File | Lines covered |
|---|---|
| `src/main.c` | 60.06% (185/308) |
| `src/parser.c` | 74.17% (89/120) |
| `src/executor.c` | 51.85% (42/81) |
| `src/pipeline.c` | 38.16% (58/152) |
| `src/history.c` | 52.78% (19/36) |
| `src/env_manager.c` | 76.92% (20/26) |

The lower pipeline/executor numbers here than in their own dedicated suites reflect that the integration scripts do not exercise every background, signal and multi-stage-pipeline code path. `tests/test_pipeline` and `test_executor` cover those directly instead.

---

## 5. SEC-03 — environment-variable exposure

**Requirement (SRS SEC-03 / SEC-OBJ-03):** the shell shall not print or log environment-variable values except via explicit `env`.

### 5.1 Static review

References to `environ`, `getenv`, `env_print`, `env_list`, `setenv` and `unsetenv` were checked across `src/` and `include/`:

```text
src/main.c:115   const char *home = getenv("HOME");     <- value used only in chdir(), never printed

src/main.c:218   env_print();                           <- the ONE explicit `env` builtin call site

src/env_manager.c  (module's own implementation)
```

There is exactly one disclosure path in the reviewed codebase (`env` → `env_print()`).

`HOME`'s value is never passed to any `printf`/`fprintf`/`perror`/`puts` call. On a failed `cd`, `perror("shell: cd")` prints only `strerror(errno)`, not the path.

The `export`/`unset` error paths in the builtin dispatch only echo the variable **name** the user typed, never its value. This was confirmed by reviewing the relevant `fprintf` call sites, where the `=` character is replaced with `\0` before the error is printed and restored afterwards.

### 5.2 Dynamic verification
`tests/test_sec03_env_exposure.c` — **37 assertions pass** (`docs/testing/logs/08_sec03_hardened_validation_20261110.txt`).

Methodology: export a secret value, then run 11 different scenarios (failed `cd`, failed input/output redirection, unknown command, malformed pipeline, malformed `export`, `unset` followed by malformed `export`, `help`, `pwd`, `echo`, and an unrelated pipeline). Each scenario checks that the shell process exits successfully, that the completion marker appears in captured output, and that the secret string never appears.

A **positive control** (`env` after `export`) confirms the methodology itself is sound: the secret does appear there, so the other checks are not passing by accident.

### 5.3 One documented, deliberate exception (not a violation)

`history` prints the user's own previously typed command lines verbatim, including an `export SECRET=value` line if the user typed one.

This is asserted explicitly in the test, with a comment explaining why it is correct: SEC-03 is about the shell not pulling values out of the live environment table **unprompted**. It is not, and cannot reasonably be, a requirement to redact a user's own typed input when they explicitly ask to see their own history.

**Conclusion: SEC-03 is satisfied by the tested scenarios.** No unintended environment-variable disclosure path was identified in the reviewed code, confirmed by static review and live testing with a planted secret across 11 distinct error/output scenarios.

---

## 6. Evidence index

All files are plain text (renamed from `.log`/`.gcov` to `.txt` so they are not swallowed by the repository's existing `*.log`/`*.gcov` gitignore rules, which otherwise target build byproducts elsewhere in the tree).

| File | Contents |
|---|---|
| `docs/testing/logs/01_baseline_make_test_*.txt` | First clean `make clean && make && make test` run on this branch, before any new tests were added. |
| `docs/testing/logs/02_ci_reference_run_42102566649.txt` | Full log pulled from the project's actual GitHub Actions run on the baseline commit — the reference environment per STP §6. Shows `Pipeline tests: 9/9 passed`. |
| `docs/testing/logs/03_coverage_build_and_test_20261106_185033.txt` | First successful coverage-instrumented full run (all 9 test binaries), before the `test_history.c` boundary-test addition. |
| `docs/testing/logs/04_final_coverage_run_*.txt` | Final coverage-instrumented full run, after all test additions. Source for §4's earlier coverage figures. |
| `docs/testing/logs/05_coverage_summary_*.txt` | Condensed per-module `gcov` percentage output. |
| `docs/testing/logs/06_final_production_build_*.txt` | Final non-instrumented (production-config) clean build and full test run. Source for §2 and §5.2. |
| `docs/testing/coverage/history.c.gcov.txt` | Line-by-line annotated coverage for `history.c` (F4). |
| `docs/testing/coverage/env_manager.c.gcov.txt` | Line-by-line annotated coverage for `env_manager.c` (F4). |
| `docs/testing/logs/08_sec03_hardened_validation_20261110.txt` | Fresh full clean-build and test-suite transcript after hardening SEC-03 with child exit-status and scenario-completion checks. Source for the updated 37-assertion SEC-03 result. |
| `docs/testing/logs/09_sec03_hardened_asan_20261110.txt` | Full AddressSanitizer-instrumented test-suite transcript after SEC-03 exit-status and completion-marker hardening. |
| `docs/testing/logs/10_linux_coverage_run_20261110.txt` | Clean Linux coverage-instrumented build and complete automated test execution on 10 November 2026. |
| `docs/testing/coverage/code_coverage_20261110.txt` | Aggregate production-source line and function coverage output from the latest Linux run. |
| `docs/testing/coverage/branch_coverage_20261110.txt` | Aggregate production-source branch coverage output from the latest Linux run. |
| `docs/testing/TEST_CASE_EXECUTION_RESULTS.md` | Execution status and evidence for all 12 planned Test Plan cases, including partial and unexecuted cases. |
| `docs/testing/UNIT_TEST_REPORT.md` | Unit and component test suite results, expected versus actual outcomes, and memory-safety evidence. |
| `docs/testing/SYSTEM_VALIDATION_REPORT.md` | End-to-end workflow validation against functional, security and non-functional requirements. |
| `docs/testing/CODE_COVERAGE_REPORT.md` | Formal aggregate production-source code coverage report. |
| `docs/testing/BRANCH_COVERAGE_REPORT.md` | Formal aggregate production-source branch coverage report. |

---

## 7. Summary / handoff

- **F4:** the history and environment modules passed their earlier dedicated unit tests with 80%+ line coverage on both files.
- **Cross-module integration coverage:** 38 CHECK assertions spanning F2+F3+F4 combinations.
- **SEC-03:** 37 assertions covering the positive control, 11 negative scenarios (including process-exit and scenario-completion checks) and the documented history exception.
- **Platform-specific test issue:** two pipeline-test failures were observed on macOS and documented; the reference CI environment passed the corresponding tests.
- **History-boundary test correction:** an initial test expectation was corrected after identifying that the `history` command itself consumes a history entry.
- **Coverage action items:** earlier dedicated unit-test coverage for F1, F2 and F3 was below the 70% NFR-05 target. The latest Linux coverage status is documented in §8.

---

## 8. Linux revalidation update — 10 November 2026

This update records the latest validation on the Linux development environment.

It supersedes earlier platform-specific test results and coverage figures wherever they differ.

**Branch used for this validation:** `test/f4-testing-and-security`

**Test commit at the time of validation:** `e1d1e63`

The subsequent SEC-03 hardening and final normal/AddressSanitizer reruns were performed on `test/sprint2-testing-security-validation` on 10 November 2026. The corresponding evidence is listed in §6.

**Coverage toolchain:** GCC 13.3.0 and gcovr 7.0.

### 8.1 Automated test results

The complete test suite was rebuilt and executed with AddressSanitizer.

| Test group | Latest result |
|---|---|
| Parser unit tests | Passed |
| History unit tests | Passed |
| Environment manager tests | Passed |
| Executor tests | 12/12 passed |
| Environment integration tests | Passed |
| Pipeline tests | 10/10 passed |
| Parser specification tests | Passed |
| Cross-module integration tests | Passed |
| SEC-03 environment exposure tests | Passed - 37 assertions |

The final AddressSanitizer run completed without sanitizer or LeakSanitizer errors.

The earlier macOS pipeline failures described in §3.1 were not reproduced in the Linux environment. The 10 November full-suite run passed all 10 pipeline tests. The focused 11 November Linux run subsequently passed all 16 pipeline tests, including the added validation, redirection-failure, and signal-termination cases.

### 8.2 Latest module-wise line coverage

| Module | Source files | Lines covered | NFR-05 (70%) |
|---|---|---:|---|
| F1 — Command Interpreter | `src/main.c`, `src/parser.c` | 79.7% (315/395) | Meets target |
| F2 — Process Executor | `src/executor.c` | **73.7% (56/76)** | Meets target |
| F3 — I/O and Pipeline Manager | `src/pipeline.c` | **64.9% (87/134)** | Below target |
| F4 — History and Environment Manager | `src/history.c`, `src/env_manager.c` | 100% (51/51) | Meets target |

F1, F2 and F4 meet the 70% line-coverage target. F3 remains below the NFR-05 target at 64.9%. These results are reported transparently and are not represented as full compliance.

Branch coverage was also measured, but the NFR-05 threshold reported here is line coverage.

The F2 and F3 figures above were remeasured on Linux on 11 November 2026
using GCC coverage instrumentation. For these process-creating modules,
the focused measurement used GCC's coverage-aware fork/exec hooks and an
exit wrapper so child-process coverage was collected. F2 measured 56/76
lines (73.7%), with 68.0% branch coverage. F3 measured 87/134 lines
(64.9%), with 64.0% branch coverage. The focused pipeline suite passed
16/16 tests. F3 remains below the 70% line-coverage target.


### 8.2.1 Aggregate production-source coverage — 10 November 2026

A separate coverage-instrumented full-suite run measured aggregate coverage
across production source files under `src/`, excluding `src/test_executor.c`.

- Line coverage: **67.1% (440/656)**
- Function coverage: **93.1% (27/29)**
- Branch coverage: **61.4% (313/510)**

These aggregate figures use a different measurement scope from the
module-specific F1–F4 results above. The aggregate line coverage is below
70%; this is reported separately and does not replace the module-wise
NFR-05 assessment.

Formal reports:
- `CODE_COVERAGE_REPORT.md`
- `BRANCH_COVERAGE_REPORT.md`

Execution evidence: `docs/testing/logs/10_linux_coverage_run_20261110.txt`.


### 8.3 AddressSanitizer finding and resolution

The first AddressSanitizer run identified a 175-byte allocation leak in `tests/test_sec03_env_exposure.c`, in the test helper output handling.

The test was corrected to free `history_output` exactly once.

The complete AddressSanitizer test suite was rerun after this correction.

All test groups passed without sanitizer or memory-leak errors.

Following the SEC-03 test-harness hardening, the full AddressSanitizer test suite was rerun on 10 November 2026. All test groups passed without AddressSanitizer or LeakSanitizer errors. Evidence: `docs/testing/logs/09_sec03_hardened_asan_20261110.txt`.

### 8.4 Current status

- Executor tests pass: 12/12.
- Pipeline tests pass: 16/16 in the focused 11 November Linux run. The earlier 10 November full-suite run recorded 10/10.
- Full AddressSanitizer test run passes.
- SEC-03 environment-exposure tests pass.
- F1 and F4 meet the NFR-05 line-coverage target.
- F2 meets the NFR-05 line-coverage target at 73.7%; F3 remains below target at 64.9%.
- The earlier 10 November Linux revalidation changes were recorded in commit `e1d1e63`. The focused 11 November pipeline test additions and coverage updates are currently documented in this working tree.
- Full-suite execution transcript: `docs/testing/logs/11_linux_full_test_run_20261111.txt`.
- Focused F2/F3 coverage transcript: `docs/testing/logs/12_f2_f3_focused_coverage_20261111.txt`.

This Linux revalidation is the latest test evidence in this report and should be used when describing the current test status.
---

## 9. Latest Linux revalidation — 11 November 2026

Sections 1–8 retain the earlier results as historical evidence. This
section records the latest Linux validation.

### 9.1 Full-suite results

All nine test groups passed in the fresh Linux run.

- Aggregate line coverage: **67.7% (444/656)**.
- Function coverage: **93.1% (27/29)**.
- Branch coverage: **62.4% (318/510)**.

Compared with 10 November (67.1% line and 61.4% branch coverage),
aggregate line coverage increased by 0.6 percentage points and branch
coverage increased by 1.0 percentage point. Function coverage remained
unchanged.

### 9.2 Latest module-wise line coverage

| Feature | Measurement scope | Result | NFR-05 |
|---|---|---:|---|
| F1 — Command Interpreter | Full suite (`main.c` + `parser.c`) | 315/395 (79.7%) | Meets |
| F2 — Process Executor | Focused, coverage-aware | 56/76 (73.7%) | Meets |
| F3 — I/O and Pipeline Manager | Focused, coverage-aware | 87/134 (64.9%) | Below |
| F4 — History and Environment Manager | Full suite | 51/51 (100%) | Meets |

F3 remains below the 70% target. NFR-05 is therefore not fully satisfied
across all four features.

### 9.3 Explanation of different measurement results

The full-suite run separately measured `executor.c` at 30/76 (39.5%)
and `pipeline.c` at 48/134 (35.8%). These ordinary full-suite figures
do not capture all coverage generated by forked or executed child
processes.

The focused F2/F3 measurement used GCC coverage-aware fork/exec hooks
and an exit wrapper to collect child-process coverage. It measured
73.7% for F2 and 64.9% for F3. These results have a different collection
scope and must not be compared directly with the ordinary full-suite
per-file figures as though they were identical measurements.

Earlier dedicated-suite results also used different toolchains, source
line denominators and test scopes:

- F1's earlier parser-only results (65.00% and 63.33%) covered parser
  tests, whereas the latest 79.7% combines `main.c` and `parser.c`
  under the full suite.
- F2's earlier 45.68% (37/81) and latest focused 73.7% (56/76) use
  different source-line denominators and coverage collection methods.
- F3's earlier 57.24% (87/152) and latest focused 64.9% (87/134) use
  different source-line denominators and measurement setups.
- F4's earlier dedicated-suite results were 80.56% for history and
  80.77% for environment management. The latest full-suite combined
  result is 100% (51/51), including additional integration execution.

These differences do not by themselves establish a regression.
All earlier results remain preserved in their original sections.

### 9.4 Latest test and coverage evidence

- `logs/11_linux_full_test_run_20261111.txt` — complete test-suite run.
- `logs/12_f2_f3_focused_coverage_20261111.txt` — focused F2/F3 coverage.
- `logs/13_linux_full_coverage_run_20261111.txt` — aggregate coverage.
