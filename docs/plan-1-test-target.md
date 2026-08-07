# Plan 1 — Test target for the undo history (before plan 2)

> Delete this file in the commit that completes the plan.
> One commit per task below.

## Goal

A minimal unit-test target covering `Cursor` + `UndoHistory`, written against the **current**
snapshot implementation and green on it. Plan 2 rewrites that implementation; these tests are what
proves the rewrite behaviour-preserving, so they must land first and must not be adapted to it
afterwards.

This is the project's first test target. It is deliberately narrow: it covers the undo subsystem
only, and exists because plan 2's inverse-application logic has edge cases — surrogate pairs,
multi-line groups, trimming — that a GUI pass cannot reach.

## Design

- **Framework**: doctest, vendored as a single MIT-licensed header at `tests/doctest.h`. No package
  manager involved — vcpkg here is classic-mode with no manifest, so a framework pulled from it would
  mean a manual install step. The header is never edited.
- **Target**: `bbloc_tests`, guarded by `if(NOT NINTENDO_SWITCH)` so the devkitPro build never sees
  it. It links five existing sources — `Cursor.cpp`, `UndoHistory.cpp`, `LineBuffer.cpp`,
  `LongestLineTracker.cpp`, `CVarInt.cpp` — plus `tests/UndoTests.cpp`. Their only includes are
  `<algorithm>`, `<string>` and `utf8.h`: no SDL, no OpenGL, no FreeType, no tree-sitter.
- **Toolchain**: `tests/` needs no hook or lint change. `misc/pre-commit` only globs `src/*.cpp` and
  `src/*.h`, and `.clang-tidy` sets `HeaderFilterRegex: '.*/src/.*'`, so the vendored header is not
  linted.
- The GPLv3 header convention applies to `tests/UndoTests.cpp`, not to the vendored `doctest.h`,
  which keeps its own MIT header.

## Tasks

- [ ] **1.1 — Test target.** Vendor `tests/doctest.h`. Add `tests/UndoTests.cpp` with one smoke case.
      Add `bbloc_tests` to `CMakeLists.txt`. Add doctest to `THIRD_PARTY_NOTICES.md`. Done when
      `./cmake-build-debug/bbloc_tests` runs green.
- [ ] **1.2 — Core undo/redo behaviour.** Round-trip: N edits then N undos restores the exact
      original text *and* cursor position; N redos restores the edited state. Redo invalidation: an
      edit after an undo clears the redo stack. Empty-history cases: undo and redo on a fresh buffer
      are no-ops.
- [ ] **1.3 — Granularity.** A typed run is one undo step; a line change starts a new one; a
      multi-line insert and a multi-line selection erase are each one step. This is the contract plan
      2 must not break, so it is pinned explicitly rather than left implicit.
- [ ] **1.4 — Edge cases, and close the plan.** Surrogate pairs: erasing and undoing across a non-BMP
      character never splits the pair and the text round-trips exactly. Trimming: exceeding the entry
      cap and exceeding the character cap both drop the oldest entries and leave the rest applicable.
      Delete this file in the commit.

## Files

`tests/doctest.h` (vendored), `tests/UndoTests.cpp`, `CMakeLists.txt`, `THIRD_PARTY_NOTICES.md`.

## Verification

`cmake --build cmake-build-debug && ./cmake-build-debug/bbloc_tests`, green against the unmodified
`UndoHistory`. Reconfigure the desktop build first if `nx/` was ever configured, so
`compile_commands.json` points back at the desktop target and the pre-commit hook actually lints.

If a case fails here, it has found a real pre-existing bug in the current implementation: report it
rather than adapting the test to it.
