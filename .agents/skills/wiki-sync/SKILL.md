---
name: wiki-sync
description: >-
  Audits and synchronizes the GitHub wiki documentation (doc/ultra.wiki) with the ULTRA codebase.
  Checks directory existence, verifies code snippets and API alignment against src/, fixes minor discrepancies while keeping docs cohesive, brief, and free of changelog residue, and provides clear action steps for major modifications.
---

# Wiki Documentation Sync

Sync and align documentation in `doc/ultra.wiki` with the ULTRA codebase in `src/`.

## 1. Directory Check

1. Check if `doc/ultra.wiki` exists and is a directory.
2. If it does not exist:
   - Report: `"Directory 'doc/ultra.wiki' not found. Skipping documentation check."`
   - Stop execution.

## 2. Documentation Standards

Maintain documentation that is clear, coherent, cohesive, and brief:
- **Current state only**: Document the codebase as it is today. Do not add or retain changelog entries, version notes, or "previously X, now Y" comparisons (except in `migrating.md`).
- **Cohesion over patching**: Integrate edits seamlessly into existing prose; avoid appending disjointed notes or transitional addenda.
- **Brevity**: Prune obsolete explanations, redundant remarks, and historical baggage.

## 3. Alignment Verification

When `doc/ultra.wiki` exists, verify alignment between wiki documentation and the codebase:

1. **Tutorials and Examples**:
   - Match wiki tutorial pages (`doc/ultra.wiki/*tutorial*.md`, `symbolic_regression*.md`...) with their corresponding example sources in `src/examples/` (note: examples often link to the wiki page via a `\see` comment in the file header).
   - Verify that code snippets in the wiki accurately reflect the C++ code in `src/examples/`.
2. **API and Core Architecture**:
   - Check API references and code samples in `doc/ultra.wiki/anatomy.md`, `dataframe.md`, `migrating.md`, etc., against headers in `src/kernel/` and `src/utility/`.
   - Verify method names, accessor signatures, class names, and namespaces.
3. **Repository Links**:
   - Check markdown links referencing files in the repository (e.g. `https://github.com/morinim/ultra/blob/main/...` or relative paths) to ensure the target files exist and haven't moved.

## 4. Automatically Fix Minor Discrepancies

Apply direct, minimal edits to `doc/ultra.wiki/*.md` for straightforward discrepancies:
- **Syntax and accessor updates**: e.g., updating property access to method calls (`result.best_individual` → `result.best_individual()`, `*res.best_measurements.fitness` → `*res.best_measurements().fitness`).
- **Moved files and header paths**: update broken links or include paths (e.g. `src/examples/pathfinding01.cc` → `src/examples/pathfinding/pathfinding01.cc`, `kernel/search.h` → `kernel/search_stats.h` for `search_stats`).
- **Namespace updates**: e.g. updated namespace qualifiers matching current code.
- **Direct snippet synchronization**: minor parameter or variable updates that have a 1-to-1 match in the source example.
- **Clean historical residue**: prune leftover changelog commentary or obsolete remarks encountered during edits.

Keep edits uncommitted in `doc/ultra.wiki` so the user can inspect changes with `git diff`.

## 5. Report Major Modifications

Do **not** edit wiki pages for major conceptual changes. Instead, provide a clear report explaining what the user needs to do:
- **Architectural redesigns**: when an example or component was completely rewritten and the tutorial explanation no longer applies.
- **Missing documentation**: new features or examples in `src/` (e.g., `src/examples/whichlang/`) that do not yet have a corresponding wiki tutorial.
- **Removed features**: when an entire subsystem described in the documentation was deleted or replaced.

For each major modification, report:
- **File & Section**: Location in `doc/ultra.wiki/`.
- **Reason**: What changed in the codebase.
- **Action Plan**: Specific step-by-step instructions on what narrative, code, or diagrams to update cohesively without changelog clutter.
