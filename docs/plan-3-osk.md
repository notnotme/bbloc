# Plan 3 — Own on-screen keyboard (after plans 1 and 2)

> Delete this file in the commit that completes the plan.
> Direction decided 2026-08-06: build our own OSK; the Switch swkbd applet is rejected
> (no Ctrl/Tab/Esc/arrows → editor chords untypable; ~500-char inline buffer; applet-mode
> display unverified). This plan is a design sketch — refine open points when starting.

## Goal

A touch-driven (and pad-navigable) on-screen keyboard rendered by bbloc itself, portable
(desktop touchscreens included), that injects **synthesized SDL events** so the entire
existing pipeline — bindings, chords, prompt, editor text input — works unchanged.

## Design

- **Injection, not integration**: key taps push `SDL_KEYDOWN`/`SDL_KEYUP` (scancode +
  keycode + current sticky-modifier mask) and, for printable keys, `SDL_TEXTINPUT` via
  `SDL_PushEvent`. Nothing downstream knows the OSK exists; `Ctrl+S` from the OSK is the
  same event as from a USB keyboard. (Check: our `SDL_HINT_TOUCH_MOUSE_EVENTS` is unrelated;
  pushed events flow through the normal queue.)
- **View**: a new OSK view drawn as a bottom strip (own `ViewState`, scissored, quads +
  glyph atlas like the other views). While visible, the editor/prompt layout shrinks
  (`resizeWindow`-style relayout) so no content is hidden. Theme colors/dimensions via CVars.
  Roughly 5 rows; at 1280×720 keys land near 90×55 px — thumb-typable.
- **Layout data**: reuse the `KeyboardKeyEntry`-style tables from the SDL usbkbd patch
  (qwerty/azerty/qwertz/spanish/…) — per-scancode strings for normal/shift/altgr/caps states,
  used both as key labels and as the TEXTINPUT payload. Layout selected via
  `Platform` (Switch: `setsysGetKeyboardLayout`, same source the patched SDL uses) with a
  CVar override.
- **Modifiers**: sticky Ctrl/Shift/Alt keys (tap = latch for next key, tap again = release;
  long-press = hold). Shift also flips the displayed labels. This is what makes chords
  (`Ctrl+Shift+Space`…) possible — the OSK's core advantage over swkbd.
- **Special keys**: Esc, Tab, arrows, Backspace, Delete, Enter, and a symbols page toggle.
  Arrow/Backspace keys auto-repeat on hold — reuse plan 2's repeat machinery (generalize it
  from pad-only if needed).
- **Input routing**: taps on the OSK area are consumed by it (PointerInput checks the OSK
  view rect first, like the other `viewContains` routing); pad navigation (d-pad moves a key
  cursor, A presses) can reuse ControllerInput with an OSK-focus mode — while the OSK has pad
  focus, movement bindings are suspended. Details open.
- **Toggle**: an `osk` command (`show|hide|toggle`), hidden = false, bindable — e.g.
  `bind None pad:leftstick "osk toggle"` and/or a touch gesture; Switch autoexec can show it
  by default in handheld. Must be `allowedDuringPrompt` so text can be typed into the prompt.

## Open points (decide when starting)

- Exact geometry/pages (letters / symbols / function row), portrait of the 40% height budget.
- Key-cursor rendering for pad navigation (highlight quad vs border).
- Whether Editor should auto-scroll (`follow_indicator`) to keep the caret above the OSK.
- Desktop default: hidden, available via `osk toggle` for touchscreen users.

## Files (expected)

`src/osk/Osk.h/.cpp` (+ layout tables header), `src/command/OskCommand.h/.cpp`,
`src/ApplicationWindow.*` (view wiring, relayout), `romfs/autoexec` (bindings),
`CMakeLists.txt`, `docs/class_diagram.md`. GPLv3 headers on new files.

## Verification

Desktop: toggle via prompt command, click keys with the mouse/touchscreen, verify chords and
prompt input. Switch: on-device typing test in handheld (touch) and docked (pad navigation),
editing + saving a real file end-to-end.
