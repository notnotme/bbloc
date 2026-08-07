# Plan 1 — Widen the unit tests beyond the undo history

> Delete this file in the commit that completes the plan.
> One commit per task below.

## Goal

Cover the classes `bbloc_tests` already links but never exercises, picking the invariants whose
breakage is **silent** rather than chasing coverage.

## Why

The target covers one subsystem: all 21 cases in `tests/UndoTests.cpp` go through `Cursor`'s undo
API. What else it links, and what a bug there would cost:

- **`LineBuffer`**, the foundation the editor sits on and the subtlest class in the repo. Its
  "current line" is *detached* from the main buffer while being edited: the characters live in
  `m_current_line`, the `m_line_data` entry is pinned at `count = 0`, and every byte offset for a
  line after it is corrected by `detachedLengthBefore()`. Those offsets are what the highlighter
  hands tree-sitter, so a mistake corrupts syntax highlighting silently instead of crashing.
  Commit `21ffe52` already had to fix a multi-line insert and an empty-insert edit here.
- **`LongestLineTracker`**, which keeps a per-line metric updated from each `BufferEdit` and only
  rescans when the longest line shrank or the tab weight changed. It feeds the horizontal scroll
  bound, so a stale maximum is a visible bug nothing would catch today.
- **`Cursor`'s non-undo half** — vertical moves clamping to a shorter line, `snapToCharBoundary`
  pulling a column off a surrogate pair, selection ranges normalising a backwards anchor. This is
  the encoding-hazard surface the project cares most about, and only its undo interaction is covered.
- **`advancePosition`** (`buffer/BufferEdit.h`), used by every undo step and only reached indirectly.

## Design

### Test the buffer against a reference model

The `LineBuffer` bugs worth catching are the ones where the class disagrees with itself after a
particular *sequence* of edits — a detached line never committed, an `m_line_data` offset left stale
by a multi-line splice. Hand-written expectations find those only if the author guessed the sequence.

`BufferTests.cpp` therefore keeps a plain `std::vector<std::u16string>` model beside the buffer,
applies each operation to both, and after **every** operation asserts the buffer still agrees with
the model on all its observables:

- `getStringCount()` and every `getString(line)`
- `getByteOffset(line, column)` at each line start and end, against the model's own arithmetic
  (characters before + one separator per preceding line, times `sizeof(char16_t)`)
- `getByteCount()` over assorted ranges
- `getLongestLineLength(weight)` and `getLineTabCount(line)`

A single `checkMatches(buffer, model)` helper does all of it, so a case is a sequence of edits with
a call after each. That is what makes the detachment reachable: editing line 3, then 0, then 3 again
forces commit-and-redetach, and the model notices any drift at once.

The returned `BufferEdit` is checked too — `start <= old_end`, byte offsets consistent with the
points — since that struct is the highlighter's entire input.

Navigation, selection and encoding have small well-defined answers; a model buys nothing there, so
those are ordinary case-by-case tests.

## Tasks

- [ ] **1 — `LineBuffer` and `LongestLineTracker` against a reference model.** New
      `tests/BufferTests.cpp`, model helper in `TestSupport.h`. Single-line and multi-line insert;
      erase inside a line, across lines, whole-buffer; insert and erase at the very end; the
      empty-insert degenerate edit; `clear()`; **edits alternating between distant lines**, to force
      the current line to commit and re-detach; byte offsets while a line is detached; longest-line
      growth *and* shrink (the shrink forces the rescan); tab weight changes. Also `advancePosition`
      directly: empty text, no newline, trailing newline, several lines.
- [ ] **2 — `Cursor` navigation, selection and encoding.** New `tests/CursorTests.cpp`. Vertical
      moves clamping to a shorter line; horizontal moves stepping a surrogate pair as one character;
      a vertical move landing mid-pair being snapped off it; `moveToStartOfFile` / `moveToEndOfFile`
      / page moves; `setPosition` throwing out of range and snapping a mid-pair column; a selection
      whose anchor is *after* the caret normalising to start→end; a degenerate selection reporting
      none; `getSelectedText` single-line and multi-line.
- [ ] **3 — `PromptCursor`, and close the plan.** New `tests/PromptTests.cpp`; add
      `src/core/cursor/PromptCursor.cpp` to `bbloc_tests` — a separate class from `Cursor`, with its
      own single-line editing and surrogate stepping, and no new dependency (`<string>` and
      `SurrogatePair.h` only). Insert, erase left and right over surrogate pairs, moves and bounds,
      `setPosition` bounds, `clear`. Delete this file in the commit.

## Files

`tests/BufferTests.cpp`, `tests/CursorTests.cpp`, `tests/PromptTests.cpp` (new),
`tests/TestSupport.h` (model helper), `CMakeLists.txt` (the new test sources plus
`PromptCursor.cpp`).

No `src/` behaviour changes are planned, so `docs/class_diagram.md` needs no update.

## If a test fails

These are written against code already shipping, so a red test is a **finding, not a fixture to
adjust**. Report it, and let the fix be its own commit with its own regression case — never quietly
reshape the expectation to match current behaviour. The exception is a genuinely intended behaviour
that was mispredicted, which gets said out loud in the commit message.

## Verification

```bash
cmake --build cmake-build-debug --target bbloc_tests && ./cmake-build-debug/bbloc_tests
```

Green after each task, with the 21 existing undo cases passing untouched. `tests/` adds nothing to
the toolchain (the hook lints `src/*` only), but task 3 touches `CMakeLists.txt`, so reconfigure the
desktop build if `nx/` was ever configured or `compile_commands.json` points at the Switch target
and clang-tidy silently no-ops.

## Out of scope

- Anything needing SDL, OpenGL, FreeType or tree-sitter — the target's cheapness is why it gets run,
  and `HighLighter`, the views and the commands each pull in one of those.
- `CVarInt` string parsing: reachable and cheap, but its failure mode is a visible error message in
  the prompt, not silent corruption.
