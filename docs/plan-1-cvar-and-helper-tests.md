# Plan 1 — Test the CVar types and the text-metric helpers

> Delete this file in the commit that completes the plan.
> One commit per task below. Every existing test stays green throughout, unchanged.

## Goal

Extend `bbloc_tests` past the text core to the remaining pieces of `src/core/` that link with no
SDL, GL, FreeType or tree-sitter: the four CVar types, the tab-stop math and the surrogate-pair
helpers.

## Why

The suite covers `LineBuffer`, `LongestLineTracker`, `Cursor`, `PromptCursor` and `UndoHistory`.
What is left inside the target's reach, and what a bug there costs:

- **The CVar types** (`core/cvar/`). Plan 1 of the previous round skipped them because a parse
  failure surfaces as a prompt message rather than corruption. That is true of the *rejection* path
  only. The acceptance path is where it hurts: `romfs/light_theme`, `romfs/dark_theme` and
  `autoexec` are command scripts, so every colour and dimension in the app arrives through
  `setValueFromStrings` at startup, where no one is reading the prompt. A channel silently taking
  the wrong value, or `CVarFloat` accepting `nan` and handing a NaN dimension to the layout, shows
  up as a mis-drawn frame with no error anywhere.
- **`CVar::provideValueCompletion`** — the default splits `getStringValue()` on spaces and emits the
  *n*-th component, which is how `cvar col_editor_text <Tab>` completes a colour channel by channel.
  It is a hand-written scanner with skip-blank and out-of-range branches, and its failure mode is a
  completion that quietly offers nothing.
- **`nextTabStop` / `visualColumns`** (`core/theme/TabStop.h`). Commit `993cb00` made these the one
  place tab geometry is decided, precisely so the measure, the glyph walk, the selection quads and
  `columnAtPixel` cannot disagree. Nothing checks them: a wrong stop misplaces the caret against the
  glyphs, which is the definition of a silent visual bug.
- **`charLengthBefore` / `charLengthAfter` / `snapToCharBoundary`** (`core/cursor/SurrogatePair.h`).
  Reached today only through the cursors, and the last commit on `main` (`7221bfd`) fixed a caller
  that walked into the middle of a pair. Splitting a pair makes the buffer unencodable, so the
  damage lands at save time, far from the edit that caused it.

## Design

The CVar cases are ordinary case-by-case tests: the contract is a small table of accepted and
rejected strings, and a model would only restate it. Each case asserts on the pair the rest of the
app sees — the returned `std::optional` message and `getStringValue()` afterwards — never on the
member, so the tests survive a change of storage.

`visualColumns` gets a reference model instead, the shape `BufferTests.cpp` established: a naive
per-character walk that snaps on every `\t`, run against the real function over a set of strings
mixing tabs, runs of text, leading and trailing tabs and consecutive tabs. The optimisation under
test is exactly a fold of that walk into `std::find` runs, so the naive version is the specification.

Both metric headers are header-only, so tasks 2 and 3 add no source to the target.

## Tasks

- [ ] **1 — The CVar types.** New `tests/CVarTests.cpp`; add `CVarBool.cpp`, `CVarFloat.cpp` and
      `CVarColor.cpp` to `bbloc_tests` (`CVarInt.cpp` is already linked). Per type: the accepted
      forms, the round-trip through `getStringValue()`, the wrong-arity message, trailing garbage
      (`4x`), and the type's own edges — `stoi` overflow for int, `nan`/`inf` for float, out-of-range
      and non-integer channels for colour, the default alpha of 255, and `true`/`false` being the
      only booleans accepted. Then the completions: `CVarBool` offering exactly `false` and `true` at
      component 0 and nothing beyond, and the default scanner emitting each colour channel at its own
      index and nothing past the last. Read-only is asserted to be *ignored* by
      `setValueFromStrings`, which is the documented contract — enforcement lives in `CVarCommand`,
      which the target cannot link.
- [ ] **2 — The tab-stop math.** New `tests/TabStopTests.cpp`. `nextTabStop` from a column already
      on a stop, from one just before it and from column 0; a tab width of 1. `visualColumns` against
      the naive reference over the mixed set, plus the two properties the callers depend on: a
      tab-free prefix measures as its length (the fast path callers take), and the width never
      shrinks as the prefix grows.
- [ ] **3 — The surrogate-pair helpers, and close the plan.** New `tests/SurrogateTests.cpp`.
      `charLengthBefore` and `charLengthAfter` on a BMP character, on both halves of a pair, at
      column 0 and at the end of the line, and on a lone surrogate (which must measure 1, not read
      past the view). `snapToCharBoundary` at a trail unit, at a lead unit, at 0, at the length, and
      idempotent. One walk case per direction: stepping the length across a mixed line lands only on
      character boundaries and reaches the end exactly. Delete this file in the commit.

## Files

`tests/CVarTests.cpp`, `tests/TabStopTests.cpp`, `tests/SurrogateTests.cpp` (new),
`CMakeLists.txt` (the three new test sources plus the three CVar sources).

No `src/` behaviour changes are planned, so `docs/class_diagram.md` needs no update.

## If a test fails

These are written against shipping code, so a red case is a **finding, not a fixture to adjust**.
Report it and stop; the fix is its own commit with its own regression case. The float `nan`/`inf`
edge is the likely one, and it is a decision to take, not a bug to patch on the way past.

Every case must be shown to bite: mutate the code under test, confirm the case goes red, revert.

## Verification

```bash
cmake --build cmake-build-debug --target bbloc_tests && ./cmake-build-debug/bbloc_tests
```

Green after each task, with the 83 existing cases passing untouched. Task 1 touches
`CMakeLists.txt`, so reconfigure the desktop build if `compile_commands.json` ever pointed at the
Switch target.

## Out of scope

- **`CommandManager::tokenize` and `split`**, the prompt's parser and the highest-value untested
  logic left. `CommandManager.cpp` includes `CursorContext.h`, which pulls `HighLighter`, `Theme`
  and `CommandRunner` — tree-sitter, FreeType and SDL headers — so linking it would cost the target
  the cheapness that gets it run. Testing it means moving those two static functions to a header of
  their own first, which is a `src/` change and belongs to its own decision.
- **`PadInput`**, whose `keycodeFromName` parsing has the same silent failure (a binding that never
  fires). It needs SDL headers only, no SDL library, so it is one include directory away — but that
  breaches the rule that keeps the target dependency-free, and the rule is worth more than the case.
- `ViewState` and `U16StringMap`, which are getters and a `using` over `std::unordered_map`.
- Anything needing SDL, GL, FreeType or tree-sitter at link time.
