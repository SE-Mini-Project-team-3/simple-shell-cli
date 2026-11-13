# Team 3 – Simple Shell CLI: Demo Commands

This document contains example commands to demonstrate the features of the Simple Shell CLI, including built-in commands, external command execution, I/O redirection, pipelines, environment management, background processes, signal handling & error handling.

Open the ubuntu/wsl terminal & execute the commands

## 1. Build and Launch

```bash
cd ~/simple-shell-cli
make clean
make
./simple-shell
```

## 2. Command Interpreter and Built-in Commands (F1)

```bash
help
pwd
echo Hello from Team 3 Simple Shell
```

### Directory Navigation

```bash
cd /tmp
pwd

cd ~/simple-shell-cli
pwd
```

### External Command Execution

```bash
ls
ls -l
whoami
date
```

## 3. I/O Redirection (F3)

### Output Redirection (`>`)

Creates a file or overwrites its existing contents.

```bash
echo Hello World > /tmp/shell_demo.txt
```

### Input Redirection (`<`)

Reads input from the specified file.

```bash
cat < /tmp/shell_demo.txt
```

### Append Redirection (`>>`)

Appends output to the existing file without overwriting its contents.

```bash
echo Second Line >> /tmp/shell_demo.txt
cat /tmp/shell_demo.txt
```

## 4. Pipelines (F3)

Pipelines pass the output of one command as input to the next command.

### Single-stage Pipeline

```bash
echo hello | cat
```

### Multiple Commands in a Pipeline

```bash
ls -l | wc -l
```

### Three-stage Pipeline

```bash
printf "apple\nbanana\napple\n" | sort | uniq
```

### Five-stage Pipeline

```bash
printf "pear\napple\nbanana\n" | sort | uniq | wc -l | cat
```

## 5. History and Environment Management (F4)

### Command History

```bash
history
```

### Export an Environment Variable

```bash
export PROJECT=SimpleShell
printenv PROJECT
```

### Unset an Environment Variable

```bash
unset PROJECT
printenv PROJECT
```

The final command should indicate that the variable is no longer available.

## 6. Background Process Execution (F2)

### Execute a Command in the Background

```bash
pwd &
echo Parent shell is still running
pwd
```

The shell should return control immediately instead of waiting for the background process to finish.

### Background Sleep Process

```bash
sleep 15 &
echo Shell remains responsive
```

### Inspect the Background Process

Open a second terminal (T2) and execute:

```bash
ps -o pid,ppid,pgid,stat,cmd -p PID
```

Replace `PID` with the actual process ID printed by the shell.

## 7. Signal Handling (F2)

### Foreground Process Interruption

In the shell terminal (T1), execute:

```bash
sleep 20
```

Press **Ctrl+C** to interrupt the foreground process.

Then execute:

```bash
echo Shell survived Ctrl+C
```

The shell should remain operational.

Press **Ctrl+C once more at the prompt** to demonstrate prompt recovery.

## 8. Background Pipeline Execution (F2 + F3)

In T1, execute:

```bash
sleep 20 | cat &
echo Background pipeline returned control immediately
```

The shell should return to the prompt without waiting for the pipeline to finish.

In T2, inspect the pipeline's process group:

```bash
ps -eo pid,ppid,pgid,stat,cmd | awk 'NR == 1 || $3 == PID'
```

Replace `PID` with the process group ID printed by the shell for the background pipeline.

## 9. Background Process Reaping (F2)

In T1, execute:

```bash
sleep 10 &
```

Wait approximately 3 seconds.

In T2, inspect the process:

```bash
ps -o pid,ppid,pgid,stat,cmd -p PID
```

Replace `PID` with the background process ID.

After the sleep command finishes, inspect the same PID again. The completed process should have been reaped by the shell.

## 10. Error Handling and Recovery

### Invalid Command

```bash
invalid_command_xyz
```

### Non-existent File

```bash
cat nonexistent_file.txt
```

### Verify Shell Recovery

```bash
history
```

The shell should remain operational after handling invalid commands and execution errors.

## 11. Exit the Shell

```bash
exit
```

After returning to the normal terminal, remove the temporary demo file:

```bash
rm -f /tmp/shell_demo.txt
```

---

## Notes

- Run commands in the Simple Shell terminal unless otherwise specified.
- T1 refers to the terminal running the Simple Shell.
- T2 refers to a separate terminal used for process inspection.
- Replace every `PID` placeholder with the appropriate process ID or process group ID printed during execution.
- Some background processes may finish before they are inspected. Run the inspection commands promptly.
- The examples demonstrate the implemented functionality of the Team 3 Simple Shell CLI.

*Author*
*Rekha Dhorigol*