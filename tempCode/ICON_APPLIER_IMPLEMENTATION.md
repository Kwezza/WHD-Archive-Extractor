# Icon Applier Integration Guide for AI Agent

## Objective
Implement the standalone icon creation/apply behavior from this repository into another program so that icons are handled as new folders are created.

This guide assumes AmigaOS/VBCC-style APIs are available (DiskObject, PutDiskObject, GetDefDiskObject, etc.).

## Scope
Include:
- Ensure drawer icons exist when creating folders.
- Apply the WHD folder template icon to newly created game folders.
- Use custom icons when available, fallback to default drawer icon when missing.

Exclude:
- Any logging framework integration.
- Any icon unsnapshot/position stripping logic.
- Any downloader/extractor/archive-index integration.

## Source of Truth in This Repo
Use these files in temp as the portable baseline:
- temp/icon_applier.h
- temp/icon_applier.c
- temp/icon_applier_main.c (example CLI driver only)

Do not pull behavior from the original extraction pipeline unless needed for host integration details.

## Public API to Integrate
### Data structure
- icon_applier_options
  - use_custom_icons: BOOL
  - icons_dir: const char * (NULL means use default PROGDIR:Icons)

### Functions
- icon_applier_ensure_drawer_icon(const char *dir_path, const icon_applier_options *options)
  - Purpose: ensure dir_path.info exists.
  - Behavior:
    1. If dir_path.info already exists, return TRUE.
    2. If custom icons enabled and icons_dir/<leaf>.info exists, use it.
    3. Otherwise use GetDefDiskObject(WBDRAWER).
    4. Write with PutDiskObject(dir_path, diskobj).

- icon_applier_apply_game_folder_icon(const char *target_directory, const char *game_folder_name, const icon_applier_options *options)
  - Purpose: apply icons_dir/WHD folder.info to target_directory/game_folder_name.info.
  - Behavior:
    1. If custom icons disabled, return TRUE.
    2. Check template existence once and cache it.
    3. If template missing, return TRUE (non-fatal).
    4. Build destination path.
    5. If destination .info exists, SetProtection(path, 0), then DeleteFile(path).
    6. Load template via GetDiskObject and write via PutDiskObject.

## Required Host Capabilities
Your target program must provide or include:
- BOOL, BPTR, DiskObject types.
- Lock/UnLock, GetDiskObject, GetDefDiskObject, PutDiskObject, FreeDiskObject.
- SetProtection, DeleteFile.
- Standard C functions: snprintf, strrchr, strchr, strcpy, strlen.

If host has different platform APIs, create a thin adapter layer with equivalent semantics.

## Integration Pattern
Call icon handling at folder creation points in this order:

1. Create or ensure base folder.
2. Create or ensure category folder.
3. Create or ensure letter folder.
4. For each of 1-3, call icon_applier_ensure_drawer_icon(path, options).
5. After creating each final game folder, call:
   - icon_applier_apply_game_folder_icon(parent_dir, game_folder_name, options)

Recommended default options:
- use_custom_icons = TRUE
- icons_dir = NULL (uses PROGDIR:Icons)

## Path Rules
- Keep Amiga-style path sanitation before filesystem calls.
- Preserve this behavior from temp/icon_applier.c:
  - Collapse duplicate slashes.
  - Remove illegal characters (* ? # | < > " { }).
  - Keep volume prefix handling with ':'.
  - Trim trailing slash where needed.

## Error Handling Contract
For host integration:
- Return FALSE only for invalid inputs or actual write/OS failures.
- Treat missing custom icon/template as non-fatal and continue with fallback/default behavior.
- Do not abort entire folder-creation workflow just because icon application failed.

## Porting Checklist
1. Copy temp/icon_applier.h and temp/icon_applier.c into target project.
2. Wire include paths for Amiga headers or host adapter types.
3. Remove/replace any include incompatibilities in target build system.
4. Hook icon_applier_ensure_drawer_icon into directory creation flow.
5. Hook icon_applier_apply_game_folder_icon right after final folder creation.
6. Ensure template file exists at PROGDIR:Icons/WHD folder.info in runtime environment.
7. Validate fallback path by temporarily removing custom icons.

## Acceptance Criteria
- New parent/category/letter folders receive an icon if missing.
- Final game folder receives WHD folder template icon when available.
- If custom icon is missing, default drawer icon is used where appropriate.
- Existing .info files are preserved for drawer creation and replaced only for final game-folder template application.
- No logging dependency is required.
- No unsnapshot/coordinate stripping code is present.

## Suggested Minimal Test Matrix
1. Custom icons directory present with matching leaf icon and WHD folder template.
2. Custom icons directory present without matching leaf icon.
3. No custom icons directory at all.
4. Existing destination .info is write-protected.
5. Custom icons disabled with use_custom_icons = FALSE.

## Notes for AI Agent
- Keep implementation behavior deterministic and side-effect minimal.
- Do not add unrelated refactors.
- Do not introduce GUI dependencies.
- Preserve C style and conservative memory usage suitable for classic Amiga environments.
