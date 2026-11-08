# C++ Coding Rules Spec

This document defines mandatory rules for all C++ code in this project.

Last modified: 2026-11.


## A) Language Standard

- The compiler **must** be invoked with `-std=c++17`.
- No other standard version (older or newer) is permitted in build flags.

## B) Fixed-Width Type Aliases

- Code **must not** use built-in/standard integer types directly, including
  (but not limited to): `int`, `unsigned int`, `short`, `long`, `long long`,
  `size_t`, `ssize_t`, `int8_t`, `uint8_t`, `int32_t`, `uint32_t`, `int64_t`,
  `uint64_t`.
- Instead, use the following project-defined type aliases:

  | Alias   | Meaning                          |
  |---------|-----------------------------------|
  | `f32`   | 32-bit floating point             |
  | `f64`   | 64-bit floating point             |
  | `i8`    | 8-bit signed integer              |
  | `u8`    | 8-bit unsigned integer            |
  | `i32`   | 32-bit signed integer             |
  | `u32`   | 32-bit unsigned integer           |
  | `i64`   | 64-bit signed integer             |
  | `u64`   | 64-bit unsigned integer           |
  | `isize` | signed pointer-sized integer      |
  | `usize` | unsigned pointer-sized integer    |

- These aliases must be defined once in a common header and included wherever
  needed.
- `char` is permitted only for raw byte/string literal handling (e.g.
  `const char*` string literals), not as a numeric type.

NOTE: Existing code might not follow this section. So keep as is.

## C) No Reference Parameters

- No function signature (free function, member function, lambda, etc.) may
  take a parameter by lvalue reference (`T&`) or rvalue reference (`T&&`).
- This applies to all parameter types, including fundamental types, pointers,
  and class/struct types.
- Pass by value or by pointer (`T*` / `const T*`) instead.

## D) No Pass-by-Value for Class Objects

- No function signature may take a `class` (or `struct`) object as a
  by-value parameter.
- Class/struct objects **must** be passed as pointers (`T*` or `const T*`),
  unless the type appears on the **allowed-by-value list** below.
- This rule does not apply to fundamental/alias types from section (B)
  (e.g. `i32`, `f64`, `u8`), raw pointers, or enums — only to class/struct
  types.

### Allowed-by-value list

Types explicitly exempt from rule (D) and permitted to be passed by value:

- `std::string_view` — a non-owning pointer + length; cheaper to copy than to
  indirect through.
- Cheap POD structs — trivially copyable aggregates of a few scalar fields
  (at most 16 bytes, e.g. `struct Size { i32 rows; i32 cols; }`), used like a
  primitive value. A struct that owns resources or holds a container is not
  cheap POD.

Any other addition to this list requires explicit approval and a documented
justification, added here as `TypeName — justification`.

## E) Error Handling

- Exceptions **must not** be used: no `throw`, `try` or `catch`.
- A function that can fail returns `bool`, where `true` means **error** and
  `false` means success. Where a status code carries more information, return
  an integer where `0` means success and non-zero means error.
- Results are returned through pointer out-parameters (`T *out`), written
  only on success. Inputs that are not modified are passed as `const T*`.
- The header comment of every fallible function states when it fails, e.g.
  "Returns true (error) if the file cannot be opened."
- Preconditions and call constraints are documented in the same comment
  ("Requires a non-empty `*argv`.", "Call at most once.").

## F) Naming

| Kind                          | Style              | Example                       |
|-------------------------------|--------------------|-------------------------------|
| Types (class, struct, enum)   | `PascalCase`       | `ChildProcess`, `ReadStatus`  |
| Functions and methods         | `camelCase`        | `runCommand`, `utf8SeqLen`    |
| Local variables, parameters   | `snake_case`       | `left_width`, `on_ready`      |
| Class data members            | `snake_case_`      | `fd_`, `max_lines_`           |
| Plain struct fields           | `snake_case`       | `rows`, `read_eof`            |
| Constants (`constexpr`)       | `kPascalCase`      | `kMaxPayload`, `kWakePeriod`  |
| Enumerators                   | `kPascalCase`      | `ReadStatus::kEof`            |
| Global variables              | `g_snake_case`     | `g_quit`                      |
| Namespaces                    | `lowercase`        | `tact`, `luna`                |
| Macros, header guards         | `UPPER_SNAKE_CASE` | `TACT_SRC_TERM_H_`            |

- Enums **must** be `enum class`.

## G) Structure

### Files

- Headers use `.h`, sources use `.cc`.
- Every file starts with a one-line description: `// foo.cc - What it does.`
  Headers put `// vim: ft=cpp` and an empty `//` line before it.
- Header guards are `<PROJECT>_<PATH>_H_` (e.g. `TACT_SRC_TERM_H_` for
  `src/term.h`); `#pragma once` is not used.
- Public headers live in `include/<project>/` and are included as
  `"<project>/foo.h"`; private headers stay next to the sources and are
  included by their path from the project root (e.g. `"src/foo.h"`).
- Includes are grouped in this order, separated by a blank line: the file's
  own header (in a `.cc`), C/POSIX system headers (`<unistd.h>`), C++
  standard headers (`<string>`), then project headers.

### Namespaces

- All code lives in the project's namespace; only `main()` is outside it.
- File-local helpers and constants go in an anonymous namespace, not
  `static`.
- Namespaces close with a comment: `}  // namespace tact`.
- `using namespace` is not permitted.

### Classes

- A class that owns a resource (fd, process, terminal state) releases it in
  its destructor (RAII) and is non-copyable: its copy constructor and copy
  assignment are `= delete`.
- Public classes hide their implementation behind a pointer
  (`struct Impl; std::unique_ptr<Impl> impl_;`) so public headers carry no
  private dependencies.
- Within a class, `public` comes before `private`; data members come last.

### Layout and Comments

- Long files are split into sections with a banner:

  ```cpp
  // ---------------------------------------------------------------------------
  // Section
  // ---------------------------------------------------------------------------
  ```

- Every declaration in a header has a comment describing its behavior;
  parameter names are written in backticks (`` `fd` ``).
- Unused parameters are marked with `(void)name;`.
- Calls to C/POSIX functions use the global scope prefix (`::close(fd)`).
- Casts use `static_cast` / `reinterpret_cast` / `const_cast`; C-style and
  function-style casts are not permitted.
- Code is formatted with `clang-format` using the repository's
  `.clang-format` before commit.

## Enforcement Notes

- Rules (C) and (D) apply to all function signatures: free functions, class
  methods, static methods, lambdas, and function pointers/`std::function`
  signatures.
- Return types are not restricted by rules (C) or (D) unless stated
  otherwise — these rules govern **parameters** only.
- Deleted special members (`T(const T &) = delete;`,
  `T &operator=(const T &) = delete;`) are exempt from rule (C), since
  C++ offers no other way to declare a type non-copyable.
- Violations should be caught via code review and, where feasible, static
  analysis (e.g. a `clang-tidy` custom check or lint script).
