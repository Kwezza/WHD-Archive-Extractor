# Project Guidelines

## Code Style
- Keep changes C89-compatible for SAS/C (avoid C99/C11 features such as mixed declarations, // comments, and stdbool).
- Prefer existing AmigaOS idioms already used in the project: STRPTR, BPTR, DOS/Exec APIs, AllocVec/FreeVec.
- Preserve the current defensive string style: use fixed-size buffers with explicit length checks before strcat/sprintf-style writes.
- When adding helper functions in `WHDArchiveExtractor.c`, declare prototypes near the existing prototype block.
- Keep function-level comments concise and useful; follow the current style in `WHDArchiveExtractor.c`.

## Architecture
- The tool is intentionally a single-file CLI in `WHDArchiveExtractor.c`.
- Core flow:
  1. Parse CLI args and optional flags.
  2. Recursively scan source directories (`get_directory_contents`).
  3. Detect `.LHA`/`.LZX` archives and map output paths.
  4. Execute extraction commands via Amiga DOS APIs.
  5. Aggregate/log errors and stop safely when limits are reached.
- Shared state (error counters, flags, archive stats) is global by design; keep updates to this state explicit and easy to trace.

## Build And Test
- Primary build uses SAS/C and `smakefile`:
  - Build: `smake`
  - Clean: `smake clean`
- If invoking compiler manually, keep `StackExtend` enabled (required for deep recursive scans):
  - `sc noicon StackExtend WHDArchiveExtractor.c`
- Runtime usage:
  - `WHDArchiveExtractor <source_directory> <output_directory> [-enablespacecheck] [-testarchivesonly]`
- External runtime tools expected in `C:`:
  - Required: `lha`
  - Optional for LZX archives: `unlzx` / LZX extractor variant
- There is no automated unit test suite. Validate behavior with representative archives (including nested folders, LHA, and LZX) and use `-testarchivesonly` for safe archive verification runs.

## Project Conventions
- Treat path handling as high-risk: preserve Amiga path semantics (`volume:` prefixes, slash normalization) and route updates through existing path helpers.
- Any new buffer or command-string logic must include explicit overflow checks before concatenation.
- Every `AllocVec` allocation should have a matching `FreeVec` on all success and error paths.
- Avoid broad refactors unless requested; small, behavior-preserving changes are preferred.

## Existing Documentation
- User-facing usage and prerequisites: `README.md`
- Release notes and historical behavior: `WHDArchiveExtractor.readme`
- Build commands and flags: `smakefile`
