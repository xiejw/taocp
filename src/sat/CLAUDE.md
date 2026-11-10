# CLAUDE.md — src/sat

## `src/eval.h` is a self-contained, header-only module

- `eval.h` must stay **header-only**: there is no `eval.cc`, and none should be
  added. All functions are defined in the header (`inline`, or in-class).
- `eval.h` must not include any project header (no `log.h`,
  `test_macros.h`, etc.) — only C/C++ standard headers. Copying the single
  file into another module must be enough to use it.
- Internal helpers go in `namespace taocp::sat::eval::detail` (not an anonymous
  namespace, which would be duplicated per translation unit in a header).
- Fatal errors use the header's own `TAOCP_EVAL_PANIC` (prints to stderr, then
  `abort()`), not the `PANIC` macro from `log.h`.
- The `Makefile` lists no `MODS` for `eval`; if you add a new module here,
  do not reintroduce an `eval.o` dependency.
