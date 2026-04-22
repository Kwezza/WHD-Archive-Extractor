# WHD Archive Extractor

A command line tool for the Amiga that scans a folder tree for WHDLoad archives and extracts them using Amiga-native tools.

It was written mainly for handling larger WHDLoad archive collections on the Amiga itself. Extracting these archives on other platforms can sometimes cause problems with Amiga file attributes or filenames, so this tool keeps the whole process on the Amiga using the usual archive tools.

The program can scan through subdirectories, preserve the original folder layout, and process large numbers of LHA and LZX archives in one run.

Check out my website that details my toold for Workbench for other relates WHDLoad programs:
https://www.madeforworkbench.com

---

## Features

- Scans input folders and subfolders for LHA and LZX archives
- Extracts archives using Amiga archive tools already installed in `C:`
- Preserves the subfolder structure from the source path
- ✨ Can test archives without extracting them
- ✨ Can skip archives whose destination drawer already exists
- ✨ Can reduce console output for use in scripts or batch runs
- ✨ Can write a simple summary file for the last run
- ✨ Optional built in support for applying custom drawer icons to newly created destination folders

✨ New in version 2.0

---

## Requirements

- **LHA** installed in `C:` — required for the program to run
  - Download from Aminet: https://aminet.net/package/util/arc/lha
- **UnLZX or LZX** installed in `C:` — required only for LZX archives
  - Download from Aminet: https://aminet.net/package/util/arc/lzx121r1
- AmigaOS 2.0.4 or later

---

## Usage

```
WHDArchiveExtractor <source_directory> <output_directory_path> [options]
WHDArchiveExtractor <source_directory> -testarchivesonly [options]
```

### Options

**`-enablespacecheck`**  
Checks free disk space before the run and before each archive is extracted. The minimum threshold is 20 MB. Off by default as the check adds overhead. Only applies in extraction mode.

**`-testarchivesonly`**  
Tests all archives in the source tree without extracting them — no destination folder is required. Uses each archive tool's built-in test command to verify integrity. Any test failures are logged to a timestamped file (`test_errors-YYYYMMDD-HHMMSS.txt`). Note: UnLZX 2.16 does not support archive testing, so LZX archives are skipped when that version is detected.

**`-skipifdestexists`**  
Skips any archive whose resolved destination drawer already exists. Useful for repeat runs over the same source to avoid re-extracting previously processed archives. The check is based on whether the destination drawer exists, not on comparing archive contents. Successfully extracted archives are logged to a timestamped file (`New-YYYYMMDD-HHMMSS.txt`).

**`-quietskips`**  
Suppresses the per-archive skip messages produced by `-skipifdestexists`. Instead of printing a line for every skipped archive, the program prints a progress heartbeat every 250 entries so you can see it is still working. Useful when scanning large collections where most archives are already extracted.

**`-enablecustomicons`**  
Applies drawer icons to newly created destination drawers during extraction. The program looks for a matching icon in `PROGDIR:Icons/<drawer_name>.info`. If no matching icon is found, a default Workbench drawer icon is used instead. Existing `.info` files are never overwritten. Icon failures are non-fatal and counted in the summary.

**`-debug`**  
Writes a timestamped debug log file to the program directory (`error-YYYYMMDD-HHMMSS.txt`). Captures additional detail for failures, fallback events, and diagnostics not shown on the console.

**`-output=script|normal|verbose`**  
Controls the amount of console output:
- `script` — Minimal output. Suppresses most status messages and redirects archive tool output to `NIL:`. Prints one progress line per archive. Warnings and errors are still shown.
- `normal` — Default output level with standard status messages.
- `verbose` — Normal output plus less-suppressed LHA extraction flags.

**`-writesummary`**  
Writes a machine-readable summary of the run to `PROGDIR:WHDArchiveExtractor_last_run.txt`. This fixed-name file is overwritten on each run and contains key=value pairs covering source and destination paths, archive counts, extraction results, elapsed time, and overall result status. Intended for use by scripts or other tools that need to inspect the outcome of a run.

### Examples

```
WHDArchiveExtractor PC0:WHDLoad/Beta DH0:WHDLoad/Beta -skipifdestexists -quietskips -output=normal -writesummary
```

Scans `PC0:WHDLoad/Beta` and extracts all archives found to `DH0:WHDLoad/Beta`, preserving the folder structure, skipping destinations that already exist, and saving a summary.

```
WHDArchiveExtractor PC0:WHDLoad/Beta -testarchivesonly -output=script -writesummary
```

Tests all archives in `PC0:WHDLoad/Beta` without extracting anything.

---

## Notes

- In normal extraction mode the program preserves the source folder structure beneath the output path.
- In test-only mode, archives are checked but not unpacked, and no output directory is required.
- The `-skipifdestexists` option checks whether the destination drawer already exists — it does not compare archive contents.
- Extraction speed will vary depending on the Amiga and storage being used. Extracting from a real hard drive to the same hard drive can be slow.

---

## Building from Source

The project is written in C and is built on the Amiga using **SAS/C** and the **Amiga SDK libraries**.

To build, run `smake` from the project directory on the Amiga:

```
smake
```

To clean build outputs:

```
smake clean
```

---

## Changelog

### [2.0.0] - 2026-04-20
- Added source-only `-testarchivesonly` mode
- Added `-skipifdestexists` for faster reruns
- Added `-quietskips` to reduce skip output noise
- Added `-output=script|normal|verbose`
- Added `-writesummary` fixed summary report output
- Added `New-[timestamp].txt` log for newly extracted archives
- Added `test_errors-[timestamp].txt` log for failed archive tests
- Added proactive destination drawer creation
- Added optional custom drawer icon support with `-enablecustomicons`
- Fixed rerun overwrite problems caused by protection reset command issues
- Fixed false corrupt-archive reports on some reruns
- Fixed LZX test command handling for LZX 1.21
- Fixed test-mode wording so test runs no longer look like extraction runs
- Fixed handling of unsupported LZX testing under UnLZX 2.16
- Fixed incorrect `-enablespacecheck` option text in runtime output

### [1.2.1] - 2025-05-10
- Fixed buffer overflow issues and a memory leak
- Added DOS version cookie

### [1.2.0] - 2025-05-06
- Added proper support for both UnLZX 2.16 and LZX 1.21
- Fixed misleading LZX support wording from earlier versions (thanks to Ed Brindley for reporting this)

### [1.1.1] - 2024-03-15
- Fixed typos and text alignment issues

### [1.1.0] - 2024-03-12
- Added LZX archive support
- Improved error handling
- Existing target folders are now scanned for protected files and protection bits are cleared so files can be replaced if needed

### [1.0.0] - 2023-04-28
- First release

---

## Disclaimer

Before using this program, please make sure to back up any important data or files. The creators and contributors of this software are not responsible for any data loss or damage that may occur as a result of using this program.

---

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (c) 2024 Kerry Thompson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
