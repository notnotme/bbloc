# Plan 1 — Unify the branches, extract SDL event handling

> Delete this file in the commit that completes the plan.

## Goal

Retire the `nintendo_switch` branch: one `main` branch that builds both the desktop target
(vcpkg toolchain) and the Switch target (devkitPro toolchain), selected at configure time.
Then split `ApplicationWindow::mainLoop`'s giant event switch into per-domain input classes,
making room for controller support (plan 2) and the OSK (plan 3).

Unify the *branches*, not the renderers: the GL 4.5 DSA and GL 4.3 bind-based renderer
implementations stay separate source sets, selected by CMake. The renderer seam should stay
tight because a native deko3d backend may replace the GL 4.3 one eventually (see Future).

## Current branch delta (what must be absorbed into main)

The switch commit (`git show nintendo_switch`) touches:
- `src/core/renderer/QuadBuffer.cpp` / `QuadProgram.cpp` / `QuadTexture.cpp` — bind-based GL 4.3 variants (headers unchanged).
- `src/ApplicationWindow.cpp` — `#include <switch.h>`, `SDL_INIT_GAMECONTROLLER`, GL 4.3 context request, `romfs:/` asset path, console color-set theme block (`setsysGetColorSetId` after autoexec), gamepad open(0)/START-quits/close.
- `CMakeLists.txt` — pkg_check_modules (portlibs) instead of find_package for freetype/glad, `nx_generate_nacp` + `nx_create_nro(... ROMFS romfs ICON misc/icon.jpg)`.
- `romfs/autoexec` — theme bind lines use `exec romfs:/...` instead of `romfs/...`.
- `SDL2-2.28.5.patch_usbkbd.diff`, README building section — additive, keep as-is.
- `.gitattributes` — `merge=ours` guards protecting the switch renderer files across branch
  rebases/merges. **Do not carry it over; delete it** — with a single branch there is nothing
  to protect against.

## Step A — dual-target build + platform seam (absorbs the switch branch)

1. **Renderer source split**: move the three DSA `.cpp` into `src/core/renderer/gl45/`, add the
   bind-based variants from the switch branch as `src/core/renderer/gl43/`. Headers stay in
   `src/core/renderer/`. CMake selects one set. The GL context version request (4.5 vs 4.3)
   becomes a constant supplied per-backend (small header in each set, or the platform header).
2. **Platform seam**: new `src/core/Platform.h` with two impls selected by CMake
   (`PlatformDesktop.cpp` / `PlatformSwitch.cpp`, the latter being the only file including
   `<switch.h>`):
   - `assetPath(std::string_view)` → resolves the `romfs/` prefix (`romfs/` on desktop,
     `romfs:/` on Switch). Used by `ApplicationWindow::create` (theme + autoexec path) and by
     `ExecCommand` so scripts can say `exec romfs/light_theme` on both platforms → the two
     autoexec variants collapse into one.
   - `preferredColorScheme()` → optional light/dark from `setsysGetColorSetId` on Switch,
     `nullopt` on desktop (SDL system-theme query is a possible future nicety).
3. **CMakeLists**: single file; `if(NINTENDO_SWITCH)` (defined by the devkitPro toolchain)
   chooses portlibs pkg-config + gl43 + PlatformSwitch + nacp/nro packaging, else vcpkg
   find_package + gl45 + PlatformDesktop. `SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER` becomes
   unconditional (harmless on desktop, needed by plan 2 anyway).
4. **Temporary gamepad code**: keep the switch commit's `SDL_GameControllerOpen(0)` +
   START-quits + close on main unguarded, marked `// temporary until controller bindings
   (plan 2)` — handheld users need a quit path until then.
5. Update `.gitignore`/build dirs as needed (`nx/` stays the Switch configure dir; it must be
   reconfigured from scratch since CMakeLists changes).
6. Verify: desktop build (`cmake-build-debug`), fresh Switch configure + build, on-device
   check (theme follows console color set, touch, USB keyboard, START quits).
7. **Delete the `nintendo_switch` branch** (and its remote). Update `CLAUDE.md`: single-branch
   policy, both configure commands, renderer-set layout, platform seam — replacing the old
   branch/rebase text (including "never unify the two renderers" → rephrase as "renderer
   backends are separate source sets; never merge them into one runtime-abstracted renderer").

## Step B — extract event handling (pure refactor, after A)

1. New `src/input/` classes, each GPLv3-headered, Doxygen-commented, added to CMakeLists:
   - `KeyboardInput` — `SDL_KEYDOWN` (chord detection, focused-view dispatch, binding fallback)
     and `SDL_TEXTINPUT` (chord blocking, focused-view routing).
   - `PointerInput` — the four mouse cases + three finger cases; owns the `MouseTarget` and
     `TouchMode` enums/state and the `viewContains` helper (all move out of ApplicationWindow).
2. Handlers are constructed once with refs to what they need (views + states, context manager,
   `BindCommand`, theme) plus a dispatch hook for the timed bound-command execution
   (`getBinding` → perf-timed `runCommand` → `inf_command_time`), which stays owned by
   ApplicationWindow — factor it as `runBoundCommand(keycode, modifiers)` since plan 2 reuses
   it for pad inputs. Exact wiring (interface vs. `ApplicationWindow&`) decided at
   implementation; keep it dumb.
3. `mainLoop` keeps: `SDL_QUIT`, window events, temporary gamepad case, loop orchestration,
   redraw; everything else delegates to the handlers. Behavior must be byte-identical.
4. Update `docs/class_diagram.md` (new input classes, §8 prose) — required before commit.
5. Verify: desktop + Switch builds; user re-checks keyboard/touch on device.

## Future (not this plan)

- **deko3d renderer**: a possible native backend replacing gl43. It would own not just the
  quad pipeline but context creation and present (SDL_GL goes away on Switch), so when it
  happens the renderer boundary must widen to cover init/swap. Until then, keep GL calls
  confined to `create()`/`endBatch()`/`destroy()` as today, and don't let SDL_GL specifics
  leak into new code outside ApplicationWindow.

## Verification summary

Both targets build from one branch; desktop behavior unchanged (user checks); Switch on-device
checklist: dark/light theme at startup, touch (tap/drag/two-finger scroll), USB keyboard with
layout, START quits. `git branch -d nintendo_switch` only after the on-device check passes.
