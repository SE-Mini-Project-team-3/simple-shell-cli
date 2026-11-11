# Team 3 — Simple Shell CLI

A lightweight command-line shell implemented in **C (C11)** as part of the Software Engineering Mini Project at PES University.

The project implements core shell functionality, including built-in commands, external command execution, input/output redirection, pipelines, background execution, command history, and environment-variable management.

---

## Team Members

| Name | SRN |
|---|---|
| Rekha Dhorigol | PES1UG24CS370 |
| Rohan Rampura Praveen Kumar | PES1UG24CS382 |
| Samika Ojha | PES1UG24CS412 |
| Sanjana Subrahmanya Aithal | PES1UG24CS421 |

---

## Features

### 1. Command Execution
- Executes external commands using process creation and execution.
- Supports foreground and background command execution.
- Handles invalid commands and abnormal child-process termination.

### 2. Built-in Commands
The shell supports built-in commands such as:

| Command | Description |
|---|---|
| `cd` | Changes the current working directory. |
| `pwd` | Displays the current working directory. |
| `echo` | Prints text to the terminal. |
| `help` | Displays shell usage information. |
| `exit` | Exits the shell. |
| `history` | Displays previously entered commands. |
| `export` | Sets or exports environment variables. |
| `unset` | Removes an environment variable. |
| `env` | Displays environment variables. |

### 3. Input and Output Redirection
Supports:
- Input redirection using `<`
- Output redirection using `>`
- Append redirection using `>>`

### 4. Pipeline Execution
- Supports connecting multiple commands using `|`.
- Supports pipelines of up to **five stages**.
- Handles pipeline execution, stage failures, and process groups.
- Includes checks for file-descriptor leaks.

### 5. Background Execution
- Supports executing commands in the background using `&`.
- Handles background pipeline execution and process-group management.

### 6. Command History
- Maintains a command history of up to **50 entries**.
- Assigns command numbers and evicts older entries when the limit is exceeded.

### 7. Input Validation and Error Handling
- Enforces input-length and argument-count limits.
- Rejects malformed command syntax, invalid redirections, and unterminated quotes.
- Handles invalid pipeline stages and execution errors.

---

## Technology Stack

| Component | Technology |
|---|---|
| Programming language | C (C11) |
| Compiler | GCC |
| Build system | GNU Make |
| Development environment | Ubuntu 24.04 / WSL |
| Version control | Git and GitHub |
| Automated testing | C unit tests and integration tests |
| Coverage measurement | GCC coverage tools |
| Memory-safety validation | AddressSanitizer |

---

## Project Structure

```text
simple-shell-cli/
│
├── .github/
│   └── workflows/
│       └── ci.yml
│
├── include/
│   ├── shell.h
│   ├── parser.h
│   ├── executor.h
│   └── pipeline.h
│
├── src/
│   ├── main.c
│   ├── parser.c
│   ├── executor.c
│   ├── pipeline.c
│   ├── history.c
│   └── env_manager.c
│
├── tests/
│   ├── test_parser.c
│   ├── test_parser_spec.c
│   ├── test_history.c
│   ├── test_env_manager.c
│   ├── test_executor.c
│   ├── test_pipeline.c
│   ├── test_env_integration.c
│   ├── test_integration.c
│   └── test_sec03_env_exposure.c
│
├── docs/
│   ├── testing/
│   ├── videos/
│   └── project documentation and sprint deliverables
│
├── Makefile
└── README.md
```

---

## Prerequisites

The project is designed for a Linux/POSIX environment.

Install the following tools:

- GCC compiler
- GNU Make
- Git

On Ubuntu or WSL, install the required packages using:

```bash
sudo apt update
sudo apt install build-essential git
```

---

## Build and Run

### 1. Clone the repository

```bash
git clone https://github.com/SE-Mini-Project-team-3/simple-shell-cli.git
cd simple-shell-cli
```

### 2. Build the project

```bash
make
```

This compiles the source files and generates the `simple-shell` executable.

### 3. Run the shell

```bash
./simple-shell
```

You can now enter commands into the shell.

### 4. Exit the shell

```bash
exit
```

---

## Running Tests

The project includes unit tests, component tests, integration tests, and security-related validation tests.

To build and execute the complete automated test suite:

```bash
make test
```

The test target runs the following nine test groups:

1. Parser unit tests
2. History unit tests
3. Environment manager tests
4. Executor tests
5. Environment integration tests
6. Pipeline tests
7. Parser specification tests
8. Cross-module integration tests
9. SEC-03 environment-exposure tests

To remove generated object files, executables, and coverage files:

```bash
make clean
```

---

## Testing and Validation

The project was validated on **Ubuntu 24.04 (WSL)** using GCC 13.3.0 and GNU Make 4.3.

The latest recorded full Linux test run on **11 November 2026** passed all nine automated test groups.

Additional validation includes:

- Parser boundary and malformed-input tests.
- Foreground and background process execution tests.
- Input, output, and append redirection tests.
- Two-stage, three-stage, and maximum five-stage pipeline tests.
- Command-history boundary and eviction tests.
- Environment-variable inheritance and integration tests.
- Security-related environment-exposure tests.
- AddressSanitizer validation of the full test suite.
- Startup and command-response latency measurements.
- Ten-run five-stage pipeline no-hang validation and first-time-user usability walkthrough.
- Cppcheck 2.13.0 analysis across 15 source and test files, with no error- or warning-severity findings.

### Latest Coverage Results

| Measurement | Result |
|---|---:|
| Aggregate line coverage | 68.6% |
| Aggregate function coverage | 93.1% |
| Aggregate branch coverage | 62.7% |
| F1 module line coverage | 79.7% |
| F2 module line coverage | 73.7% |
| F3 module line coverage | 64.9% |
| F4 module line coverage | 100% |

**Coverage note:** The F3 pipeline module remains below the 70% line-coverage target. This limitation is documented in the testing reports. The team has retained the measured result rather than presenting it as meeting the target.

### Performance Measurements

The documented three-run measurements recorded:

- Average shell startup time: **1.773 ms**
- Average foreground `pwd` completion-to-next-prompt time: **0.332 ms**

Both measurements are below the documented one-second response-time target.

NFR-03 passed: the five-stage pipeline completed 10/10 runs without hangs or deadlocks. NFR-04 passed: a first-time user independently completed three core built-ins within approximately 1.5 minutes.

---

## Documentation and Deliverables

The `docs/` directory contains project documentation and supporting evidence, including:

- Project documentation and sprint deliverables.
- Testing and validation reports.
- Unit and integration test results.
- Code and branch coverage reports.
- Security and memory-safety validation logs.
- Static analysis report and Cppcheck evidence.
- Performance measurement evidence.
- Sprint demonstration videos.

Refer to the relevant documents under `docs/` for detailed implementation and validation information.

---

## Collaboration and Version Control

The project was developed collaboratively using Git and GitHub.

The team used:
- Feature branches for individual development tasks.
- Pull requests for code integration and review.
- GitHub Issues for task tracking.
- GitHub Projects for sprint and work-item management.
- GitHub Actions for automated workflow checks.

Completed work was integrated into the repository's `main` branch through pull requests.

---

## Project Status

The project implementation, documented deliverables, and tracked GitHub issues have been completed.

- All 14 tracked GitHub issue cards are in the **Done** column.
- No cards remain in Todo or In Progress.
- All nine automated test groups passed in the latest recorded full Linux test run.
- Known coverage and validation limitations are documented in the repository.

---

**Developed by Team 3**
**Software Engineering Mini Project — PES University**
