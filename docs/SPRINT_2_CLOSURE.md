# Sprint 2 Closure Report

**Project:** Team 3 – Simple Shell CLI
**Sprint:** Sprint 2
**Final implementation and validation branch:** `main`

## 1. Sprint 2 Implementation Summary

Sprint 2 extended the shell implementation with process management and integrated functionality.

### Background Process Management
- Implemented background execution for external commands using `&`.
- Background processes are placed in separate process groups.
- Added background built-in execution in child processes to preserve parent shell state.
- Added periodic background child-process reaping using `poll()` and `waitpid(..., WNOHANG)`.

### Signal Handling
- The shell ignores terminal-generated SIGINT so that Ctrl+C does not terminate the shell.
- Foreground child processes restore default SIGINT handling.
- Background processes use separate process groups to isolate them from terminal-generated Ctrl+C.

### Background Pipeline Support
- Implemented asynchronous background pipeline execution.
- All stages of a background pipeline share one separate process group.
- Added gate-pipe synchronization so that pipeline children are released after process-group setup.
- Added cleanup handling for partially created pipelines.

### Integration
- Integrated the process executor, pipeline implementation, parser, history and environment modules into the main shell.
- Updated the main command loop to support background process polling and reaping.

## 2. Final Verification Results

The following commands were executed on the final integrated implementation:

- `make clean`
- `make`
- `make test`

The build completed successfully, and all nine test groups passed:

1. Parser unit tests
2. History unit tests
3. Environment manager tests
4. Executor tests
5. Environment integration tests
6. Pipeline tests
7. Parser specification tests
8. Cross-module integration tests
9. SEC-03 environment exposure tests

The latest main-branch GitHub Actions workflow completed successfully.

### Security and Memory-Safety Validation
- Shell-metacharacter injection validation passed.
- Explicit redirection behavior was tested.
- The complete AddressSanitizer suite completed without sanitizer or LeakSanitizer errors.
- Valgrind was not run; this is not claimed as completed validation.

### Coverage Results

Final aggregate production-source coverage:

- Line coverage: 68.6% (450/656)
- Function coverage: 93.1% (27/29)
- Branch coverage: 62.7% (320/510)

Final module-specific line coverage:

| Module | Line Coverage |
|---|---:|
| F1 | 79.7% |
| F2 | 73.7% |
| F3 (pipeline) | 64.9% |
| F4 | 100% |

F3 remains below the 70% target. The uncovered process-management and operating-system-dependent error paths, along with the limitations of deterministic child-process coverage measurement, are documented in the testing reports. The team attempted additional failure-path tests, but discarded the experimental changes when they did not produce reliable coverage improvement.

### Performance Validation

Three-run measurements recorded:

- Average shell startup time: 1.773 ms
- Average foreground `pwd` completion-to-next-prompt time: 0.332 ms

Both measurements are below the 1-second limits.

NFR-03 passed: the five-stage pipeline completed 10/10 runs without hangs or deadlocks. NFR-04 passed: a first-time user independently executed help, pwd and exit (3/5 core built-ins) in approximately 1.5 minutes without hints.

## 3. Known Limitations

- Full POSIX interactive job control (`jobs`, `fg`, `bg`) is not implemented.
- Terminal foreground process-group handoff is not implemented.
- History is maintained in memory and is not persisted across sessions.
- NFR-05 remains partially unmet: F3 line coverage is 64.9%, below its 70% per-module target; F1, F2 and F4 meet the target.
- Valgrind validation was not performed.

These limitations are documented rather than represented as completed work.

## 4. Sprint Outcome

Sprint 2 implementation and integration are complete on `main`.

The build, all nine automated test groups, the latest CI workflow, documented security checks, and AddressSanitizer validation completed successfully.

Coverage and performance results, including the remaining exceptions, are recorded in the testing documentation. The GitHub Project board has all 14 tracked issues in Done, with Todo and In Progress empty.
