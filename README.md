# Processes-Shell: Building Your Own Shell in C

## Project Approach

I started the project by reviewing the project requirements, tackling the basic functionality and gradually adding complexity. A rough outline for those steps is as follows:

- Accept commands in **interactive** and **batch** mode 
- Support **built-in commands** (`exit`, `cd`, `path`) 
- Handle **output redirection** using `>` 
- Support **concurrent execution** with `&` 
- Search for **executables in specified paths**

---

## Setting Up the Project Structure

I began by setting up the basic `main()` function that could switch between:

- **Interactive mode**: taking input from `stdin` 
- **Batch mode**: reading commands from a file 
---

## Reading and Tokenizing Input

Implemented `cell_read_line()` to read full lines from the input source. Then created two parsing functions:

- `cell_split_commands()`: splits input lines by `&` (for concurrent execution) 
- `cell_split_line()`: tokenizes each command string by whitespace 

---

## Handling Built-in Commands

To avoid unnecessary process creation, I handled built-in commands before forking:

- `exit`: exits the shell 
- `cd`: used `chdir()` to change the directory 
- `path`: maintained a list of shell paths which could be updated 

All of this logic was implemented in `cell_built_ins()`.

---

## Implementing Executable Resolution

Created `cell_check_executable()` to loop through `shell_paths` and use `access()` to find the correct executable path.

---

## Adding Redirection Support

Implemented output redirection in two parts:

- `cell_search_redirect()`: detects the `>` operator 
- `cell_validate_redirect()`: validates syntax and extracts the output file 

In the child process, I used `open()` and `dup2()` to redirect `stdout` and `stderr` to the specified file.

---

## Normalizing Redirection

To handle cases where `>` had no spaces (e.g. `ls>out.txt`), I created `normalize_redirect()` to insert spaces around `>` so it could be parsed like any other token.

---

## Adding Concurrency with Threads

To support multiple concurrent commands:

- Defined a `Command` struct with `args` and `pthread_t` 
- Created threads using `pthread_create()` for each command 
- Executed commands inside `cell_handle_command()` which handled forking and redirection 
- Used `pthread_join()` to wait for all threads to finish 

This ensured each command ran independently.

---

## Memory Management

I allocated memory dynamically for tokens and command structs, and ensured everything was freed after each command batch:

- Freed command arguments and structs after joining threads 
- Freed `line` input string every loop iteration 
- Used `strdup()` for duplicating strings safely 

---

## Testing & Debugging

I tested the shell in both interactive and batch mode:

- Ran single and multiple commands 
- Checked output redirection 
- Tried edge cases like missing files, empty inputs, and malformed redirection 
- Ensured commands executed concurrently when using `&`