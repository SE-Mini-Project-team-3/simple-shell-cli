# Sprint 2 Closure Report

**Project:** Team 3 – Simple Shell CLI  
**Sprint:** Sprint 2  
**Baseline commit:** `b2c4270`  
**Baseline branch:** `main`

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

## 2. Verification Results

The following commands were executed on the latest `main` branch:

- `make clean`
- `make`
- `make test`

The build completed successfully.

All test groups passed:
- Parser tests
- History tests
- Environment manager tests
- Executor tests
- Environment integration tests
- Pipeline tests (9/9)
- Parser specification tests

The GitHub Actions workflow was also checked on the latest main-branch baseline.

## 3. Known Limitations and Pending Validation

- Full POSIX interactive job control (`jobs`, `fg`, `bg`) is not implemented.
- Terminal foreground process-group handoff is not implemented.
- History is maintained in memory and is not persisted across sessions.
- The 70% line-coverage target requires separate measurement.
- Complete security and memory-safety validation evidence remains pending.
- Startup and command-response performance measurements remain pending.

## 4. Sprint Outcome

Sprint 2 implementation has been integrated into the `main` branch.

Completed implementation work is recorded in the GitHub Project board. Remaining usability, coverage, security and performance validation tasks are tracked separately.

The `main` branch at commit `b2c4270` is the Sprint 2 implementation baseline.