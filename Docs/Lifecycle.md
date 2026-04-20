# WHDArchiveExtractor Program Lifecycle

This document reflects current behavior implemented in WHDArchiveExtractor.c.

## 1. Startup and Tool Detection

On startup, the program does the following:

1. Parses early output-related options so startup verbosity can be adjusted.
2. Prints the startup banner unless -output=script is active.
3. Checks for required c:lha.
4. Checks for optional c:unlzx.

Tool behavior:

- c:lha missing: fatal. Program prints a message and exits.
- c:unlzx missing: non-fatal. Program continues; LZX files are detected but cannot be expanded.
- c:unlzx present: version is detected (version c:unlzx), then LZX command tokens are selected:

| Detected version | LZX command token | Target token |
|---|---|---|
| UnLZX 2.16 | -x | -o |
| LZX 1.21 | -q -x e | (none) |
| Unknown |  e | (none) |

In -testarchivesonly mode with UnLZX 2.16, LZX test commands are not attempted (unsupported by that tool version).

## 2. CLI Parsing and Mode Resolution

Usage supports two launch forms:

- WHDArchiveExtractor <source_directory> <output_directory> [options]
- WHDArchiveExtractor <source_directory> -testarchivesonly [options]

Supported options:

- -enablespacecheck
- -testarchivesonly
- -skipifdestexists
- -quietskips
- -enablecustomicons
- -debug
- -output=script|normal|verbose
- -writesummary

Important parsing behavior:

1. The second argument can be treated as an option (not destination) when it begins with -.
2. -testarchivesonly is detected early across arguments.
3. If running source-only test mode, destination is internally set to source path, and target-folder existence checks are skipped.
4. Trailing / is removed from source and destination paths.

Option effects:

- -enablespacecheck: enables 20MB free-space check (startup and per-archive in extraction mode).
- -testarchivesonly: validates archives only, no extraction.
- -skipifdestexists: in extraction mode, skips archive when resolved destination drawer already exists.
- -quietskips: suppresses per-archive skip messages and prints heartbeat progress.
- -enablecustomicons: applies drawer icons to newly created destination drawers.
- -debug: writes timestamped debug log file (error-YYYYMMDD-HHMMSS.txt).
- -output=script|normal|verbose:
  - script: minimal console output and extractor output redirected to NIL:.
  - normal: default output.
  - verbose: normal output plus less-suppressed LHA extraction flags.
- -writesummary: writes fixed summary file PROGDIR:WHDArchiveExtractor_last_run.txt.

## 3. Preflight Validation and Run Logging Setup

Before scanning begins:

1. Source folder must exist.
2. Destination folder must exist only when not in -testarchivesonly mode.
3. If -enablespacecheck is active and not in test mode, free-space preflight must pass.

Run-scoped log setup:

- -debug: enables error-YYYYMMDD-HHMMSS.txt.
- -skipifdestexists in extraction mode: enables New-YYYYMMDD-HHMMSS.txt.
- -testarchivesonly: enables test_errors-YYYYMMDD-HHMMSS.txt.

Startup prints active mode/log states unless -output=script is active.

## 4. Recursive Scan Engine

The program recursively scans from source root.

Per entry:

1. Build full path safely; on path overflow, log error and continue.
2. If drawer: recurse.
3. If file: process only .LHA or .LZX (case-insensitive via uppercase conversion).

A progress heartbeat is printed every QUIET_HEARTBEAT_INTERVAL entries when -quietskips is active and output mode is not script.

## 5. Destination Resolution, Conflict Checks, and Skip Rules

For each archive:

1. Resolve relative source subpath and base destination path.
2. Build default destination drawer as <archive_name_without_extension>.
3. For .LHA, inspect archive listing (c:lha vq ... >ram:listing.txt) and, if found, prefer first top-level drawer name as destination drawer.

Conflict checks:

- If destination exists but is not a drawer: count conflict and skip archive.
- If two archives in same scanned folder claim same destination key: count conflict and skip archive.
- Conflict samples are tracked for summary (with overflow count once sample cap is reached).

Skip rule:

- If -skipifdestexists and not test mode and destination drawer already exists, archive is skipped and counted.
- Skip line visibility depends on -quietskips and output mode.

## 6. Archive Mode Commands

### LHA

- Test mode: lha t.
- Normal mode: lha -T -M -N -m x.
- Verbose mode: lha -T x.

### LZX

Command token depends on detected c:unlzx version:

- UnLZX 2.16: extraction uses -x with target token -o.
- LZX 1.21: extraction uses -q -x e.
- Unknown version: extraction uses  e fallback.

LZX in test mode:

- UnLZX 2.16: test is skipped and counted (num_lzx_archives_skipped_test_unsupported).
- Other versions: test uses t.

## 7. Destination Prep, Icons, and Fallback

In extraction mode only (not test mode):

1. Optionally perform per-archive disk-space check when -enablespacecheck is active.
2. Attempt proactive destination drawer creation from output root to resolved destination path.
3. If path prep succeeds: count created drawers.
4. If path prep fails: warn and continue with extractor-managed folder creation fallback.

Custom icon handling (-enablecustomicons):

- Applied only when a new drawer is created during prep.
- Existing .info files are preserved.
- Attempts matching icon from PROGDIR:Icons/<leaf_name>.info.
- Falls back to default WBDRAWER icon if custom icon not found.
- Icon failures are non-fatal and counted.

Fallback usage is counted and optionally logged in debug log.

## 8. LHA Protection Reset (Overwrite Prep)

For LHA extraction mode, the program performs a pre-extraction overwrite prep:

1. Reads archive listing to resolve top-level destination drawer.
2. If that destination drawer already exists, executes:

   protect "<target>/#?" all rwed >NIL:

This attempts to clear restrictive protection bits before extraction overwrite.

## 9. Command Execution and Error Handling

Extraction/test command is assembled with source archive and resolved destination path, sanitized, and executed via SystemTagList().

-output=script behavior:

- Extractor command output is redirected to NIL:.
- In extraction mode, one progress line is printed per archive: Extracting: <archive>.
- Warnings/errors still appear.

Return-code handling:

- 0: success.
- 10: logged as corrupt archive.
- Other non-zero: logged as generic extraction failure.

Additional logging:

- In -testarchivesonly, non-zero returns append to test_errors-...txt.
- In extraction mode with -skipifdestexists, successful new extractions append to New-...txt.
- In debug mode, selected failures/fallback events append context lines.

Hard stop:

- When error count reaches MAX_ERRORS (40), scan aborts.

## 10. End-of-Run Summary and Optional Fixed Summary File

After scan completion:

1. Elapsed time is computed.
2. If -writesummary, a fixed summary file is written to:
   - PROGDIR:WHDArchiveExtractor_last_run.txt
3. Console summary is printed.

Console summary includes:

- Entries/archives totals and LHA/LZX split.
- Extracted, skipped, conflicts.
- Conflict subtype and sample count details when present.
- Destination drawers created.
- Destination prep fallback usage count.
- Icon attempt/apply/failure counts when icon mode active.
- LZX-unavailable note when LZX found and c:unlzx missing.
- Test-mode LZX-skip count for UnLZX 2.16.
- Quiet-skip note when enabled.
- New archive log count/path when enabled.
- Test error log count/path when applicable.
- Runtime H:MM:SS and error count.
- Detailed error list when errors exist.

Clean test footer:

- In -testarchivesonly, when no errors and no test failures were logged, prints:
  - Archive test result: no damaged archives found.

Final line:

- Prints closing version banner.

## 11. Current Behavioral Notes

- Destination conflict detection is folder-local and sample-capped.
- Source-only test mode is fully supported without destination argument.
- New-...txt is run-scoped and records successful extractions only.
- test_errors-...txt is run-scoped and test-mode only.
- WHDArchiveExtractor_last_run.txt is overwritten each run when enabled.
