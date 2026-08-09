# Plan 1 — Test the OSK layouts and the key modifiers, and fix the documentation drift

> Delete this file in the commit that completes the plan.
> One commit per task below. Every existing test stays green throughout, unchanged.

## Goal

Spend the SDL-headers decision: `bbloc_tests` may compile against SDL headers as long as it links
no SDL library. That unlocks the two remaining pure-table units whose failure nobody would ever
see — `OskLayout` and the key-modifier folding inside `BindCommand` — and the plan finishes with the
documentation drift an audit of `CLAUDE.md`, `README.md`, `docs/manual.md` and
`docs/class_diagram.md` turned up.

## Why

**`OskLayout`** decides what every on-screen key displays and emits, across nine layouts. Its
failure mode is the quietest in the repo: one key types the wrong character on one layout, and
nobody working on a qwerty desktop will ever see it. The design has three moving parts that can each
be wrong on their own:

- a **fixed US base** shared by every layout (digits always plain, Shift giving the US symbols,
  punctuation keeping US semantics);
- a **letter permutation** per layout, where the slot and the displayed letter part company —
  azerty puts `a` on the US `Q` slot and `q` on the US `A` slot;
- an **accent map keyed by the displayed letter, not by the slot**. That is the subtle one: in
  azerty, AltGr on the `Q` slot must give `à` (the slot shows `a`), and AltGr on the `A` slot must
  give `œ` (the slot shows `q`, and `q` carries the œ accent). Swap those two and the layout still
  looks plausible.

`OskLayout.cpp` calls **no SDL function** — it uses `SDL_Scancode` constants only — so this task
needs headers, not a library, and no `src/` change at all.

**`BindCommand::normalizeModifiers`** folds the left/right variants of Ctrl, Shift, Alt and GUI into
their pair bits, passes the two pad-shoulder bits through, and drops everything else. That last part
is load-bearing and unstated: Caps Lock or Num Lock being on must not stop a binding from matching.
Get it wrong and a binding is silently unreachable in one keyboard state — the kind of thing that
gets reported as "it sometimes doesn't work". `mapModifier` and the `MODIFIER_MAP` behind it turn
`Ctrl+Shift` into bits and feed the completion of modifier names.

They are private statics of a `Command<CursorContext>` subclass, so they are unreachable by
construction and have to move first, the way `CommandLine` did.

## Design

**The SDL-headers line** (task 1):

```cmake
target_include_directories(bbloc_tests PRIVATE $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>)
```

Headers only — `bbloc_tests` still links no SDL, and the rule in CLAUDE.md becomes "compiles against
SDL headers, links no SDL library". Time the target from clean before and after; if it stops
building in seconds, the premise is gone and the task is reverted rather than argued with.

**`KeyModifiers`** (task 3): `src/core/base/KeyModifiers.{h,cpp}`, static-only like `CommandLine` and
`PadInput`, holding the `MODIFIER_MAP` and three methods — `normalize(uint16_t)`,
`fromName(std::u16string_view)` and `forEachName(const AutoCompleteCallback &)`. The map moves with
them because `BindCommand::provideAutoComplete` enumerates its keys; leaving the map behind would
mean exposing it, and `PadInput::forEachName` is the precedent for handing the enumeration out
instead. `BindCommand` keeps everything else and calls the three. The header includes
`<SDL_keycode.h>`, `AutoCompleteCallback.h` and `U16StringMap.h` — no `CursorContext`.

**Testing `OskLayout`** (task 2) is mostly a table read, so the cases that earn their place are the
ones a table read cannot get right by accident: the displayed-letter accent lookup, and two sweeps
over all nine layouts and all 256 scancodes asserting the invariants the hybrid design promises —
a key that has a plain form never blanks out under any modifier combination, digits stay digits
everywhere, and **every accent entry is reachable** (its letter is displayed by some slot in that
layout). A dead accent entry is a typo that shows up as one missing character on one layout, which
is exactly the class of bug this whole task exists for.

## Tasks

- [ ] **1 — Let the test target see the SDL headers.** `CMakeLists.txt`: the
      `target_include_directories` line above, plus `src/osk/OskLayout.cpp` in `bbloc_tests`. Update
      the one sentence in CLAUDE.md's Testing section that states the no-SDL rule, since this is the
      rule change — the coverage list waits for task 6. No test file yet: the commit is the
      dependency decision, reviewable on its own. Report the clean build time before and after.
- [ ] **2 — Test `OskLayout`.** New `tests/OskLayoutTests.cpp`.
      `findLayout`: each of the nine names resolves; an unknown name, a prefix (`qwert`), a
      different case (`QWERTY`) and an empty name all give nullptr. `defaultLayout` is qwerty and
      carries no override and no accent.
      The US base through `resolve`: letters lower and upper; digits plain and their US shifted
      symbols (`1` → `!`, `0` → `)`); a slot the base does not define (a function key) → nullptr.
      The permutation: azerty `Q` slot → `a`/`A`, `A` slot → `q`/`Q`, `SEMICOLON` → `m`,
      `M` → `;`/`:`; qwertz `Y`/`Z` swapped; russian `Q` → `й` and its extension onto punctuation
      slots (`SEMICOLON` → `ж`, `SLASH` → `ё`).
      The accents, keyed by the **displayed** letter: azerty AltGr on the `Q` slot → `à`, Shift+AltGr
      → `À`; AltGr on the `A` slot → `œ` (that slot displays `q`); AltGr on `E` → `é`/`É`; a letter
      with no accent in the layout falls back to its plain column rather than blanking; AltGr on any
      slot in a layout with no accent map (russian) is the plain letter.
      `forEachName` enumerates exactly the nine names, in table order.
      Then the two sweeps over every layout and every scancode: a slot with a plain form has a
      non-null form under all four shift/altgr combinations, and digits resolve to themselves in
      every layout. Plus the reachability check: every accent's letter is displayed by some slot of
      the layout that declares it.
- [ ] **3 — Extract the key modifiers out of `BindCommand`.** New `src/core/base/KeyModifiers.{h,cpp}`
      with `MODIFIER_MAP`, `normalize`, `fromName` and `forEachName`, bodies moved unchanged.
      `BindCommand` drops the three members and calls the new class; check whether its `<ranges>`
      include is still needed once `std::views::keys` leaves. Add the `.cpp` to **both** targets.
      Diagram in the same commit. No tests here — the diff is "is this a no-op?".
- [ ] **4 — Test `KeyModifiers`.** New `tests/KeyModifiersTests.cpp`. `normalize`: each left and
      right variant folding to its pair bit, both together folding to the same, combinations,
      `KMOD_NONE` → 0, the two pad bits passing through alone and alongside a keyboard modifier, and
      — the one that matters — `KMOD_CAPS` and `KMOD_NUM` being **dropped**, so a binding still
      matches with Caps Lock on. Idempotence: normalizing twice changes nothing. `fromName`: the six
      names map to their bits, `None` maps to zero, and an unknown name, a wrong case (`ctrl`) and
      an empty name give -1. `forEachName` enumerates exactly the six.
- [ ] **5 — Fix the documentation drift the audit found.** Four small, unrelated corrections, all
      verified against the code (details under *Audit* below): the `Platform` seam paragraph in
      CLAUDE.md, a testing note in README.md, the OSK layout default in `docs/manual.md`, and the
      `Color` struct in `docs/class_diagram.md`.
- [ ] **6 — Describe the new coverage, and close the plan.** CLAUDE.md's coverage list gains
      `OskLayoutTests.cpp` and `KeyModifiersTests.cpp`, and the "links … only" sentence gains
      `OskLayout` and `KeyModifiers`. Delete this file in the commit.

## Audit

What was checked against the tree, so it is not repeated:

**Clean, leave alone.** All 27 registered commands and all 46 registered CVars are documented in
`docs/manual.md`, with no phantom entries in either direction (`tab_to_space` and `show_scrollbar`
are registered by `Editor`, which is why a grep of `ApplicationWindow`/`Theme`/`PromptState` alone
makes them look missing — they are not). The nine OSK layout names in the manual match the table.
The README language lists match `ParserCatalog` and `CMakeLists.txt`. The four names in
`class_diagram.md` with no matching class — `Renderer`, `Shader`, `SurrogatePair`, `TabStop` — are
deliberate pseudo-class boxes carrying `<<free functions>>` / `<<OpenGL module>>` stereotypes, not
drift.

**Drift, fixed in task 5.**

1. **CLAUDE.md, the `Platform` seam paragraph** lists `assetPath()`, `userConfigDir()` and
   `preferredColorScheme()`, but `Platform` also has `keyboardLayout()` (the console keyboard
   setting; `"qwerty"` on desktop) and `addControllerMappings()` (no-op on desktop). The class
   diagram already names both, so CLAUDE.md is the one out of date.
2. **README.md never mentions the test suite.** Neither *Building*, nor *Project Status →
   Implemented*, nor *Contributing → Code Style* says a doctest target exists, let alone how to run
   it — and it is now 142 cases over eight files. A short *Testing* note under *Building*, with the
   two-line command, plus a line in *Implemented*.
3. **`docs/manual.md`, the on-screen keyboard section** documents `osk layout <name>` and the nine
   names, but not where the initial one comes from: `OskState` asks `Platform::keyboardLayout()`,
   so on Switch the OSK follows the console keyboard setting and on desktop it starts on qwerty,
   with `osk layout` overriding either. The Switch paragraph further down documents the console
   colour set the same way and should mention this beside it.
4. **`docs/class_diagram.md` never models `Color`** (`core/cvar/Color.h`), although `CVarColor` is a
   `TypedCVar<Color>` and `Theme` hands `Color` values to every view. Section 2 is where it belongs.
   `U16StringViewHash`/`U16StringMap` are also unmodelled; leave them — a transparent-hash alias is
   a spelling, not a relationship, and the diagram is not a header index.

## Files

New: `src/core/base/KeyModifiers.{h,cpp}`, `tests/OskLayoutTests.cpp`, `tests/KeyModifiersTests.cpp`.
Changed: `CMakeLists.txt`, `src/command/BindCommand.{h,cpp}`, `docs/class_diagram.md` (tasks 3 and
5), `CLAUDE.md` (tasks 1, 5 and 6), `README.md` and `docs/manual.md` (task 5).

## If a test fails

Written against shipping code, so a red case is a **finding, not a fixture to adjust** — most likely
in a layout table, where the fix is a table entry and belongs in its own commit. Report it and stop.
Every case must also be shown to bite: mutate the code under test, confirm red, revert — with the
mutation script failing loudly on a build error, since a mutant that does not compile re-runs the
stale binary and reads as a survivor.

## Verification

```bash
cmake --build cmake-build-debug && cmake --build cmake-build-debug --target bbloc_tests && ./cmake-build-debug/bbloc_tests
```

Task 3 is proven by identity first (`git show --color-moved=zebra`, and a diff of the moved bodies),
then by launching the app from the repository root: `romfs/autoexec` is 47 lines of `bind`, so a
broken modifier map means bindings that no longer fire, and a broken `normalize` means they fire
only in some keyboard states — check a `Ctrl+` chord and a bare key, and check `bind` completion
still offers the modifier names. Task 1 changes `CMakeLists.txt`, so **`nx/` must be reconfigured
from scratch and built** before this plan is considered done — that has been pending since the
previous plan added `CommandLine.cpp` and `/opt/devkitpro` does not exist on this machine.

## Out of scope

- **`SearchCommand::LineScanner`** — the last high-value unit, and `Cursor` is already linked so the
  whole scan could be pinned against a real buffer. It needs a nested private class promoted out,
  which substantially rewrites one header: its own plan.
- **`getPathCompletions`, `InputRepeater`, `Editor::computeScrollbarMetrics`, `ParserCatalog`** —
  rejected with reasons in the previous plan; the reasons have not changed.
- Anything needing SDL, GL, FreeType or tree-sitter **at link time**. The rule that moves in task 1
  is about headers only, and it moves that far and no further.
