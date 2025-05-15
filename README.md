
# Command Executor with Security & Resource Management

This project implements a command-line program that executes user commands with extended functionality for:

- Blocking dangerous commands.
- Handling background execution.
- Supporting `|` pipes and output redirection (`2>`).
- Custom implementation of `tee` (`my_tee`).
- Tracking execution times.
- Handling system signals and resource limits (`rlimit`).

## 📦 Features

✅ Executes commands from standard input  
✅ Filters "dangerous commands" using a user-defined file  
✅ Supports background execution using `&`  
✅ Detects and handles commands with `|` (pipes)  
✅ Custom support for `2>` (stderr redirection)  
✅ Implements `my_tee` for redirecting outputs to multiple files  
✅ Handles signals: `SIGXCPU`, `SIGXFSZ`, `SIGSEGV`, `SIGCHLD`  
✅ Applies `rlimit` configurations from special input commands  
✅ Logs command execution times and statistics

---

## 📁 Files

- `ex2.c` – Main C file with the program logic
- `README_ex2.txt` – Basic usage documentation (superseded by this README)

---

## 🧠 How It Works

### 1. **Startup**
The program expects two arguments:
```bash
./ex2 <dangerous_commands.txt> <log_file.txt>
```

- **dangerous_commands.txt**: A file with commands that should be blocked for security.
- **log_file.txt**: File where execution logs and statistics are stored.

### 2. **Command Input**
Commands are read from `stdin`. Before execution, each command is:

- Validated for extra spaces or too many arguments.
- Checked if it matches a known dangerous command.
- Parsed into arguments (up to 6 args supported).
- Checked for `|` (pipe), `2>` (stderr redirect), or `&` (background).

### 3. **Resource Limit Commands**
Special commands like:
```bash
rlimit set <resource>=<value>
```
are detected and applied using `setrlimit`.

### 4. **Execution**
Depending on the type of command:
- It runs using `fork()` and `execvp()`.
- If `my_tee` is detected, a custom function is invoked.
- If piped, both sides are executed with proper piping.
- Background commands don't block the main thread.

### 5. **Logging**
Each command's:
- Execution time (real)
- Status (exit code or signal)
- Resource usage

…are saved to the provided log file.

---

## 🧪 Example Usage

```bash
$ ./ex2 dangerous.txt log.txt

> ls -l
> my_tee file1.txt file2.txt
> rlimit set cpu=2
> cat bigfile.txt | grep error
> dangerous_command     # Blocked!
> ls not_a_file 2> err.txt
> ./infinite_loop &     # Background
```

---

## ⚙️ Compilation

Use `gcc` to compile:

```bash
gcc -o ex2 ex2.c
```

---

## 🚨 Signals Handled

- `SIGCHLD` – Cleans up zombie processes
- `SIGXCPU` – CPU time exceeded
- `SIGXFSZ` – File size limit exceeded
- `SIGSEGV` – Invalid memory access
- `SIGXFILE` – Too many open files (custom handler)

---

## 📊 Logging Format (log.txt)

Each command is logged with:

- Command string
- Start time and end time
- Duration in seconds
- Min/Max/Average duration (over time)
- Status (exit code or signal)

---

## 📎 Limitations

- Up to 6 arguments per command (`MAX_ARGS`)
- No support for `&&` or `||` command chaining
- No command history or autocomplete
---

## 👨‍💻 Author

> Developed as part of a systems programming assignment focused on signals, forking, resource limits, and process control in C.

---

## 📄 License

This project is provided for educational purposes. No warranty or guarantee is given.
