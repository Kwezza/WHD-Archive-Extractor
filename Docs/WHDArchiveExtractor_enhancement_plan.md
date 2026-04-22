# WHD Archive Extractor: Focused Enhancement Plan

## Purpose of this document

This document outlines four small but useful enhancements for **WHD Archive Extractor**. It also explains why the program will not receive major new workflow features, and how each proposed enhancement would work in theory.

The aim is to improve the program for users who already have archive-based workflows and scripts that they are happy with, while keeping the tool small, predictable, and easy to use on real Amiga systems.

## Why there will not be major new enhancements

WHD Archive Extractor now sits alongside **WHDFetch**, which has effectively become the more complete and modern tool.

WHDFetch goes much further than simple archive extraction. It can:

- download Retroplay packs directly from the Turran server
- compare against DAT listings
- detect new or changed archives
- maintain local tracking files and extraction markers
- estimate storage requirements before a real run
- apply machine and content filters
- produce reports and maintain a longer-term collection workflow

Because of that, it no longer makes sense to add major collection-management features to WHD Archive Extractor.

If those features were added here as well, the result would likely be:

- duplicated effort across two tools
- a more complex and less predictable extractor
- blurred boundaries between the old archive extractor and the newer full workflow tool
- pressure to backport more and more WHDFetch behaviour over time

That is not the direction this tool should take.

## Intended role of WHD Archive Extractor

WHD Archive Extractor should remain:

- a small command-line tool
- archive-focused
- script-friendly
- offline
- stateless, or as close to stateless as practical
- easy to understand and easy to trust

In simple terms:

- **WHD Archive Extractor** is for users who already have the archives and just want to process them.
- **WHDFetch** is for users who want a full download, compare, update, and extraction workflow.

That division gives both programs a clear purpose.

## Design rule for future changes

A useful boundary rule is:

**WHD Archive Extractor may get smarter about the current run, but it should not start remembering the past.**

That means enhancements should focus on:

- improving decisions made during the current scan and extraction run
- reducing wasted work
- making output clearer and safer
- helping script users detect problems early

It should avoid:

- tracking databases
- marker files
- DAT parsing
- update history
- download logic
- long-term collection state management

## The four proposed enhancements

The following four changes fit well within that boundary.

---

# 1. Skip extraction if destination already exists

## Why this feature is worth adding

This is the most useful enhancement for users running the tool repeatedly on slow Amiga systems, especially those using their own scripts.

At present, the program derives the destination folder path as part of its normal work. Once that destination path is known, the program could also check whether the folder already exists.

If it does exist, and if the user has explicitly enabled this mode, the program can assume the archive has already been extracted and skip doing the work again.

This would:

- reduce unnecessary disk I/O
- speed up repeat runs
- help interrupted jobs resume more efficiently
- give more value to users who already have established archive workflows

## Why it should be optional

The existence of a destination folder is only a practical shortcut. It does **not** prove that the archive was extracted fully or correctly.

For example, the folder may already exist because:

- a previous extraction was interrupted
- the user created it manually
- a different archive mapped to the same destination
- an older version of the archive was extracted there previously

Because of that, this behaviour must be opt-in only.

Default behaviour should remain unchanged.

## Suggested command-line argument

Possible names:

- `-skipifdestexists`


## Theoretical behaviour

When the flag is enabled:

1. Scan for a supported archive as normal.
2. Derive the destination extraction path as normal.
3. Check whether the destination path already exists.
4. If it does not exist, continue normally and extract.
5. If it does exist and is a directory, skip extraction for that archive.
6. Record that the archive was skipped for summary reporting.
7. Print a clear message such as: `Skipping: destination already exists`.

## Important limits

This mode should not try to:

- compare timestamps
- compare archive contents
- inspect previous extraction history
- create persistent markers

It should remain a simple fast-path mode.

---

# 2. Destination conflict detection

## Why this feature is worth adding

If the program is going to make decisions based on destination paths, then it should also warn when two archives appear to target the same destination.

This is useful even without the skip feature, because it helps users spot naming clashes and script mistakes.

Example problem cases:

- two different archives resolve to the same output folder name
- the destination path exists as a file rather than a directory
- an archive name implies a destination that is ambiguous or suspicious

Without detection, these cases could be confusing and may lead to silent overwrites, misleading skips, or incorrect assumptions.

## Why it fits the program

This feature is still completely about the current run. It does not require any saved history or metadata.

It simply makes the scan phase smarter and safer.

## Theoretical behaviour

During scanning:

1. Each supported archive is mapped to its intended destination path.
2. The program keeps an in-memory record of destinations already claimed during the current run.
3. When another archive maps to the same destination, the program flags a conflict.
4. The conflict is shown in output and included in the final summary.

Possible responses when a conflict is found:

- warn and continue
- warn and skip both conflicting archives
- warn and skip only the later archive

The simplest and safest initial behaviour would be:

- warn clearly
- do not silently merge behaviour
- leave extraction disabled for the conflicting archive unless the behaviour is already well understood

## Example output

- `Conflict: two archives map to the same destination drawer`
- `Conflict: destination path exists but is not a drawer`

## Important limits

This should remain basic and practical. It should not attempt advanced archive identity analysis.

Memory-saving note for current implementation:

- destination claim tracking is flushed per scanned folder scope
- conflict detection therefore applies within the current folder scope
- same-destination conflicts across different folder scopes in one run may not always be detected

---

# 3. Dry-run mode

## Why this feature is worth adding

A dry-run mode would be very useful for script users and cautious users on real hardware.

It would let the program scan archives, build destination paths, apply optional rules such as skip-if-destination-exists, detect conflicts, and report what it *would* do, without actually performing extraction.

That would help users:

- validate source and output paths
- test batch jobs safely
- preview the effect of new command-line options
- identify conflicts before a long extraction run

## Why it fits the program

Dry-run mode does not add any long-term state or workflow logic. It simply reuses the current run's decision-making and suppresses the final extraction step.

This is exactly the sort of power feature that suits a command-line utility.

## Suggested command-line argument

Possible names:

- `-dryrun`
- `-reportonly`
- `-whatif`

The clearest option is probably:

`-dryrun`

## Theoretical behaviour

When enabled:

1. Scan source folders as normal.
2. Identify supported archives.
3. Derive destination paths.
4. Apply optional checks such as skip-if-destination-exists.
5. Detect conflicts and report them.
6. Print the planned action for each archive.
7. Do not call the extraction command.
8. Produce a final summary as if it were a real run, but clearly marked as a dry run.

## Example planned actions

- `Would extract`
- `Would skip: destination already exists`
- `Would warn: destination conflict`
- `Would fail: invalid output path`

## Important limits

Dry-run mode should report only what the program can know from scanning and path resolution. It should not pretend to validate the internal contents of the archive.

---

# 4. Better end-of-run summary reporting

## Why this feature is worth adding

A better summary gives immediate value even if a user never enables any of the other new options.

Users running larger jobs benefit from being able to see, at a glance, what happened during the run.

This is especially useful for:

- long script-driven runs
- slower Amiga systems where reruns are costly
- troubleshooting unexpected results
- checking the effect of new options like skip or dry-run

## Why it fits the program

This is an output and usability improvement, not a major behavioural change.

It keeps the program simple while making it more transparent.

## Theoretical behaviour

At the end of the run, print a compact summary block.

Suggested counters:

- total files scanned
- supported archives found
- archives extracted
- archives skipped because destination existed
- conflicts detected
- archive test-only checks performed
- extraction failures
- unsupported archives ignored

If dry-run mode is enabled, the summary should say so clearly.

If no optional features were used, the summary should still remain useful and uncluttered.

## Example summary

```text
Run Summary
-----------
Files scanned: 1240
Supported archives found: 412
Archives extracted: 398
Skipped because destination existed: 9
Destination conflicts: 2
Extraction failures: 3
Mode: normal
```

Or in dry-run mode:

```text
Run Summary
-----------
Files scanned: 1240
Supported archives found: 412
Would extract: 401
Would skip because destination existed: 9
Destination conflicts: 2
Mode: dry-run
```

## Important limits

The summary should remain short and readable. It does not need to become a full reporting system.

---

## Why these four features work well together

These changes form a good set because they all improve the **current run** without introducing persistent state.

Together they provide:

- faster repeat runs through destination-based skipping
- safer behaviour through conflict detection
- better control through dry-run mode
- clearer visibility through improved summary reporting

They also complement one another naturally:

- destination conflict detection makes skip-if-destination-exists safer
- dry-run mode lets users preview skip and conflict behaviour
- summary reporting helps users understand the overall result

## Features deliberately not planned

To keep the product boundary clear, the following should remain outside the scope of WHD Archive Extractor:

- archive download support
- DAT parsing or DAT comparison
- text marker files for extraction tracking
- local index files
- new-versus-updated archive tracking across runs
- machine or language filters
- storage estimation mode
- INI-based configuration systems
- full report/session file frameworks

These are now part of WHDFetch's role.

## Optional end-of-run mention of WHDFetch

Since WHDFetch is the newer and more capable workflow tool, a brief note at the end of a run could be useful.

This should be framed as a helpful pointer, not a sales message.

Possible example:

```text
Tip: For automatic downloading, update detection, and extraction tracking,
see WHDFetch.
```

A slightly fuller version:

```text
Tip: If you want a full WHDLoad collection workflow with downloading,
update detection, and extraction tracking, see WHDFetch.
```

This keeps the older tool useful while making users aware that a broader option exists.

## Final recommendation

WHD Archive Extractor should continue to evolve, but only in small, focused ways.

The right path is to make it a sharper archive extraction tool for people who already like its current script-friendly model, rather than turning it into a partial clone of WHDFetch.

The four enhancements in this document support that goal well:

1. Skip extraction if destination already exists
2. Destination conflict detection
3. Dry-run mode
4. Better end-of-run summary reporting

These are practical, easy to justify, and unlikely to damage the program's identity.
