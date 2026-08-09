# Plan 1 — Extract the command-line parser, and test it

> Delete this file in the commit that completes the plan.
> One commit per task below. Every existing test stays green throughout, unchanged.

## Goal

Put `tokenize` and `split` somewhere `bbloc_tests` can link them, then pin them. Then do the same
for `PromptState`, which is pure logic held out of reach by a single over-wide reference.

## Why

`romfs/autoexec`, `romfs/light_theme` and `romfs/dark_theme` are command scripts, so every key
binding and every colour in the app arrives through `CommandManager::tokenize` before the first
frame. A tokenizer that mis-splits a line produces `Unknown command: …`, which lands in a prompt
message the next script line immediately overwrites: the editor comes up with one binding missing
and nothing anywhere says so. This is the same argument the CVar tests were written on.

The functions are already `static` and already pure — they touch no member. They are unreachable
only because `CommandManager.cpp` includes `CursorContext.h`, and so tree-sitter, FreeType and SDL.

There is a second contract worth pinning. `AutoCompleteCommand::run` does pointer arithmetic *into
the caller's string* through the token views:

```cpp
const auto *last_token_end = tokens.empty() ? input_end : tokens.back().data() + tokens.back().size();
reconstituted_command = input.substr(0, tokens.back().data() - input.data());
```

That only works because the tokens alias the input. Nothing states it, nothing checks it, and a
`tokenize` that ever materialised or trimmed a token would silently corrupt the completion prefix
instead of failing to compile.

`PromptState` (tasks 4–6) is the same situation for a different reason. `PromptState.cpp` includes
`<algorithm>` and nothing else — it is a command-history ring, a completion ring, a trim and a
clamp — but `PromptState.h` includes `CommandManager.h` to hold a `CommandManager &`, whose only use
in the whole class is one `registerCvar` call. Narrowing that member to the `CVarRegistry` interface
makes the class linkable, and `58b1c25` "Register the theme CVars through CVarRegistry" already did
exactly this for `Theme`. What breaks silently there: the two negative-modulo wraps
(`((i - 1) % n + n) % n`), the rule that the first press of Up *or* Down returns the most recent
entry, and the `dim_max_history` trim. All three fail by showing the wrong history entry, which
reads as "I mistyped", never as "the editor is broken".

## Design

`src/core/base/CommandLine.h` + `.cpp`, a static-only class in the established idiom
(`CommandLine() = delete;`, like `PadInput` and `Platform`), holding `tokenize` and `split`.

- **`core/base/` and not `core/`**, because `core/base/` is the layer that knows nothing about
  `CursorContext` — every other file there is already free of heavy includes. That makes "this unit
  links with nothing" a property of the directory rather than an accident the next change can undo.
  `CommandLine.h` includes `<string_view>` and `<vector>`, and that is the whole list.
- **A `.cpp`, not a header-only helper.** `TabStop.h` and `SurrogatePair.h` are header-only because
  they run per character inside the render walks. A 40-line parser called once per command line
  belongs in the `PadInput`/`OskLayout` shape, out of every TU that merely calls it.
- **The name.** The thing being parsed is a command line — typed at the prompt, bound to a key, read
  from an `exec` script. `CommandParser` was rejected: the tree already has a `Parser` (tree-sitter).
  `split` is a generic split rather than command syntax, and hosting it here is the one judgement
  call in the design; a second unit for one 18-line function would be worse.
- **No forwarding wrappers on `CommandManager`.** Three call sites, three one-line edits. A
  forwarding static would keep the old spelling alive forever for nothing.

The three call sites: `ApplicationWindow.cpp:555`, `AutoCompleteCommand.cpp:68`,
`BindCommand.cpp:97`. Only `AutoCompleteCommand.cpp` drops its `CommandManager.h` include (that call
is its only reason for it); the other two use the manager for other things and keep it.

Task 2 tests the parser the way `BufferTests.cpp` tests the buffer: named cases for the edges, plus
a **differential sweep** against an independent state machine written in the test file, run over
every string up to length 6 drawn from `{'a', ' ', '"', '\t'}` and every string up to length 7 from
`{'a', 'b', '+'}`. Four thousand-odd inputs cost milliseconds and cover the sequences nobody would
think to write down.

## Tasks

- [ ] **1 — Move the parser out of `CommandManager`.** New `src/core/base/CommandLine.h` and
      `CommandLine.cpp`, with the two bodies moved **character-identical** apart from the qualifier
      and the Doxygen blocks moved verbatim. Delete both from `CommandManager.{h,cpp}` and touch
      nothing else in them — in particular leave `#include <vector>` alone. Migrate the three call
      sites. Add `CommandLine.cpp` to **both** `bbloc` and `bbloc_tests` in `CMakeLists.txt`. Update
      `docs/class_diagram.md` in this commit. **No tests here**: the diff must be reviewable as the
      single question "is this a no-op?".
- [ ] **2 — Pin the parser.** New `tests/CommandLineTests.cpp`, added to `bbloc_tests`.
      `tokenize`: empty input; whitespace only; consecutive spaces; leading and trailing spaces; a
      quoted argument with spaces; an **empty quoted string** (`open ""` → two tokens, the second
      empty); a **lone quote** (one empty token); an unterminated quote taking the rest of the line;
      a **quote mid-word** (`ab"cd ef"` → `ab"cd`, `ef"` — the quote is not special there); a
      **closing quote followed by text** (`"ab"cd` → two tokens); a tab **not** being a delimiter;
      the **views aliasing the input** (`tokens[n].data()` inside the caller's buffer), which is the
      contract `AutoCompleteCommand` depends on; and the out-vector being **cleared** first, since
      `ApplicationWindow` reuses one scratch vector.
      `split`: empty input; no delimiter; a normal `Ctrl+Shift`; **consecutive delimiters** yielding
      no empty part; leading and trailing delimiters; **delimiter only** (`+` → empty, so
      `bind + <key> <cmd>` binds with no modifier and reports nothing); a string that is all
      delimiter; aliasing. Then the differential sweep described above.
- [ ] **3 — Describe it in CLAUDE.md.** Add `CommandLineTests.cpp` to the coverage list and
      `CommandLine` to the "links … only" sentence, and **rewrite the sentence naming
      `CommandManager::tokenize` as what the cheapness rule costs** — it will no longer be true.
      What replaces it: every `Command<CursorContext>` subclass is unreachable by construction,
      because `run` is virtual on the payload, so the header needs `CursorContext` complete, and no
      forward declaration is allowed to break that.
- [ ] **4 — Narrow `PromptState` to `CVarRegistry`.** `PromptState.h` swaps `CommandManager.h` for
      `base/CVarRegistry.h`, and the member and constructor parameter follow. `PromptState.cpp`
      changes at the init list and at the one `registerCvar` call. `ApplicationWindow` needs no
      change (derived-to-base reference conversion, and the manager is declared first). Check
      whether any TU was getting `CommandManager.h` transitively through `PromptState.h` — the fix
      if so is a direct include there, never a forward declaration. Diagram in the same commit.
- [ ] **5 — Pin the history and completion rings.** New `tests/PromptStateTests.cpp` with a local
      `CVarRegistry` fake that keeps the callback (so the `dim_max_history` path is reachable); add
      `PromptState.cpp` and `ViewState.cpp` to `bbloc_tests`. Cases: the first `nextHistory()` *and*
      the first `previousHistory()` both returning the most recent entry; wrapping in both
      directions; an empty history answering without moving the index; `addHistory` resetting the
      index; the overflow trim dropping the oldest; the `dim_max_history` callback clamping to
      `[8, 255]` and trimming from the front, asserted as both facts; `getCurrentCompletion()` on an
      empty list; `sortCompletions()` resetting the index; the completion ring wrapping both ways;
      `clearCompletions()`.
- [ ] **6 — Describe it, and close the plan.** CLAUDE.md gains `PromptStateTests.cpp` and the two
      new linked sources. Delete this file in the commit.

## Files

New: `src/core/base/CommandLine.{h,cpp}`, `tests/CommandLineTests.cpp`, `tests/PromptStateTests.cpp`.
Changed: `src/core/CommandManager.{h,cpp}`, `src/ApplicationWindow.cpp`,
`src/command/AutoCompleteCommand.cpp`, `src/command/BindCommand.cpp`, `src/prompt/PromptState.{h,cpp}`,
`CMakeLists.txt`, `docs/class_diagram.md` (tasks 1 and 4, in those commits), `CLAUDE.md`.

## If a test fails

Written against shipping code, so a red case is a **finding, not a fixture to adjust**. Report it
and stop. Every case must also be shown to bite: mutate the code under test, confirm red, revert —
and make the mutation script fail loudly on a build error, or a mutant that does not compile
re-runs the stale binary and reads as a survivor.

## Verification

Task 1 is proven by identity, not by tests (there are none yet at that point):

```bash
git show HEAD:src/core/CommandManager.cpp | sed -n '133,193p' | sed 's/CommandManager::/CommandLine::/' \
  | diff - <(sed -n '<new range>p' src/core/base/CommandLine.cpp)
```

must print nothing, and `git show --color-moved=zebra` shows the reviewer the same fact. Then both
targets build, the suite stays green, and the app is launched from the repository root — `autoexec`
and a theme script run through the parser before the first frame, so wrong colours or a dead
binding is the end-to-end assertion. By hand: a bound chord fires; a quoted path opens as one
argument; `bind Ctrl+Alt q quit` binds (that is `split`); Tab completion still reconstructs the
left-hand prefix of a partially typed argument (that is the aliasing contract, and the smoke test is
the only thing that catches it end to end).

`CMakeLists.txt` changes, so **`nx/` must be reconfigured from scratch and built** — a missing
`CommandLine.cpp` there fails at link time on the Switch and nowhere else. And the desktop build has
to be **reconfigured before committing**: `misc/pre-commit` skips staged sources absent from
`compile_commands.json`, so a new `.cpp` ships unlinted otherwise.

Time `bbloc_tests` from clean before and after tasks 2 and 5. `CommandLine.cpp` adds nothing;
`PromptState.cpp` plus `ViewState.cpp` add about 220 lines and no library. If the target stops
building in seconds, something was mis-added and the premise is gone.

## Out of scope

Recorded so they are not re-derived:

- **`CommandManager::getPathCompletions`.** Not pure — its output is the state of a directory.
  Testing it means either writing to the filesystem from a target whose whole value is being cheap
  and always run, or asserting against the repo tree, which goes red the day `romfs/` gains a file.
  Moving it buys nothing either: `CommandManager.cpp` keeps `CursorContext.h` for `run()` regardless.
- **`SearchCommand::LineScanner`** — genuinely valuable (`isSelfOverlapping` gates whether backward
  stepping can trust the match enumeration, and is exactly the kind of predicate that is wrong for
  one input class forever), but it is a nested private class and promoting it substantially rewrites
  one header. Its own plan. Note that `Cursor` is already linked, so once it is out, the whole scan
  can be pinned against a real buffer.
- **`OskLayout` and `BindCommand::normalizeModifiers`** — out of *this* plan, but approved and next.
  Both are pure table logic whose failure is maximally silent (one key typing the wrong character on
  one layout; one binding silently unreachable). `OskLayout.cpp` calls no SDL function at all — it
  uses `SDL_Scancode` constants — so it needs **SDL headers without the SDL library**, one
  `target_include_directories` line. Romain approved that on 2026-08-09: `bbloc_tests` may compile
  against SDL headers as long as it links no SDL. The rule in CLAUDE.md moves with it. Their own
  plan, so this one stays reviewable.
- **`InputRepeater`** — cannot be a pure move. `arm`/`rearm`/`isDue` call `SDL_GetTicks64()`
  directly, so testing means injecting a clock: an API change for five lines of arithmetic.
- **`Editor::computeScrollbarMetrics`** — pure and already static, but a wrong scrollbar thumb is
  the most *visible* bug in the app, not a silent one, and extracting it drags `ScrollbarMetrics` out
  of `Editor.h` and touches the render and mouse paths.
- **`ParserCatalog::findModeByExtension`** — a map lookup whose failure ("no highlighting") is
  noticed instantly, and splitting the extension table from the grammar descriptors would create two
  places to edit when adding a language, against what CLAUDE.md tells the next person to do.
