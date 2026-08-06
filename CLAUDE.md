# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

bbloc is a minimalist GPU-rendered text editor in C++20 (SDL2, OpenGL, glad, FreeType, utfcpp, tree-sitter). It targets Linux and Nintendo Switch (devkitPro). No test suite or lint config exists.

## Plans

Upcoming work is planned in numbered `docs/plan-*.md` files, executed in order. Delete each plan file in the commit that completes it.

## Building

One `main` branch builds both targets; the toolchain file chosen at configure time selects the target. Desktop dependencies come from vcpkg:

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build cmake-build-debug
```

Run from the repository root — the app loads `romfs/` (font, `autoexec`, themes) relative to the working directory:

```bash
./cmake-build-debug/bbloc [file-to-open]
```

The Nintendo Switch target uses the devkitPro toolchain (`NINTENDO_SWITCH` is defined by it) and configures in `nx/`, which must be reconfigured from scratch after CMakeLists changes:

```bash
mkdir nx && cd nx
source $DEVKITPRO/switchvars.sh
cmake -G"Unix Makefiles" -DCMAKE_C_FLAGS="$CFLAGS $CPPFLAGS" -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake ..
make
```

On Switch, assets are packaged into the NRO and read from the `romfs:/` device — code and scripts still write `romfs/` paths, resolved through the platform seam (below).

Source files are listed explicitly in `CMakeLists.txt` — every new `.cpp` must be added there by hand.

## Architecture

Single-threaded SDL event loop. `ApplicationWindow` (src/ApplicationWindow.cpp) is the composition root: it owns every subsystem, implements the `CommandRunner` interface, registers all commands and CVars in `create()`, then runs `exec romfs/autoexec` (which sets the default key bindings via `bind`).

**Everything is a command.** Any action beyond raw typing (open, save, move, undo, search…) is a `Command<CursorContext>` subclass in `src/command/`, registered by name in `ApplicationWindow::create()`. The prompt, key bindings, and `exec` scripts all go through `CommandManager` → `runCommand()`. Adding a feature usually means: new class in `src/command/`, register it in `ApplicationWindow.cpp`, add the `.cpp` to `CMakeLists.txt`, optionally bind a key in `romfs/autoexec`. Commands return `std::optional<std::u16string>` as a feedback/error message shown in the prompt, and can request interactive follow-up input through `CommandFeedback` (confirmations, argument prompts with Tab completion).

**CursorContext** (src/core/CursorContext.h) is the runtime state hub passed as the command payload and to views: it bundles the `Cursor` (which owns the `TextBuffer`), `HighLighter`, `Theme`, `PromptCursor`, focus target, scroll/search/column-stick state, `command_feedback`, and the `wants_redraw` flag that drives re-rendering. There is one per open file, managed by `CursorContextManager` (src/core/CursorContextManager.h); views and commands always receive the active one, and the `buffer` command switches between them.

**Views**: four `View` subclasses — `InfoBar` (top), `Editor` (center), `Prompt` (bottom command line), and `Osk` (an on-screen keyboard strip below the prompt while visible, toggled by the `osk` command) — each paired with a `ViewState` (`PromptState` and `OskState` extend it). Input focus switches via `FocusTarget` between Editor and Prompt; the third value, `FocusTarget::Osk`, redirects only the game pad to the OSK — physical keyboard and text input still reach the editor. All views share one `QuadBuffer`, passed as a `render()` parameter: each view stages one batch CPU-side (`beginBatch`/`insert`/`endBatch`) and draws it immediately; the GPU buffer starts at `DEFAULT_QUAD_CAPACITY` (8192) and regrows on demand, so batches are never truncated. `ApplicationWindow::mainLoop` calls `resetFrame()` once per redraw. Rendering is batched textured quads through `QuadProgram`, with glyphs from a FreeType-backed layered texture atlas (`AtlasArray`), scissor-clipped per view.

**Input** (src/input/): `ApplicationWindow::mainLoop` polls SDL and routes events to `KeyboardInput` (focused view first, then key bindings; Ctrl/Alt chords skip the view), `PointerInput` (mouse and touch — one finger drags like the left button, two fingers scroll), and `ControllerInput` (pad buttons/axes encoded as `pad:*` pseudo-keycodes dispatched through the same `bind` table, with the shoulders as the `L`/`R` modifier layers). `InputRepeater` drives hold auto-repeat for the pad and the OSK. The OSK itself synthesizes SDL key/text events for the tapped keys, so everything downstream stays unaware of it.

**Renderer backends**: `QuadBuffer`/`QuadProgram`/`QuadTexture` headers live in `src/core/renderer/`; their `.cpp` implementations exist twice, as CMake-selected source sets — `src/core/renderer/gl45/` (OpenGL 4.5 DSA, desktop) and `src/core/renderer/gl43/` (bind-based GL 4.3, Switch). Each set ships a `GlBackend.h` with the GL context version to request, supplied to `ApplicationWindow` through a per-set include path. Renderer backends are separate source sets; never merge them into one runtime-abstracted renderer. Keep GL calls in `QuadBuffer` confined to `create()`/`endBatch()`/`destroy()` so the backends stay small.

**Platform seam** (src/platform/Platform.h): the few desktop/Switch behavior differences live behind static methods of the `Platform` class, with one CMake-selected implementation — `src/platform/PlatformDesktop.cpp` or `src/platform/PlatformSwitch.cpp` (the only file allowed to include `<switch.h>`). The `src/platform/` directory also holds the Switch SDL2 patch. `assetPath()` resolves the `romfs/` prefix (`romfs:/` on Switch), used by `ApplicationWindow::create` and `ExecCommand` so scripts say `exec romfs/...` everywhere; `preferredColorScheme()` returns the console color set on Switch (applied as light/dark theme after autoexec) and `nullopt` on desktop.

**CVars** (src/core/cvar/): typed runtime config (int/bool/float/Color) registered with optional change callbacks; modified at runtime via the `cvar` command. Theme colors and dimensions are CVars (see `core/theme/ColorId.h`, `DimensionId.h`); `romfs/light_theme` and `romfs/dark_theme` are just command scripts setting them.

**Syntax highlighting** (src/core/highlighter/): tree-sitter with incremental re-parsing on `BufferEdit`. Languages (C++, JSON, INI, YAML, TOML) are declared in `ParserCatalog` with their queries in `core/highlighter/query/*_query.h` — adding a language means a new query header, a catalog entry, and linking the grammar in `CMakeLists.txt`.

**Text encoding**: UTF-16 (`std::u16string`) internally everywhere; UTF-8 only at file I/O boundaries, converted with utfcpp.

`docs/class_diagram.md` contains mermaid class diagrams of these subsystems. It must be updated before each commit so it reflects any classes or relationships the commit adds, removes, or changes.

## Conventions

- Every source file starts with the GPLv3 copyright header — include it in new files.
- Doxygen-style `@brief` comments on classes and methods; members prefixed `m_` (values), `p_` (pointers).
