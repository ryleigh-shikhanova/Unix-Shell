# Custom Unix Shell

A small Unix-style command shell written in C as part of an Operating Systems course.

The shell implements command parsing, built-in commands, executable lookup through the user's `PATH`, process creation with `fork()`, program execution with `execve()`, and parent/child synchronization with `waitpid()`.

## Features

- Custom shell prompt displaying the current user and working directory
- Command parsing with support for command-line arguments
- Built-in commands:
  - `cd`
  - `exit`
  - `quit`
  - `logout`
  - `kill`
  - `list`
  - `pid`
  - `printenv`
  - `setenv`
  - `pwd`
  - `which`
  - `which -a`
- Executable lookup using a linked-list representation of the user's `PATH`
- Absolute and relative executable path support
- Process creation using `fork()`
- Program execution using `execve()`
- Parent process synchronization using `waitpid()`
- Signal delivery using `kill()`
- Directory traversal using the Linux `getdents` system call
- Custom environment variables passed to child processes
- Error handling for invalid commands and system-call failures

## Project Structure

```text
custom_shell/
├── CMakeLists.txt
├── include/
│   └── get_path.h
└── src/
    ├── get_path.c
    └── main.c
