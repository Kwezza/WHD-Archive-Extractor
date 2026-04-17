# WHDArchiveExtractor — Program Lifecycle

## 1. Startup and Tool Checks

Before argument parsing, the program performs mandatory tool checks:

- **`c:lha` (required):** Checked with `does_file_exist("c:lha")`. If missing, the program prints an error and exits immediately. No extraction is possible without it.
- **`c:unlzx` (optional):** Also checked with `does_file_exist`. If missing, the program prints a warning and continues — LZX archives will be skipped. If present, the program runs `version c:unlzx` and inspects the output to set the correct extraction flags:

  | Detected version | Extract command | Target flag |
  |-----------------|----------------|-------------|
  | `UnLZX 2.16`    | `-x`           | `-o`        |
  | `LZX 1.21`      | `-q -x e`      | *(none)*    |
  | Unknown / none  | ` e`           | *(none)*    |

---

## 2. Argument Parsing

Minimum required: `WHDArchiveExtractor <source_directory> <output_directory>`

If fewer than 3 arguments are supplied the program prints usage text and exits with code `1`.

```
WHDArchiveExtractor <source_directory> <output_directory> [-enablespacecheck] [-testarchivesonly]
```

| Argument              | Effect |
|-----------------------|--------|
| `argv[1]`             | Source directory path (`input_directory_path`) |
| `argv[2]`             | Destination directory path (`output_directory_path`) |
| `-enablespacecheck`   | Enables the 20 MB free-space check before each extraction. Disabled by default. |
| `-testarchivesonly`   | Passes a test/verify flag to the extractor instead of extracting files. |

After parsing, trailing slashes are stripped from both paths with `remove_trailing_slash()`.

---

## 3. Source and Target Existence Checks

Both directories are verified with `does_folder_exists()`, which attempts an Amiga `Lock()` on the path:

1. **Source directory** — if the lock fails the program prints `Unable to find the source folder <path>` and exits with code `0`.
2. **Target directory** — same check; if the lock fails the program prints `Unable to find the target folder <path>` and exits with code `0`.

If `-enablespacecheck` was passed, a third check runs at this point: `check_disk_space()` queries the target volume via `Info()` and requires at least 20 MB free. If the check fails the program prints an error and exits before any scanning begins.

---

## 4. Recursive Directory Scan (`get_directory_contents`)

With both directories confirmed, a timer starts (`time(NULL)`) and `get_directory_contents()` is called recursively.

For every entry found:

- **Sub-directory:** `get_directory_contents()` is called again on that path (depth-first).
- **File with `.LHA` or `.LZX` extension:** triggers the extraction flow below.
- **Any other file:** silently skipped.

---

## 5. Per-Archive Extraction Flow

### 5a. Protection-bit reset (LHA only, normal mode)

Before building the extraction command, the program runs:

```
c:lha vq "<archive>" >ram:listing.txt
```

It reads `ram:listing.txt` to find the first directory name inside the archive. If that directory already exists in the target path, the program runs an Amiga `Protect` command to clear all protection bits recursively:

```
Protect <target_path>/<dir>/#? ALL rwed >NIL:
```

This ensures existing protected files can be overwritten by the extraction. The temporary file `ram:listing.txt` is deleted afterwards.

### 5b. Disk space check (if `-enablespacecheck` is active)

`check_disk_space()` is called on the target path before each archive. If less than 20 MB is free, `should_stop_app` is set to `1` and the loop exits, stopping all further extraction.

### 5c. Command execution

The extraction command is assembled from:

- The extractor binary (`lha` or `c:unlzx`)
- The extract flags (or test flags when `-testarchivesonly` is active)
- The quoted source archive path
- The quoted target path (output directory + relative sub-path derived from the source tree)

The path is sanitised by `sanitize_amiga_path()` to handle Amiga volume-colon and double-slash edge cases, then executed with `SystemTagList()`.

---

## 6. Extraction Error Handling

The return code from `SystemTagList()` is inspected after every archive:

| Return code | Meaning logged |
|-------------|----------------|
| `0`         | Success — no action |
| `10`        | Corrupt archive — logged as `"<path> is corrupt"` |
| Any other   | General failure — logged as `"<path> failed to extract. Unknown error"` |

Errors are stored in `error_messages_array[MAX_ERRORS][MAX_ERROR_LENGTH]` via `log_error()`. Each entry is capped at 255 characters.

**Hard stop:** if `error_count` reaches `MAX_ERRORS` (40), the program prints `Maximum number of errors reached. Aborting.`, sets `should_stop_app = 1`, and exits the scan loop immediately.

Buffer overflow situations (path too long to fit in a fixed 256-byte buffer) are also logged as errors and skip the affected archive rather than truncating the path.

---

## 7. End-of-Run Summary

After the scan loop completes, the program prints:

1. **Directory and archive counts:**
   ```
   Scanned N directories and found N archives.
   Archives composed of N LHA and N LZX archives.
   ```
2. **UnLZX warning** (if LZX archives were found but `c:unlzx` is not installed):
   ```
   UnLZX is not installed. N LZX archives were found but not expanded.
   ```
3. **Elapsed time** in `H:MM:SS` format.
4. **Error list** (via `print_errors()`):
   - If errors were logged, each is printed as `Error N: <message>`.
   - If no errors occurred, prints `No errors encountered.`
5. **Version banner** — the program name and version string are printed again as a closing line.
