# Developer Log

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

This update adds proactive destination drawer creation before archive extraction, and optional drawer icon application for newly created drawers.  This previously required a seperate program (available on Aminet) called IconSync to be run after WHDArchiveExtractor to create the icons.

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
