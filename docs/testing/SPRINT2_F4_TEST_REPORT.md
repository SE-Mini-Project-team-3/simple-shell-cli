# Sprint 2 Testing & Security Report — F4 (History & Environment) + Test Coordination

**Author:** Sanjana Subrahmanya Aithal (F4 owner)
**Scope:** History & environment tests, cross-module integration testing, coordinating/running the complete automated test suite, collecting test logs/defects/evidence, consolidating coverage, and SEC-03 (environment-variable exposure) validation.
**Baseline:** `main` at `9b7cf66` ("docs: add sprint 2 closure report")
**Machine:** macOS (Darwin 25.6.0, arm64). Reference test environment per STP §6 is Ubuntu/GCC — platform differences are called out explicitly below wherever relevant.

Every number in this report was produced by an actual command run on this branch; the raw output is saved under `docs/testing/logs/` and `docs/testing/coverage/` and is referenced by filename throughout. Nothing here is asserted without a corresponding log.

---

## 1. What was added this pass

| File | Purpose |
|---|---|
| `tests/test_history.c` (extended) | Added `test_out_of_range_access()`: asserts `history_get`/`history_get_number` reject negative and at-capacity positions. Closed a real unit-coverage gap (see §4). |
| `tests/test_integration.c` (new) | Cross-module integration tests. Drives the *compiled shell binary*, not isolated module calls, through sequences spanning History + Environment + Pipeline/Redirection + Process Execution together: `history` output format (FR-HE-02), export visible inside a pipeline stage (F3+F4), `env` output redirection (F3+F4), export visible to a background external process (F2+F4), and the 50-entry history boundary exercised through the real parser→dispatcher path (FR-HE-01/03), not direct `history_add()` calls. |
| `tests/test_sec03_env_exposure.c` (new) | SEC-03 validation: plants a secret env value, exercises 11 different error/builtin paths, and asserts the secret never leaks except via the one sanctioned `env` disclosure path. Includes a positive control. |
| `Makefile` (extended) | Wired the two new test binaries into `make test`, and `clean`. |

All new test code was built and run repeatedly during development; failures were investigated and root-caused before being accepted or fixed (see §3 for two genuine bugs found and fixed in the *tests*, not silently special-cased).

---

## 2. Full automated test suite — results

Command: `make clean && make && make <each test target> && <run each binary>` (see `docs/testing/logs/06_final_production_build_*.txt` for the complete, unedited transcript of this exact run, non-instrumented/production config).

| Test binary | Module(s) | Result | Notes |
|---|---|---|---|
| `tests/test_parser` | F1 |  pass | |
| `tests/test_parser_spec` | F1 |  pass | US-01 acceptance spec |
| `tests/test_history` | F4 |  pass | 15/15, incl. 4 new boundary checks |
| `tests/test_env_manager` | F4 |  pass | |
| `test_executor` | F2 |  pass | |
| `tests/test_env_integration` | F4 (via real shell) |  pass | |
| `tests/test_pipeline` | F3 |  7/9 | 2 failures — platform-specific, see §3 |
| `tests/test_integration` | F2+F3+F4 (new) |  pass | 18/18 checks |
| `tests/test_sec03_env_exposure` | F4 / security (new) |  pass | 18/18 checks |

**9 binaries run, 8 fully green, 1 with 2 known-platform failures out of 9 sub-checks.** Nothing here is marked passed without the corresponding log.

---

## 3. Defects found

### 3.1 `tests/test_pipeline.c` — 2 failures on macOS, confirmed passing on the reference CI environment

```
FAIL: file descriptor count changed (before=-1, after=-1)
FAIL: background pipeline process-group isolation (status=0, pid_files=1, group_check=0, reaped=2)
```

- **Root cause 1 (fd-count test):** `count_open_fds()` in `tests/test_pipeline.c` opens `/proc/self/fd`, which does not exist on macOS/BSD (it's Linux-specific). `opendir()` returns `NULL`, the function returns `-1` for both `before` and `after`, and the check `before >= 0 && before == after` fails on the `>= 0` guard — not because of an actual descriptor leak.
- **Root cause 2 (process-group test):** `getpgid()`-based process-group checks return `group_check_ok = 0` consistently (reproduced 3/3 runs, not flaky) — a real macOS vs. Linux pgid/signal-semantics difference in how the test observes the child's process group, not a crash or hang.
- **This is not my module (F3, Samika's), so I did not modify `pipeline.c` or the test.** I verified instead, with evidence, that this is an environment artifact and not a functional regression:
  - Local failure log: `docs/testing/logs/06_final_production_build_*.txt`
  - **Reference-environment (Ubuntu, GitHub Actions) log pulled directly from the project's own CI, same baseline commit:** `docs/testing/logs/02_ci_reference_run_42102566649.txt` — shows **`Pipeline tests: 9/9 passed`**.
- **Recommendation:** flag to Samika (F3 owner) as a test-portability improvement (use `/dev/fd` as a fallback, or skip/guard the fd-count check when `/proc/self/fd` is unavailable) — not blocking, since the reference environment passes clean.

### 3.2 Bug found and fixed in my own new test (`test_integration.c`) during development

My first version of the 55-command history-boundary test asserted the oldest surviving entry would be `cmd6`. It failed on first run — investigation showed the `history` command itself is a successfully-parsed command line, so running it **also** consumes a history slot (recorded before it prints). With 55 `echo` commands + the `history` invocation itself = 56 entries added against a 50-entry cap, the correct oldest surviving entry is `cmd7`, with `history`'s own invocation occupying the 50th (newest) slot. This is correct FR-HE-01 behavior, not a shell bug — I fixed the test's expectations to assert this precisely (see the comment in `test_history_boundary_through_real_shell()`), rather than loosening the assertion to hide the discrepancy.

No other defects were found in F4-owned code during this pass.

---

## 4. Coverage (NFR-05: ≥70% line coverage per module's own unit test suite)

Toolchain note: `make`'s recipe shell does not read this machine's `gcc` alias, so `make`-driven builds use Apple clang via `/usr/bin/gcc` — confirmed by `.gcno` format version. Coverage was measured with the matching `/usr/bin/gcov`. Full build+run transcript: `docs/testing/logs/04_final_coverage_run_*.txt`; summary command output: `docs/testing/logs/05_coverage_summary_*.txt`.

### 4.1 Per-module dedicated unit test suite (the NFR-05 metric)

| Module | Owner | Suite | Lines covered | **vs. 70% target** |
|---|---|---|---|---|
| F4 `history.c` | Sanjana | `tests/test_history` | **80.56%** (29/36) |  pass |
| F4 `env_manager.c` | Sanjana | `tests/test_env_manager` | **80.77%** (21/26) |  pass |
| F1 `parser.c` | Rekha | `tests/test_parser` | 65.00% (78/120) |  below target |
| F1 `parser.c` | Rekha | `tests/test_parser_spec` | 63.33% (76/120) |  below target |
| F2 `executor.c` | Rohan | `test_executor` | 45.68% (37/81) |  below target |
| F3 `pipeline.c` | Samika | `tests/test_pipeline` | 57.24% (87/152) |  below target |

**F4 (my own module) clears the NFR-05 target in both files.** Annotated line-by-line evidence for both is saved at `docs/testing/coverage/history.c.gcov.txt` and `docs/testing/coverage/env_manager.c.gcov.txt`. The only uncovered lines in either file are `history_print()` and `env_print()`'s bodies — their *output* is instead verified by the integration/SEC-03 tests below (a deliberate choice: output-formatting functions are better asserted end-to-end than by instrumenting stdout in a unit test), plus the two boundary-check lines added in §1/§3.2 area... (now covered).

**F1, F2, F3 are below the 70% target in their own dedicated suites.** This is not something I fixed — not my modules, and I don't have context on which paths each owner considers already-validated by other means. Flagging as an action item for each owner; happy to pair on closing specific gaps if useful.

### 4.2 Supplementary: whole compiled `simple-shell` binary, exercised end-to-end

This is extra context, not the NFR-05 metric itself — it shows how much of each file gets touched when the *actual shipped binary* is driven through realistic sessions (`test_env_integration` + `test_integration` + `test_sec03_env_exposure`, which spawn `./simple-shell` as a subprocess), as opposed to each module's isolated unit calls.

| File | Lines covered |
|---|---|
| `src/main.c` | 60.06% (185/308) |
| `src/parser.c` | 74.17% (89/120) |
| `src/executor.c` | 51.85% (42/81) |
| `src/pipeline.c` | 38.16% (58/152) |
| `src/history.c` | 52.78% (19/36) |
| `src/env_manager.c` | 76.92% (20/26) |

(Lower pipeline/executor numbers here than their own dedicated suites reflect that my three integration scripts don't exercise every background/signal/multi-stage-pipeline code path — `tests/test_pipeline` and `test_executor` cover those directly instead.)

---

## 5. SEC-03 — environment-variable exposure

**Requirement (SRS SEC-03 / SEC-OBJ-03):** the shell shall not print or log environment-variable values except via explicit `env`.

### 5.1 Static review

Grepped every reference to `environ`, `getenv`, `env_print`, `env_list`, `setenv`, `unsetenv` across `src/` and `include/`:

```
src/main.c:115   const char *home = getenv("HOME");     <- value used only in chdir(), never printed
src/main.c:218   env_print();                            <- the ONE explicit `env` builtin call site
src/env_manager.c  (module's own implementation)
```

There is exactly one disclosure path in the entire codebase (`env` → `env_print()`). `HOME`'s value is never passed to any `printf`/`fprintf`/`perror`/`puts` call — on a failed `cd`, `perror("shell: cd")` prints only `strerror(errno)`, not the path. The `export`/`unset` error paths (`src/main.c` builtin dispatch) only ever echo back the variable **name** the user typed, never a value — confirmed by reading the exact `fprintf` call sites, where the `=` character is replaced with `\0` *before* the error is printed and restored *after*.

### 5.2 Dynamic verification

`tests/test_sec03_env_exposure.c` — 18/18 checks pass (`docs/testing/logs/06_final_production_build_*.txt`). Methodology: export a secret value, then run 11 different scenarios (failed `cd`, failed input/output redirection, unknown command, malformed pipeline, malformed `export`, `unset` + malformed `export`, `help`, `pwd`, `echo`, an unrelated pipeline) and assert the secret string never appears in output. A **positive control** (`env` after `export`) confirms the methodology itself is sound — the secret does appear there, so the other checks aren't passing by accident (e.g. a broken test harness that never actually captures output).

### 5.3 One documented, deliberate exception (not a violation)

`history` prints the user's own previously-typed command lines verbatim, including an `export SECRET=value` line if the user typed one. This is asserted explicitly in the test (not avoided) with a comment explaining why it's correct: SEC-03 is about the shell not pulling values out of the live environment table *unprompted*; it is not, and cannot reasonably be, a requirement to redact a user's own typed input when they explicitly ask to see their own history.

**Conclusion: SEC-03 is satisfied.** No unintended environment-variable disclosure path exists, confirmed both by static code review and by live testing with a planted secret across 11 distinct error/output scenarios.

---

## 6. Evidence index

All files are plain text (renamed from `.log`/`.gcov` to `.txt` so they aren't swallowed by the repo's existing `*.log`/`*.gcov` gitignore rules, which otherwise target build byproducts elsewhere in the tree).

| File | Contents |
|---|---|
| `docs/testing/logs/01_baseline_make_test_*.txt` | First clean `make clean && make && make test` run on this branch, before any new tests were added. |
| `docs/testing/logs/02_ci_reference_run_42102566649.txt` | Full log pulled from the project's actual GitHub Actions run on this baseline commit — the reference environment per STP §6. Shows `Pipeline tests: 9/9 passed`. |
| `docs/testing/logs/03_coverage_build_and_test_20261106_185033.txt` | First successful coverage-instrumented full run (all 9 test binaries), before the `test_history.c` boundary-test addition. |
| `docs/testing/logs/04_final_coverage_run_*.txt` | Final coverage-instrumented full run, after all test additions. Source for §4's numbers. |
| `docs/testing/logs/05_coverage_summary_*.txt` | Condensed per-module `gcov` percentage output. |
| `docs/testing/logs/06_final_production_build_*.txt` | Final non-instrumented (production-config) clean build + full test run. Source for §2 and §5.2. |
| `docs/testing/coverage/history.c.gcov.txt` | Line-by-line annotated coverage for `history.c` (F4). |
| `docs/testing/coverage/env_manager.c.gcov.txt` | Line-by-line annotated coverage for `env_manager.c` (F4). |

---

## 7. Summary / handoff

- **F4 (my module):** fully tested, 80%+ unit coverage on both files, no defects found, SEC-03 verified compliant with both static and dynamic evidence.
- **New cross-module integration coverage:** 18 new checks spanning F2+F3+F4 combinations that didn't exist before this pass.
- **One real defect found and documented** (pipeline tests, platform-specific, not blocking — confirmed green on the actual reference CI environment).
- **One bug found and fixed in my own new test** during development (history-boundary expectation), documented rather than silently patched over.
- **Action items for other owners:** F1/F2/F3 are below the 70% NFR-05 target in their own dedicated unit suites (§4.1) — flagging for Rekha/Rohan/Samika respectively, not fixed here since it's outside F4 and I don't have context on each module's intended remaining test scope.

---

## 8. Linux revalidation update — 10 November 2026

This update records the latest validation on the Linux development environment.
It supersedes earlier platform-specific test results and coverage figures
wherever they differ.

**Branch:** `test/f4-testing-and-security`
**Test commit:** `eb9d7a4`
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
| SEC-03 environment exposure tests | Passed |

The final AddressSanitizer run completed without sanitizer or
LeakSanitizer errors.

The earlier macOS pipeline failures described in §3.1 were not reproduced
in the Linux environment. All 10 current pipeline tests passed, including
file-descriptor handling and background process-group checks.

### 8.2 Latest module-wise line coverage

| Module | Source files | Lines covered | NFR-05 (70%) |
|---|---|---:|---|
| F1 — Command Interpreter | `src/main.c`, `src/parser.c` | 79.7% (315/395) | Meets target |
| F2 — Process Executor | `src/executor.c` | 61.9% (52/84) | Below target |
| F3 — I/O and Pipeline Manager | `src/pipeline.c` | 52.45% (75/143) | Below target |
| F4 — History and Environment Manager | `src/history.c`, `src/env_manager.c` | 100% (51/51) | Meets target |

F1 and F4 meet the 70% line-coverage target. F2 and F3 remain below
the NFR-05 target. These gaps are reported transparently and are not
represented as full compliance.

Branch coverage was also measured, but the NFR-05 threshold reported
here is line coverage.

### 8.3 AddressSanitizer finding and resolution

The first AddressSanitizer run identified a 175-byte allocation leak
in `tests/test_sec03_env_exposure.c`, in the test helper output handling.

The test was corrected to free `history_output` exactly once.
The complete AddressSanitizer test suite was rerun after this correction.
All test groups passed without sanitizer or memory-leak errors.

### 8.4 Current status

- Executor tests pass: 12/12.
- Pipeline tests pass: 10/10.
- Full AddressSanitizer test run passes.
- SEC-03 environment-exposure tests pass.
- F1 and F4 meet the NFR-05 line-coverage target.
- F2 and F3 remain below the NFR-05 line-coverage target.
- Test changes are committed in `eb9d7a4`.

This Linux revalidation is the latest test evidence and should be used
when describing the current test status.
