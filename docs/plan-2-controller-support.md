# Plan 2 — Game-controller support (after plan 1)

> Delete this file in the commit that completes the plan.
> Design reviewed and approved 2026-08-05. Line numbers have shifted since; use the symbols.

## Goal

Controller buttons become bindable through the existing `bind` command; sticks/triggers act as
digital pseudo-buttons; L/R shoulders are binding modifiers; held inputs auto-repeat; a default
binding table ships in `romfs/autoexec`. Replaces the temporary START-quits code from plan 1.
The event-handling code lands in a new `src/input/ControllerInput` class (structure from
plan 1). Works identically with desktop gamepads.

## Decided design

- **Pad keycode namespace**: `SDL_Keycode` is Sint32 and all real keycodes are positive →
  negative values are collision-free. New `src/core/base/PadInput.h/.cpp`, namespace
  `pad_input`: `fromButton(b) = -(1 + b)`, `fromAxis(a, positive) = -(64 + 2a + positive)`,
  `KMOD_PAD_L = 0x0004`, `KMOD_PAD_R = 0x0008` (KMOD bits SDL leaves free),
  `keycodeFromName("pad:...")` (buttons via `SDL_GameControllerGetButtonFromString`,
  **rejecting leftshoulder/rightshoulder** — reserved as modifiers; stick axes need a `-`/`+`
  suffix, e.g. `pad:lefty-`; triggers `pad:lefttrigger`, suffix optional), and
  `forEachName(cb)` for autocomplete.
- **BindCommand**: add `L`/`R` to `MODIFIER_MAP`; try `pad_input::keycodeFromName` before
  `SDL_GetKeyFromName` in `run`; append pad names to the key-argument autocomplete.
  **Critical**: `normalizeModifiers` must pass through the two pad bits (today it masks to
  CTRL|SHIFT|ALT|GUI — forgetting this silently breaks every L/R combo).
- **`prompt` command**: extract `Prompt::onKeyDown`'s SDLK_RETURN body into
  `Prompt::confirm(context, viewState)` and SDLK_ESCAPE into `Prompt::cancel(...)`;
  `onKeyDown` delegates (no keyboard behavior change). New `src/command/PromptCommand`
  (ctor: `Prompt&, PromptState&`, model after ActivatePromptCommand): one arg
  `confirm|cancel`; no-op unless `getRunningState() == Running` (covers feedback prompts);
  set `payload.wants_redraw = true` **before** delegating (confirm may destroy the payload
  context via `buffer close` — never touch payload after). Register hidden = true,
  **allowedDuringPrompt = true**. Note `move` and `auto_complete` are already
  prompt-aware/allowed, so the pad drives history/cursor/completion with no extra work.
- **ControllerInput state machine**:
  - Constants: repeat delay 400 ms, interval 40 ms; axis hysteresis press ≥ 16000,
    release < 11000 (of 32767).
  - State: open `SDL_GameController*` handles (hotplug), live pad-modifier mask, repeat state
    (keycode = SDLK_UNKNOWN when disarmed, modifiers captured at press, `SDL_GetTicks64`
    deadline), per-(axis, direction) pressed flags.
  - `SDL_CONTROLLERDEVICEADDED` (`which` = device index) → open, store. `...REMOVED`
    (`which` = instance id) → close via `SDL_GameControllerFromInstanceID`, erase, and reset
    all pad state (a disconnected pad never sends its releases). SDL posts ADDED for pads
    already connected at init — no manual open.
  - `BUTTONDOWN`: shoulders → set modifier bit (an in-flight repeat keeps its captured mask);
    others → press: disarm repeat, `runBoundCommand(padKeycode, padModifiers)`, on success arm
    repeat (delay phase). `BUTTONUP`: shoulders → clear bit; others → disarm if it's the
    repeating keycode.
  - `AXISMOTION`: evaluate both directions with hysteresis; press-transition behaves like
    BUTTONDOWN of the pseudo-button, release-transition like BUTTONUP. Triggers only ever go
    positive.
  - No `which` filtering on button/axis events — all pads feed one state, like a keyboard.
- **Event-loop wait**: repeat armed → `SDL_WaitEventTimeout(nullptr, clamp(deadline - now,
  ≥1 ms))` instead of `SDL_WaitEvent`. Repeat tick runs **after** the poll loop (so fresh
  events can disarm/replace first): re-lookup via `runBoundCommand` each tick (no cached
  string_view — a repeated command may rebind), then re-arm at the fast interval. The command
  sets `wants_redraw`, so rendering follows naturally.
- Remove plan 1's temporary open(0)/START-quit/close code.

## Default bindings (append to `romfs/autoexec`)

Scheme: None = act, L = select, R = jump/secondary, L+R = jump+select. One `bind` per line.
- D-pad + left stick (`pad:dpup/dpdown/dpleft/dpright`, `pad:lefty±`, `pad:leftx±`):
  `move up/down/left/right`; same with `L` → `... true` (selection).
- `R` + d-pad: `move bol/eol/bof/eof`; `L+R` + d-pad: same with `true`.
- `pad:lefttrigger` / `pad:righttrigger` (ZL/ZR): `move page_up/page_down`; with `L`: `true`.
  `pad:righty-`/`pad:righty+`: `move page_up/page_down` too.
- `pad:a` `"prompt confirm"`, `pad:b` `"prompt cancel"` (no-ops while prompt idle),
  `pad:x` `activate_prompt`, `pad:y` `"auto_complete forward"` (`L` → backward).
- `L pad:a` copy, `R pad:a` paste, `L pad:b` undo, `R pad:b` redo, `L pad:x` save,
  `R pad:x` cut.
- `R pad:y` search, `pad:rightx+` find_next, `pad:rightx-` find_prev.
- `pad:back` `"buffer next"` (`L` → prev), `pad:start` quit.
- `pad:guide`/`pad:leftstick`/`pad:rightstick` left unbound. On Switch, SDL's a/b/x/y match
  the printed labels.

## Files

`src/core/base/PadInput.h/.cpp` (new), `src/input/ControllerInput.h/.cpp` (new),
`src/command/PromptCommand.h/.cpp` (new), `src/command/BindCommand.*`, `src/prompt/Prompt.*`,
`src/ApplicationWindow.*` (registration, wait-timeout, handler wiring), `romfs/autoexec`,
`CMakeLists.txt` (three new .cpp), `docs/class_diagram.md` (PromptCommand in §9, Prompt
confirm/cancel + controller prose in §8). GPLv3 headers on new files.

## Verification

Desktop build; user tests with a desktop pad (movement + repeat cadence, L/R layers, no axis
flutter at half-deflection, A/B in prompt and in a `quit`-with-dirty-buffer feedback question,
hotplug incl. disconnect while holding a direction, keyboard unchanged). Switch build; user
tests Joy-Con layers and START-quit-via-binding on device.
