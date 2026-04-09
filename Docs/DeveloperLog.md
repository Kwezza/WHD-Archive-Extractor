# Developer Log

## 2026-04-09

Implemented safety and reliability fixes in WHDArchiveExtractor.c (items 1-6 from code review):

1. Fixed stack overflow risk by increasing `program_name` buffer size to safely hold `"c:unlzx"`.
2. Hardened `log_error` with a bounds guard (`error_count < MAX_ERRORS`) before writing.
3. Corrected argument validation from `argc < 2` to `argc < 3` to match required positional arguments.
4. Added null handling for `versionInfo` before `strcmp` and before `FreeVec`.
5. Added return-value check for `get_file_extension`; skip files when extension parsing fails.
6. Corrected optional argument parsing loop start index from `i = 4` to `i = 3`.
8. Replaced unbounded command formatting in `get_executable_version` with explicit length checks before command construction.

Notes:
- Changes were kept minimal and behavior-preserving except for defensive handling in invalid/error cases.
- No broad refactor was performed.

## Pending Issues

1. Recursion depth risk in directory scanning remains open.
	StackExtend reduces risk but does not guarantee safety for extremely deep or pathological directory trees.
	Suggested follow-up: add a maximum recursion depth guard (configurable) or move to an iterative directory traversal.
