WHD Archive Extractor 2.0
=========================

Author:   Kerry Thompson (kwezza@gmail.com)
License:  MIT License
Source:   https://github.com/Kwezza/WHD-Archive-Extractor
Website:  https://www.madeforworkbench.com


OVERVIEW
--------

WHD Archive Extractor is a command line tool for the Amiga that scans a
folder tree for WHDLoad archives (.LHA and .LZX) and extracts them using
Amiga-native archive tools.

It was written mainly to help process larger WHDLoad archive collections
directly on the Amiga.  Extracting these archives on other platforms can
sometimes cause problems with Amiga file attributes or file names, so
this tool keeps the whole process on the Amiga using the usual archive
tools.

The program can scan through subdirectories, preserve the original folder
layout, and process large numbers of LHA and LZX archives in one run.


REQUIREMENTS
------------

- AmigaOS 2.0.4 or later
- LHA installed in C: (required - the program will not run without it)
- UnLZX or LZX installed in C: (optional - needed only for LZX archives)

The program detects which LZX tool version is present and adjusts its
command syntax accordingly.  Both UnLZX 2.16 and LZX 1.21 are supported.


USAGE
-----

There are two ways to run the program:

  WHDArchiveExtractor <source> <destination> [options]
  WHDArchiveExtractor <source> -testarchivesonly [options]

The first form extracts archives found under the source folder into the
destination folder, preserving subfolder structure.  The second form tests
archives without extracting them.

Options can appear in any order after the positional arguments.

Example - extract all archives from a WHDLoad collection:

  WHDArchiveExtractor DH1:WHDLoad/Games DH2:Games

Example - test archives without extracting:

  WHDArchiveExtractor DH1:WHDLoad/Games -testarchivesonly

Example - extract, skipping archives already extracted, with summary:

  WHDArchiveExtractor DH1:WHDLoad/Games DH2:Games -skipifdestexists -writesummary


OPTIONS
-------

-enablespacecheck

    Enables a free disk space check before the run starts and before each
    archive is extracted.  The minimum threshold is 20 MB.  This option is
    off by default because the check adds overhead.  It only applies in
    extraction mode.

-testarchivesonly

    Tests all archives found in the source tree without extracting them.
    No destination folder is required.  The program uses each archive
    tool's built-in test command to verify integrity.

    Note: UnLZX 2.16 does not support archive testing, so LZX archives
    are skipped when that version is detected.  LZX 1.21 supports testing
    normally.

    When all tests pass, the summary prints:
      Archive test result: no damaged archives found.

    Any test failures are logged to a timestamped file:
      test_errors-YYYYMMDD-HHMMSS.txt

-skipifdestexists
    In extraction mode, skips any archive whose resolved destination
    drawer already exists.  This is useful for repeat runs over the same
    source folder to avoid re-extracting archives that were already
    processed.

    The check is based on whether the destination drawer exists, not on
    comparing archive contents.

    Successfully extracted archives are logged to a timestamped file:
      New-YYYYMMDD-HHMMSS.txt

    Each line in that file records the archive and its destination path.
    The log is run-scoped and only records successful extractions.

-quietskips

    Suppresses per-archive skip messages when used with -skipifdestexists.
    Instead of printing a line for every skipped archive, the program
    prints a progress heartbeat every 250 entries so you can see it is
    still working.  This is useful when scanning large collections where
    most archives are already extracted.

-enablecustomicons

    Applies drawer icons to newly created destination drawers during
    extraction.  The program looks for a matching icon file in:
      PROGDIR:Icons/<drawer_name>.info

    If no matching icon is found, a default Workbench drawer icon is used
    instead.  Existing .info files are never overwritten.  Icon failures
    are non-fatal and counted in the summary.

-debug

    Writes a timestamped debug log file to the program directory:
      error-YYYYMMDD-HHMMSS.txt

    This log captures additional context for failures, fallback events,
    and other diagnostic details not shown on the console.

-output=script|normal|verbose

    Controls the amount of console output.

    script   - Minimal output.  Suppresses most status messages and
               redirects archive tool output to NIL:.  Prints one
               progress line per archive during extraction:
                 Extracting: <archive>
               Warnings and errors are still shown.

    normal   - Default output level.  Standard status messages are
               printed during the run.

    verbose  - Normal output plus less-suppressed LHA extraction flags.
               LZX output is unchanged.

-writesummary

    Writes a machine-readable summary of the run to a fixed-name file:
      PROGDIR:WHDArchiveExtractor_last_run.txt

    This file is overwritten on each run.  It contains key=value pairs
    including source and destination paths, archive counts, extraction
    results, elapsed time, and overall result status.  This is intended
    for scripts or other tools that need to inspect the outcome of a run.


HOW IT WORKS
------------

Startup

  The program checks for required tools at startup.  LHA must be present
  in C: or the program exits immediately.  UnLZX/LZX in C: is optional;
  if missing, LZX archives are detected but skipped.

Scanning

  The source folder is scanned recursively.  Every .LHA and .LZX file
  found is queued for processing.  File extension matching is
  case-insensitive.

Destination Resolution

  For each archive, the program resolves the destination drawer path by
  combining the output directory with the relative subfolder structure
  from the source tree.  For LHA archives, the program inspects the
  archive listing to determine the top-level drawer name inside the
  archive and uses that as the destination drawer name.  For LZX archives
  or when no top-level drawer is found, the archive filename (without
  extension) is used.

Conflict Detection

  Before extraction, the program checks for conflicts:
  - If a file (not a drawer) already exists at the destination path, the
    archive is skipped and counted as a conflict.
  - If two archives in the same source folder resolve to the same
    destination drawer, the second archive is skipped and counted as a
    conflict.

Extraction

  In extraction mode, the program creates the destination drawer path
  first, then runs the appropriate archive tool command.  If destination
  drawer creation fails, the program falls back to letting the archive
  tool create the folder itself.

  For LHA archives that overwrite an existing destination, the program
  clears protection bits on existing files before extraction to avoid
  overwrite failures.

  The program stops scanning if 40 extraction errors are reached in a
  single run.

Summary

  At the end of the run, a summary is printed showing totals for archives
  found, extracted, skipped, and failed, along with elapsed time and any
  error details.


LOG FILES
---------

The program can create several log files depending on the options used:

  error-YYYYMMDD-HHMMSS.txt
    Debug log, created when -debug is active.

  New-YYYYMMDD-HHMMSS.txt
    List of newly extracted archives, created when -skipifdestexists is
    active in extraction mode.

  test_errors-YYYYMMDD-HHMMSS.txt
    List of failed archive tests, created when -testarchivesonly is active
    and test failures occur.

  WHDArchiveExtractor_last_run.txt
    Machine-readable run summary, created when -writesummary is active.
    Located in PROGDIR: and overwritten each run.


MORE TOOLS
----------

For other related Amiga utilities, including WHDFetch, which can
download, filter, and extract Retroplay's WHDLoad archives 
directly on the Amiga, visit:

  https://www.madeforworkbench.com
