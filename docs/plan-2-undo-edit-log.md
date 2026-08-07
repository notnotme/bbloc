# Plan 2 — Undo history as an inverse-edit log (after plan 1)

> Delete this file in the commit that completes the plan.
> One commit per task below. Every plan-1 test stays green throughout, unchanged.

## Goal

Stop retaining the whole document per undo step. Store what actually changed — a start position, the
text removed, the text inserted — and undo by applying the inverse.

## Why

`UndoHistory` stores full-buffer snapshots: every entry holds a `std::u16string` of the entire
document. Commit `21ffe52` already optimised the two costs reachable without changing that
representation (it narrowed the undo/redo `BufferEdit` to the differing range, 189000 code units → 1
for a one-character undo, and added a byte cap on top of the entry-count cap). What remains is the
representation itself:

- **Capture is O(document).** `Cursor::recordBeforeEdit()` calls `getText()`, joining every line into
  a fresh string, once per undo group. `newLine()` and the line-joining branch of `eraseLeft()` mark
  a boundary unconditionally, so holding Enter or Backspace under `InputRepeater` auto-repeat costs
  one full-document join per repeat tick.
- **Undo and redo are O(document) too** — both call `getText()` again to build the state they hand to
  the history.
- **`replace_all` retains a document copy per touched line.**
- **Peak memory is depth × filesize**, not depth × editsize. On a 1 MB file the 33.5M-code-unit
  character cap silently reduces the effective depth to ~33 groups; on a 10 MB file, ~3.

## Design

### The record

`UndoHistory::Snapshot` is replaced by two types:

```cpp
/** One text replacement, in the coordinates valid at the moment it is applied. */
struct Edit final {
    BufferEdit::Position start;   ///< Where the replaced range begins.
    std::u16string removed;       ///< Text present before, empty for a pure insert.
    std::u16string inserted;      ///< Text present after, empty for a pure erase.
};

/** A run of edits undone and redone as a single step. */
struct Group final {
    std::vector<Edit> edits;
    BufferEdit::Position cursor_before;  ///< Cursor at the group's start, restored by undo.
    BufferEdit::Position cursor_after;   ///< Cursor after the last edit, restored by redo.
    uint64_t id;                         ///< Monotonic identity, used by the saved-point marker.
};
```

Both stacks become `std::deque<Group>`.

**No rebasing is needed.** Groups are applied strictly in LIFO order, so each record's coordinates
are valid exactly when its turn comes. Positions are line/column, matching `BufferEdit::Position`;
`LineBuffer::getByteOffset` is O(1), so the byte offsets the highlighter needs stay free at apply
time.

### Grouping must not change

The boundary flag and every `markBoundary()` call site stay exactly as they are: a group opens when
an edit arrives at a boundary and closes at the next one, so the granularity the user feels is
identical to today. The plan-1 granularity tests are the guard.

Within an open group, adjacent records coalesce so a typed run stays one record: an insert whose
`start` equals the previous record's end-of-`inserted` appends to it; an erase whose end equals the
previous record's `start` prepends to its `removed`. That keeps a 60-character line at one record
rather than 60, which matters because each record costs one `ts_tree_edit` on undo.

### Applying

```
undo:  pop Group g from the undo stack
       for each Edit e in reverse(g.edits):
           end = advanceToIndex(e.inserted, from e.start)   // O(chunk), counts newlines
           buffer.erase(e.start, end); buffer.insert(e.start, e.removed)
       cursor -> g.cursor_before ; push g onto the redo stack
redo:  mirror image — forward order, removed/inserted swapped, cursor -> g.cursor_after
```

The erase+insert pair per record merges into one `BufferEdit` exactly as `Cursor::restore()` already
does. `Cursor::undo()/redo()` therefore return `std::vector<BufferEdit>` instead of
`std::optional<BufferEdit>`, and `UndoCommand`/`RedoCommand` loop over it feeding
`highlighter.edit(...)`. That is safe: `HighLighter::edit` is incremental and accumulates a dirty
line span across repeated calls before the next parse.

`Cursor::restore()` and its `getText()`-based diffing become dead and are deleted. The static helpers
`splitsSurrogatePair` and `advanceToIndex` are kept and reused — the inverse application needs the
same surrogate-boundary guarantee, and `advanceToIndex` converts a chunk length into an end position.
`getText()` itself stays; `SaveFileCommand` still uses it.

### What plan 1 deliberately left untested

The **character cap** has no test, on purpose. Its threshold is representation-specific: with
snapshots it bites at ~33 groups on a 1 MB file, and after this rewrite the same input would never
reach it. A test pinning today's behaviour would have to be rewritten by task 2.2, which is exactly
what the plan-1 tests exist to avoid. Reaching it also costs a ~68 MB fixture.

Task 2.2 must therefore re-check the character accounting by inspection rather than by a
pre-existing test, and may add a cap test afterwards written against the new thresholds.

## Tasks

- [ ] **2.1 — `Cursor::textInRange`.** The erase paths discard what they remove, so add a private
      `textInRange(lineStart, columnStart, lineEnd, columnEnd) -> std::u16string` built from
      `m_buffer->getString(line)` slices — O(range), same shape as `getSelectedText()`. Pure addition,
      no call sites yet. `getSelectedText()` is left untouched: `CopyTextCommand` depends on it
      returning views.
- [ ] **2.2 — The rewrite.** `Edit`/`Group` replace `Snapshot`; group open/coalesce/apply; character
      accounting over `removed` + `inserted`; `Cursor` records at its six mutation points (`insert`,
      `newLine`, `eraseLeft`, `eraseRight`, `eraseSelection`, `clear`); `undo`/`redo` return
      `std::vector<BufferEdit>`; `restore()` deleted; `UndoCommand`/`RedoCommand` loop. The entry and
      character caps and the `max_undo` CVar wiring keep their current semantics; the "never drop the
      last snapshot" rule in `trim()` goes away, since dropping the oldest group now only shortens
      reach. Updates `docs/class_diagram.md`.
- [ ] **2.3 — The file-open path.** `OpenFileCommand::loadInto` does `clear()` then
      `insert(content)` with the whole file. Today that snapshots the *empty* buffer and costs
      nothing; after 2.2 it would record a full-file copy and immediately free it — a regression. Add
      `Cursor::loadContent(std::u16string_view) -> std::vector<BufferEdit>` doing the clear + insert
      with recording suspended, resetting history and cursor; `loadInto` calls it and feeds the
      returned edits to the highlighter.
- [ ] **2.4 — Exact modified-tracking, and close the plan.** The buffer state is identified by the
      `id` on top of the undo stack, or 0 for an empty history. `setModified(false)` — the only form
      ever called — records that id as the saved point; `isModified()` returns `top id != saved id`.
      Undo and redo move groups between the stacks, so the top id tracks the state for free. If the
      saved group is trimmed away the saved state is unreachable: mark it so and return true from then
      on. `InfoBar`, `QuitCommand` and `BufferCommand` are unchanged. Add the two tests that could not
      exist before: save → edit → undo reports unmodified, and save → edit → trim past the saved group
      reports modified. Delete this file in the commit.

## Files

`src/core/cursor/UndoHistory.h/.cpp`, `src/core/cursor/Cursor.h/.cpp`,
`src/command/UndoCommand.cpp`, `src/command/RedoCommand.cpp`, `src/command/OpenFileCommand.cpp`,
`tests/UndoTests.cpp`, `docs/class_diagram.md`.

## Verification

Per task: `cmake --build cmake-build-debug && ./cmake-build-debug/bbloc_tests` green before
committing.

After 2.4, end to end:

1. GUI pass, same method as `21ffe52`: run under a nested Xephyr display (never `:0`), from the repo
   root, over open → edit → undo → redo → cut/paste → replace_all → save, and confirm the saved file
   is byte-identical to the pre-change build's output for the same key sequence.
2. The memory check that motivates the change: open a large file, hold Backspace and Enter through
   auto-repeat, and confirm retained history size tracks the edits rather than the file size.

## Out of scope

- `PromptCursor` — a separate `final` class with no undo history; its `PromptState` command history
  is unrelated.
- The undo granularity rules. Every `markBoundary()` call site is preserved as-is.
