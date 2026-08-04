# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

bbloc is a minimalist GPU-rendered text editor in C++20 (SDL2, OpenGL, glad, FreeType, utfcpp, tree-sitter). It targets Linux and Nintendo Switch (devkitPro). No test suite or lint config exists.

## Building

Dependencies come from vcpkg; configuration requires the vcpkg toolchain file:

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build cmake-build-debug
```

Run from the repository root — the app loads `romfs/` (font, `autoexec`, themes) relative to the working directory:

```bash
./cmake-build-debug/bbloc [file-to-open]
```

Source files are listed explicitly in `CMakeLists.txt` — every new `.cpp` must be added there by hand.

The Nintendo Switch build uses the devkitPro toolchain (see README "Building"); on that platform assets come from `romfs:/` instead of `romfs/`.

## Architecture

Single-threaded SDL event loop. `ApplicationWindow` (src/ApplicationWindow.cpp) is the composition root: it owns every subsystem, implements the `CommandRunner` interface, registers all commands and CVars in `create()`, then runs `exec romfs/autoexec` (which sets the default key bindings via `bind`).

**Everything is a command.** Any action beyond raw typing (open, save, move, undo, search…) is a `Command<CursorContext>` subclass in `src/command/`, registered by name in `ApplicationWindow::create()`. The prompt, key bindings, and `exec` scripts all go through `CommandManager` → `runCommand()`. Adding a feature usually means: new class in `src/command/`, register it in `ApplicationWindow.cpp`, add the `.cpp` to `CMakeLists.txt`, optionally bind a key in `romfs/autoexec`. Commands return `std::optional<std::u16string>` as a feedback/error message shown in the prompt, and can request interactive follow-up input through `CommandFeedback` (confirmations, argument prompts with Tab completion).

**CursorContext** (src/core/CursorContext.h) is the runtime state hub passed as the command payload and to views: it bundles the `Cursor` (which owns the `TextBuffer`), `HighLighter`, `Theme`, `PromptCursor`, focus target, scroll/search/column-stick state, `command_feedback`, and the `wants_redraw` flag that drives re-rendering. There is one per open file, managed by `CursorContextManager` (src/core/CursorContextManager.h); views and commands always receive the active one, and the `buffer` command switches between them.

**Views**: three `View` subclasses — `InfoBar` (top), `Editor` (center), `Prompt` (bottom command line) — each paired with a `ViewState` (`PromptState` extends it). Input focus switches between Editor and Prompt via `FocusTarget`. All views share one `QuadBuffer`, passed as a `render()` parameter: each view stages one batch CPU-side (`beginBatch`/`insert`/`endBatch`) and draws it immediately; the GPU buffer starts at `DEFAULT_QUAD_CAPACITY` (8192) and regrows on demand, so batches are never truncated. `ApplicationWindow::mainLoop` calls `resetFrame()` once per redraw. Rendering is batched textured quads through `QuadProgram`, with glyphs from a FreeType-backed layered texture atlas (`AtlasArray`), scissor-clipped per view.

**Renderer portability**: main uses OpenGL DSA calls; the `nintendo_switch` branch (one commit rebased onto main) carries bind-based GL 4.3 equivalents plus platform packaging. Keep GL calls in `QuadBuffer` confined to `create()`/`endBatch()`/`destroy()` so the Switch-side translation stays small, and never unify the two renderers.

**CVars** (src/core/cvar/): typed runtime config (int/bool/float/Color) registered with optional change callbacks; modified at runtime via the `cvar` command. Theme colors and dimensions are CVars (see `core/theme/ColorId.h`, `DimensionId.h`); `romfs/light_theme` and `romfs/dark_theme` are just command scripts setting them.

**Syntax highlighting** (src/core/highlighter/): tree-sitter with incremental re-parsing on `BufferEdit`. Languages (C++, JSON, INI) are declared in `ParserCatalog` with their queries in `core/highlighter/query/*_query.h` — adding a language means a new query header, a catalog entry, and linking the grammar in `CMakeLists.txt`.

**Text encoding**: UTF-16 (`std::u16string`) internally everywhere; UTF-8 only at file I/O boundaries, converted with utfcpp.

`docs/class_diagram.md` contains mermaid class diagrams of these subsystems. It must be updated before each commit so it reflects any classes or relationships the commit adds, removes, or changes.

## Conventions

- Every source file starts with the GPLv3 copyright header — include it in new files.
- Doxygen-style `@brief` comments on classes and methods; members prefixed `m_` (values), `p_` (pointers).
