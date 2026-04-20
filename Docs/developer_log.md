# Developer Log

## 2026-04-20 - Added automatic fallback to extractor-managed folder creation when destination prep fails

This update hardens archive extraction against failures in the proactive destination drawer creation step.

### Behavior

- During normal extraction mode, destination drawer preparation still runs first.
- If manual destination prep fails for an archive, that archive is no longer skipped immediately.
- The program now prints a warning and falls back to extractor-managed folder creation for that archive.
- If extraction then fails, existing error handling remains unchanged (error is logged and scan continues).

### Visibility

- A new summary line reports how many archives used this fallback path:
  - `Destination drawer prep fallback used: <count>`
- In `-debug` mode, fallback events append debug log lines including archive and destination context.

### Scope

- Applies only to normal extraction flow.
- `-testarchivesonly` flow is unchanged.
- Existing success-path behavior for directory creation and optional icon application is unchanged.

## 2026-04-18 - Corrected disk-check option name in runtime error message

This update fixes inconsistent option naming in disk-space check failure guidance.

### What was fixed

1. Corrected the option name shown in the runtime error message from `-enablediskcheck` to `-enablespacecheck`.

### Why

- The actual CLI flag is `-enablespacecheck`.
- The old text could mislead users when trying to disable the free-space guard.

### Scope

- Message text only.
- No behavior changes to argument parsing or disk-space validation.

## 2026-04-18 - Skip LZX archive testing when UnLZX 2.16 is detected

This update applies a test-mode compatibility rule based on UnLZX version capabilities.

### Behavior

- In `-testarchivesonly` mode:
  - `LZX 1.21` continues to test LZX archives using the `t` command token.
  - `UnLZX 2.16` does not attempt LZX archive testing.

### User messaging

- When `UnLZX 2.16` is active in test mode, the run prints:
  - `UnLZX 2.16 does not support testing so LZX archives have been skipped.`
- End-of-run summary also reports the count of skipped LZX test archives.

### Scope

- This affects only LZX handling in `-testarchivesonly` mode.
- LHA testing and normal extraction behavior remain unchanged.

## 2026-04-18 - Fixed LZX test command token for UnLZX 1.21 and clarified test-mode status text

This update corrects LZX test-mode command invocation and removes misleading extraction wording in test-only runs.

### What was fixed

1. In `-testarchivesonly` mode, LZX archives now use the UnLZX test command token `t` instead of `-v`.
2. Test-mode per-archive console status now prints `Testing <archive>` instead of `Extracting <archive> to <path>`.

### Why

- On UnLZX 1.21, `-v` was interpreted as an unknown option in this code path.
- In test-only mode, extraction wording plus destination path output (for example `RAM:`) was confusing for users.

### Scope

- Change is limited to test-only behavior.
- Normal extraction commands and version-specific extraction options remain unchanged.

## 2026-04-18 - Added test_errors-[timestamp].txt logging for -testarchivesonly failures

This update adds a focused per-run error log for archive test failures when running in `-testarchivesonly` mode.

### Why this was added

Console output during large scans can make it difficult to quickly identify which archives failed and what return code was reported.

### What was added

1. A timestamped test error log is created for test-only runs:
  - `test_errors-YYYYMMDD-HHMMSS.txt`
2. For each archive command failure in test-only mode, one line is appended containing:
  - archive path
  - return code
3. Logging reuses the existing timestamp and append logging helpers used by other run logs.

### Runtime visibility

- Startup now reports whether the test error log is enabled and shows the filename.
- End-of-run summary now reports how many test errors were logged and points to the file when at least one test failure was recorded.

### Scope (current implementation)

- Active only in `-testarchivesonly` mode.
- Normal extraction runs are unchanged by this feature.

## 2026-04-18 - Documented `-testarchivesonly` argument-order quirk and temporary workaround

For this version, argument parsing remains positional for the first two parameters.

### Known quirk

When launched as:

- `WHDArchiveExtractor <source> -testarchivesonly`

the program currently interprets `-testarchivesonly` as the output directory (second positional argument), then fails target validation.

### Temporary workaround (current release)

Provide a valid destination path even in test-only mode, for example:

- `WHDArchiveExtractor <source> RAM: -testarchivesonly`

Using `RAM:` satisfies the current output-directory requirement while still running archive test/verify behavior.

### Notes

- This is a known CLI parsing limitation for the current version.
- A parser improvement to allow source-only test mode is deferred to a later update.

## 2026-04-18 - Added New-[timestamp].txt tracking for newly extracted archives in skip-if-destination mode

This update adds a focused run log so users can quickly identify archives that were newly extracted when running with `-skipifdestexists`.

### Problem addressed

When scanning large trees with `-skipifdestexists`, successful new extractions were easy to miss in console output due to the high volume of skip messages.

### What was added

1. A per-run timestamped file is created when `-skipifdestexists` is active (and not in `-testarchivesonly` mode):
  - `New-YYYYMMDD-HHMMSS.txt`
2. On each successful extraction, one line is appended to this file.
3. Logging reuses the existing append-and-flush utility already used by debug logging.

### Logged line format

Each entry contains:

- Archive filename
- Final destination drawer path used for extraction

Format:

- `archive=<archive-file> | destination=<resolved-destination-drawer-path>`

For LHA archives, the destination includes the resolved top-level drawer name discovered via the existing RAM listing flow, so the logged path reflects the actual extracted drawer target.

### Runtime visibility

When enabled, startup now reports the New log filename, and the summary reports how many archives were logged to it.

### Validation

Confirmed in user run output via generated file:

- `New-20260418-133723.txt`

## 2026-04-18 - Fixed protection reset command regression causing false 'corrupt archive' errors on rerun

This update fixes a long-running rerun extraction issue where some archives were reported as corrupt on second pass, even though extraction succeeded on first pass.

### Root cause

Two problems existed in the pre-extraction protection reset logic for LHA archives:

1. Destination drawer path was built with an accidental duplicate append of the archive root drawer name, causing folder existence checks to fail in some cases.
2. The protection command string omitted the `protect` executable name and attempted to run a wildcard path directly (`.../#? ALL rwed >NIL:`), which produced `#?: Unknown command` and skipped permission updates.

### What was fixed

1. Removed duplicate append of `directoryName` when building the protection target path.
2. Corrected command construction to valid AmigaDOS syntax:
   - `protect "<target>/#?" all rwed >NIL:`

### Validation evidence

Single-archive rerun test using `Jonathan_v1.1_De_0515.lha` now shows:

- Protection prep command executes with no command error.
- Debug log contains:
  - `protect-cmd=protect "work:test/output/JonathanDe/#?" all rwed >NIL:`
  - `protect-rc=0`
- Archive extraction succeeds on rerun:
  - `459 files extracted, all files OK.`
  - `Errors: 0`

### Temporary diagnostics

Temporary `-debug` instrumentation was added around protection prep to log:

- Target path
- Protect command string
- Protect return code
- Sample listing of matched files

This diagnostics block should be removed after final confirmation across a broader archive sample.

## 2026-04-17 - Task 2 implemented: extracted counter and quiet skip logging

This update improves rerun visibility when using destination-exists skipping, and reduces output noise during large scans.

### Command-line behavior

- New optional switch: `-quietskips`
- `-quietskips` suppresses per-archive skip lines emitted by `-skipifdestexists`.
- Quiet mode keeps conflict and error output visible.
- Quiet mode prints periodic scan heartbeat lines to show progress on slower machines.

### Summary changes

- Added summary line:
  - `Archives extracted: <count>`
- Extracted count increments only when an archive extraction command succeeds in normal mode.
- `-testarchivesonly` runs do not increment extracted count.
- When quiet mode is active, an extra summary note is printed to indicate reduced skip verbosity.

### Notes for maintainers

- Heartbeat cadence is currently fixed in source as `QUIET_HEARTBEAT_INTERVAL`.
- Existing skip counter behavior is unchanged:
  - `Skipped because destination existed: <count>` remains in summary.

## 2026-04-17 - Destination path preparation and drawer icon integration

This update adds proactive destination drawer creation before archive extraction, and optional drawer icon application for newly created drawers.  This previously required a seperate program (available on Aminet) called IconSync to be run after WHDArchiveExtractor to create the icons.  This feature was requested by a user, but at the time, the quickest fix was to pull out the directory traversal code out of WHDAchiveExtractor and place it into a new program, rather the reprogram WHDAchiveExtractor to no longer depend on the extract tool to create the directories.

### What changed

- The extractor now walks the destination path from the output root down to the final archive destination drawer.
- Each missing path component is created before extraction starts.
- This removes reliance on extraction tools to create parent drawers implicitly.

### Directory preparation behavior

For each archive destination path:

1. Validate the output root exists and is a drawer.
2. Derive a relative destination path from the output root.
3. Walk each component in order (left to right).
4. Create missing drawers as needed.
5. Continue extraction only after path preparation succeeds.

If directory preparation fails for one archive destination, that archive is skipped and processing continues for the remaining archives.

### Custom drawer icon behavior

Custom drawer icon application is optional.

- Disabled by default.
- Enabled using the `-enablecustomicons` command-line switch.
- Icon source folder is fixed to `PROGDIR:Icons`.

When enabled, for each newly created drawer:

1. If `<new_drawer_path>.info` already exists, it is preserved.
2. The program attempts to load a matching icon from `PROGDIR:Icons/<leaf_name>.info`.
3. If matching custom icon is not found, it falls back to the default drawer icon (`WBDRAWER`).
4. Icon failures are treated as non-fatal and extraction continues.

### Important requirement for icon source content

The icon source folder must contain matching drawer names for icon names.

Example:

- If the icon file is `B.info`, there must also be a drawer named `B` in the icons folder.

If this pairing is missing, output is undefined. In observed runs, this has produced destination drawers that Workbench cannot see.

### With and without custom icons

Without `-enablecustomicons`:

- Drawer creation still happens.
- No custom icon matching is attempted.
- Extraction behavior remains deterministic and unaffected by icon source content.

With `-enablecustomicons`:

- Drawer creation still happens.
- Matching icon copy is attempted for each newly created drawer.
- Fallback to default drawer icon applies when no matching custom icon is available.
- Incorrect icon source layout (missing matching drawer names) can lead to undefined Workbench results.

### Notes for maintainers

- All icon integration is in the main source file.
- The game-folder template API (`icon_applier_apply_game_folder_icon`) is intentionally not integrated.
- The current integration scope is drawer icon ensure/copy only for newly created path components.

## 2026-04-17 - Task 1 implemented: skip extraction when destination already exists

This update implements Task 1 from the enhancement plan: optional skip behavior when the intended extraction destination already exists.

### Command-line behavior

- New optional switch: `-skipifdestexists`
- Default behavior remains unchanged when this switch is not provided.
- Skip logic is only applied during real extraction mode.
- In `-testarchivesonly` mode, archives are still tested and are not skipped by destination existence.

### Task 1 flow implemented

For each supported archive:

1. Resolve the destination path as normal.
2. Resolve the intended destination drawer path for the archive.
3. Check whether that destination path already exists as a drawer.
4. If it exists and `-skipifdestexists` is enabled, skip extraction.
5. If it does not exist, continue with normal extraction behavior.
6. Record skips in end-of-run summary output.

### Output and summary changes

- Added per-archive skip message for existing destinations.
- Skip message text was shortened for CLI readability:
  - `Skipping <archive> (already exists: /relative/path)`
- Skip path is shown relative to the configured extraction output root.
- Added summary counter line:
  - `Skipped because destination existed: <count>`

### Important limits (Task 1 scope)

- Existence is treated as a practical shortcut, not proof of successful extraction.
- The feature does not compare timestamps or archive contents.
- The feature does not use persistent marker files or historical state.
- The feature does not track prior runs.

### Path handling notes

- Amiga path separator remains `/`.
- Root path normalization still prevents invalid `:/` forms at volume roots.
- Relative skip output is derived from normalized destination paths.

## 2026-04-17 - Post-merge verification of safety and reliability fixes

After resolving a branch/versioning merge issue, `WHDArchiveExtractor.c` was reviewed to verify that previously shipped safety fixes are still present in `main`.

### Verified fixes still applied

1. `program_name` buffer remains sized to safely hold `"c:unlzx"`.
2. `log_error` still guards writes with `error_count < MAX_ERRORS` semantics (returns early when max is reached).
3. Argument validation remains `argc < 3` to match required positional arguments.
4. `versionInfo` null handling remains in place before `strcmp` checks and before `FreeVec`.
5. `get_file_extension` return value is still checked and files are skipped when parsing fails.
6. Optional argument parsing loop still starts at `i = 3`.
7. `get_executable_version` still uses explicit length checks before command construction.

### Notes

- This entry records verification status after merge reconciliation.
- No behavioral changes were introduced as part of this documentation update.
