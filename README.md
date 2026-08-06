# bbloc

bbloc is a minimalist text editor developed in C++ using SDL2, OpenGL, glad, Freetype, utfcpp, and tree-sitter, targeting Linux and the Nintendo Switch (homebrew). It features a command-driven interface, syntax highlighting, and a layered UI with real-time configuration via CVars, controllable by keyboard, mouse, touch, or game controller.

## Table of Contents

- [Features](#features)
- [Concept](#concept)
- [Architecture](#architecture)
- [UI Components](#ui-components)
- [Key Bindings](#key-bindings)
- [Commands](#commands)
- [CVars](#cvars)
- [Dependencies](#dependencies)
- [Building](#building)
- [Screenshots](#screenshots)

## Features

- **Syntax Highlighting**: Built on tree-sitter for C++, JSON, INI, YAML, and TOML syntax highlighting (more to come)
- **Command-Driven Interface**: Execute operations via text commands with auto-completion
- **Multiple Buffers**: Open several files at once and switch between them with the `buffer` command; opening an already-open file switches to its buffer instead of loading a second copy
- **Real-Time Configuration**: Change colors, dimensions, and settings at runtime
- **Customizable Key Bindings**: Rebind any key combination to commands
- **Mouse & Touch Support**: Click to place the caret, drag to select text, wheel scrolling, and draggable scrollbar thumbs; one finger acts as the mouse, two fingers scroll
- **Game Controller Support**: Every pad button and axis can be bound to commands like a key, with the shoulders acting as modifier layers
- **On-Screen Keyboard**: A built-in OSK with sticky modifiers, hold auto-repeat, and international layouts — usable by touch, mouse, or pad

## Concept

The application window is divided into three distinct areas (plus an optional on-screen keyboard strip):

### Top Bar (InfoBar)
Displays information about the active text buffer:
- Cursor position (line, column)
- Current syntax highlight mode
- Font size settings
- File information
- Buffer position (e.g. `[2/3]`) when several buffers are open
- Unsaved-changes marker (`*` after the file name) when the buffer is modified

### Center Area (Editor)
The main text editing area featuring:
- Syntax-highlighted text rendering
- Line numbers display
- Cursor tracking and movement
- Selection support
- Scrollable content
- Optional scrollbars (`cvar show_scrollbar true|false`), shown only when content overflows, with draggable thumbs

### Bottom Bar (Prompt)
An interactive command-line interface serving as both:
- Command input/output console
- Status bar for feedback messages
- Auto-completion interface for commands and files

### On-Screen Keyboard (Osk)
An optional keyboard strip at the bottom of the window (`osk show|hide|toggle`), designed for the Switch but available everywhere:
- Two key pages sharing one 5-row geometry; sticky Ctrl/Shift/Alt/AltGr keys latch on tap and hold on long-press
- Tapped keys synthesize regular key and text events, so shortcuts (e.g. sticky Ctrl + T) work exactly like a physical keyboard
- Hold auto-repeat on repeatable keys
- International layouts (`osk layout <name>`): qwerty, azerty, qwertz, uk, spanish, spanish_latin, italian, portuguese, russian
- Driven by touch, mouse, or pad (d-pad/A move and press a key cursor, B hands control back to the editor)

### Command System
Every action beyond basic text typing is implemented as a command. The prompt allows executing commands using text input. By default, the key combination `Ctrl+Shift+space` opens the prompt for command entry.

### Key Bindings
Actions can be mapped to keystrokes using the `bind` command. The editor includes comprehensive default key bindings covering navigation, editing, and system operations.

## Architecture

### Core Components

#### Command System
- **Command Pattern**: Template-based command implementation with `Command<TPayload>`
- **CommandManager**: Central registry managing command execution and auto-completion
- **CommandRunner Interface**: Abstract interface for command execution from different contexts
- **CommandFeedback**: Interactive prompts for commands needing more input — confirmations (e.g., "Overwrite? [y/n]") or a follow-up argument with Tab completion (e.g., a file path for `open`)

#### Configuration Variables (CVar)
- **Type-Safe Storage**: Support for int32_t, bool, float, and Color types
- **Runtime Modification**: Change configuration at runtime through the command prompt
- **Callback System**: Reactive updates when CVars change

#### Cursor Management
- **TextBuffer**: Interface for text storage operations
- **Cursor**: Manages caret location, insert/delete operations, and selection
- **PromptCursor**: Specialized cursor for command prompt input
- **BufferEdit**: Data structure returned after edits for syntax highlighting updates

#### Syntax Highlighting
- **Tree-sitter Integration**: Parse text for syntax patterns
- **Language Parsers**: Support for C++ (tree-sitter-cpp), JSON (tree-sitter-json), INI (tree-sitter-ini), YAML (tree-sitter-yaml), and TOML (tree-sitter-toml)
- **Dynamic Switching**: Change highlight modes based on file extensions
- **Real-time Updates**: Re-parse incrementaly changed text segments

#### Theme System
- **Font Rendering**: FreeType-based glyph atlas generation and caching
- **Color Configuration**: Runtime-modifiable UI and syntax colors
- **Dimension Settings**: Layout dimensions (padding, borders, tabs, scroll amounts)
- **Texture Atlas**: Layered texture storage with naive packing algorithm

#### Renderer
- **OpenGL Integration**: Dynamic function loading via glad
- **Batched Quad Rendering**: Efficient vertex buffer rendering with QuadBuffer
- **Shader System**: Custom QuadProgram for textured quad rendering
- **Orthogonal Projection**: Coordinate system for UI layout

#### Views
- **View Pattern**: Base class with common rendering and input handling
- **View Subclasses**: InfoBar, Editor, Prompt, and Osk implementations
- **Focus Management**: Handles focus switching between Editor and Prompt; a third Osk focus redirects only the pad to the on-screen keyboard
- **State Management**: ViewState hierarchy for view-specific state

#### Input Routing
- **KeyboardInput**: Dispatches keys to the focused view, falling back to the key bindings; Ctrl/Alt chords skip the view and go straight to the bindings
- **PointerInput**: Mouse buttons, motion, wheel, and touch fingers — one finger emulates the left button, two fingers scroll
- **ControllerInput**: Pad buttons and axes encoded as `pad:*` pseudo-keycodes, dispatched through the same binding table; the shoulders act as the L/R modifier layers
- **InputRepeater**: Deadline-based hold auto-repeat, shared by the pad and the OSK

#### Application Window
- **Lifecycle Management**: SDL window creation and OpenGL context handling
- **Event Loop**: SDL event processing and input routing
- **Rendering Pipeline**: Coordination of view rendering and syntax parsing
- **Command Integration**: Connects CommandRunner with UI and state

### Data Flow

1. **User Input**: SDL events routed through the input classes (keyboard, pointer, controller) to the focused view or the key bindings
2. **Command Processing**: CommandManager executes registered commands
3. **State Updates**: CVars and theme attributes reflect changes
4. **Rendering**: Views render based on current state and syntax highlighting
5. **Feedback**: Prompts and status messages communicate to user

### Design Patterns

- **Command Pattern**: For operations and actions
- **Singleton-like**: Global Theme and CommandManager instances
- **Observer Pattern**: CVars and callbacks for reactive updates
- **Factory Pattern**: Lazy glyph generation and command instantiation
- **Template Method**: TypedCVar for type-safe configuration
- **Registry Pattern**: Centralized storage for commands and CVars

## UI Components

### View Hierarchy

```
ApplicationWindow
├── InfoBar (InfoBarState)
├── Editor (EditorState)
├── Prompt (PromptState)
└── Osk (OskState)
```

### View Layout

- **InfoBar**: Top bar, occupies top portion of window
- **Prompt**: Bottom bar, occupies bottom portion of window
- **Editor**: Central area, fills remaining space between bars
- **Osk**: Optional strip below the prompt while visible (`dim_osk_height` percent of the window); the editor and prompt shift up to make room

### Focus Management

- Default focus on Editor
- `activate_prompt` command switches focus to Prompt
- Return/Enter in prompt switches back to Editor
- Escape in prompt switches back to Editor
- Commands from prompt reset focus to Editor
- The Osk focus is pad-only: acquired on the first d-pad/A press while the OSK is visible, released with B or `osk hide` — the physical keyboard keeps editing the buffer throughout

## Key Bindings

### Navigation
| Keys | Command | Description |
|------|---------|-------------|
| Up | move up | Move cursor up one line |
| Shift+Up | move up true | Move up with text selection |
| Down | move down | Move cursor down one line |
| Shift+Down | move down true | Move down with text selection |
| Left | move left | Move cursor left one character |
| Shift+Left | move left true | Move left with selection |
| Right | move right | Move cursor right one character |
| Shift+Right | move right true | Move right with selection |
| Home | move bol | Move to beginning of line |
| Shift+Home | move bol true | Select to beginning of line |
| End | move eol | Move to end of line |
| Shift+End | move eol true | Select to end of line |
| PageUp | move page_up | Move up one page |
| Shift+PageUp | move page_up true | Select page up |
| PageDown | move page_down | Move down one page |
| Shift+PageDown | move page_down true | Select page down |

### Editing
| Keys | Command | Description |
|------|---------|-------------|
| Ctrl+C | copy | Copy selection to clipboard |
| Ctrl+X | cut | Cut selection to clipboard |
| Ctrl+V | paste | Paste from clipboard |
| Ctrl+Z | undo | Undo the last text modification |
| Ctrl+Shift+Z | redo | Redo the last undone modification |
| Tab | auto_complete forward | Auto-complete input forward |
| Shift+Tab | auto_complete backward | Auto-complete input backward |
| Ctrl+F | search | Prompt for a term and select its first match |
| F3 | find_next | Jump to the next match of the search term |
| Shift+F3 | find_prev | Jump to the previous match of the search term |
| Ctrl+G | goto_line | Prompt for a line number and jump to it |

### System
| Keys | Command | Description |
|------|---------|-------------|
| Ctrl+T | cvar inf_draw_time | Display max render time |
| Ctrl+Shift+T | reset_draw_time | Reset render time to 0 |
| Ctrl+Y | cvar inf_command_time | Display max command time |
| Ctrl+Shift+Y | reset_command_time | Reset command time to 0 |
| Ctrl+Keypad+ | set_font_size + | Increase font size by 1 |
| Ctrl+Keypad- | set_font_size - | Decrease font size by 1 |
| Ctrl+O | open | Prompt for a path and open the file |
| Ctrl+Tab | buffer next | Switch to the next open buffer |
| Ctrl+Shift+Tab | buffer prev | Switch to the previous open buffer |
| Ctrl+W | buffer close | Close the current buffer (asks confirmation on unsaved changes) |
| Ctrl+Shift+S | save | Save current buffer to file (prompts for a name when the buffer has none) |
| Ctrl+Shift+Space | activate_prompt | Open command prompt |
| Ctrl+Shift+Q | quit | Quit application (asks confirmation when any buffer has unsaved changes) |
| Ctrl+Shift+L | exec romfs/light_theme | Load the light theme (temporary binding) |
| Ctrl+Shift+D | exec romfs/dark_theme | Load the dark theme (temporary binding) |

### Mouse
| Input | Action |
|-------|--------|
| Left click (text area) | Place the caret at the clicked character and focus the editor |
| Left drag (text area) | Select from the pressed position to the pointer |
| Left drag (scrollbar thumb) | Scroll the view, vertically or horizontally |
| Left click (scrollbar track) | Jump the scroll by one page toward the click |
| Wheel | Scroll vertically (horizontal wheel scrolls horizontally) |
| Left click/drag (OSK) | Tap a key; holding it auto-repeats |

### Touch
| Input | Action |
|-------|--------|
| One finger | Acts as the left mouse button: tap to place the caret, drag to select, tap OSK keys |
| Two fingers | Scroll the editor; the gesture ends only when every finger lifts |

### Game Controller

The shoulders are modifier layers: no modifier acts, L selects, R jumps, L+R jumps while selecting. Buttons and axes are bound in `romfs/autoexec` as `pad:*` pseudo-keys, so everything below can be rebound.

| Input | No modifier | L held | R held |
|-------|-------------|--------|--------|
| D-pad / left stick | Move the cursor | Move with selection | Jump to line/file boundaries (bol/eol/bof/eof) |
| ZL / ZR, right stick up/down | Page up / page down | Page with selection | - |
| A | Confirm the prompt | copy | paste |
| B | Cancel the prompt | undo | redo |
| X | Open the prompt | save | cut |
| Y | Cycle completions forward | Cycle backward | search |
| Right stick left/right | find_prev / find_next | - | - |
| Back | Next buffer | Previous buffer | - |
| Start | quit | - | - |
| Left stick click | Toggle the on-screen keyboard | - | - |

While the OSK is visible, the first d-pad/A press hands the pad to it: the d-pad moves a key cursor, A presses the key, and B returns the pad to the editor.

## Commands

### File Operations
| Command | Arguments | Description |
|---------|-----------|-------------|
| `open <filename>` | filename | Open file in editor, sets highlight mode by extension (prompts for the path when bound to a key); opens a new buffer unless the active one is untouched, and switches to the existing buffer when the file is already open |
| `buffer <next\|prev\|close [-f]\|name>` | action or buffer name | Cycle through the open buffers, close the active one (asks confirmation on unsaved changes, `-f` skips it), or switch to a buffer by name |
| `save <filename> -f` | filename, -f | Save buffer with optional overwrite flag (prompts for a name when the buffer has none and it is bound to a key) |
| `quit [-f]` | -f | Exit application (asks confirmation when any buffer has unsaved changes, `-f` skips it) |
| `exec <filename>` | filename | Execute commands from file line by line |

### Configuration
| Command | Arguments | Description |
|---------|-----------|-------------|
| `cvar <name> [value1] [value2] ...` | cvar name, values | Print/set CVar value |
| `reset_draw_time` | - | Reset render time CVar to 0 |
| `reset_command_time` | - | Reset command time CVar to 0 |
| `set_font_size <size>` | size, +, - | Set font size (absolute or relative) |
| `set_hl_mode <mode>` | mode | Set syntax highlight mode (cpp, json, etc.) |
| `bind <modifiers>+<modifiers> <key> "<command>"` | modifiers, key, command | Bind key to command |

### Cursor
| Command | Arguments | Description |
|---------|-----------|-------------|
| `move <direction> <expand_selection>` | direction, bool | Move cursor (up/down/left/right/bol/eol/bof/eof/page_up/page_down) |
| `goto_line <line>` | line | Jump to a 1-based line (clamped to range) |

### Search & Replace
| Command | Arguments | Description |
|---------|-----------|-------------|
| `search <term>` | term | Store the term and select its first match, reporting the match count (prompts for the term when bound to a key) |
| `find_next` | - | Select the next match of the stored term (wraps to the top) |
| `find_prev` | - | Select the previous match of the stored term (wraps to the bottom) |
| `replace <from> <to>` | from, to | Replace the next occurrence of `from` with `to` |
| `replace_all <from> <to>` | from, to | Replace every occurrence of `from` with `to` |

### System
| Command | Arguments | Description |
|---------|-----------|-------------|
| `activate_prompt` | - | Open command prompt (intended for key bindings, hidden from prompt completion) |
| `copy` | - | Copy selection to clipboard |
| `paste` | - | Paste from clipboard |
| `cut` | - | Cut selection to clipboard |
| `undo` | - | Undo the last text modification |
| `redo` | - | Redo the last undone modification |
| `auto_complete <direction>` | direction | Provide command/argument completion (intended for key bindings, hidden from prompt completion) |
| `prompt <confirm\|cancel>` | action | Confirm or cancel the prompt, like Return/Escape (intended for pad bindings, hidden from prompt completion) |
| `osk <show\|hide\|toggle>` | action | Show, hide, or toggle the on-screen keyboard |
| `osk layout <name>` | layout name | Select the OSK layout (qwerty, azerty, qwertz, uk, spanish, spanish_latin, italian, portuguese, russian) |

### Auto-Completion
- Commands: Type command name and press Tab
- Arguments: Type command with incomplete argument and press Tab
- File paths: Type `open` or `save` with filename and press Tab

## CVars

### System
| Variable | Type | Description |
|----------|------|-------------|
| `tab_to_space` | bool | Insert tab or spaces up to `dim_tab_to_space` |
| `search_case_sensitive` | bool | Whether search and replace match case |
| `show_scrollbar` | bool | Show editor scrollbars when content overflows |

### Read-Only Metrics
| Variable | Type | Description |
|----------|------|-------------|
| `inf_draw_time` | float | Maximum render time in seconds (reset-able) |
| `inf_command_time` | float | Maximum command processing time (reset-able) |

### Theme Colors
| Variable | Type | Description |
|----------|------|-------------|
| `col_margin_background` | Color | Margin background color |
| `col_info_bar_background` | Color | Info bar background color |
| `col_editor_background` | Color | Editor background color |
| `col_prompt_background` | Color | Prompt background color |
| `col_current_line_background` | Color | Current line highlight color |
| `col_selected_text_background` | Color | Selected text background |
| `col_line_number` | Color | Line number color |
| `col_info_bar_text` | Color | Info bar text color |
| `col_prompt_text` | Color | Prompt text color |
| `col_prompt_input_text` | Color | Prompt input text color |
| `col_border` | Color | Border color |
| `col_cursor_indicator` | Color | Cursor indicator color |
| `col_scrollbar` | Color | Scrollbar track color |
| `col_scrollbar_thumb` | Color | Scrollbar thumb color |
| `col_osk_background` | Color | On-screen keyboard strip background |
| `col_osk_key_background` | Color | OSK key background color |
| `col_osk_key_text` | Color | OSK key label color |
| `col_osk_key_cursor` | Color | Highlight behind the pad-selected OSK key |
| `col_osk_key_pressed` | Color | OSK pressed/latched key color |

### Highlight Colors
| Variable | Type | Description |
|----------|------|-------------|
| `hl_text` | Color | Plain text color |
| `hl_comment` | Color | Comment syntax color |
| `hl_string` | Color | String syntax color |
| `hl_preprocessor` | Color | Preprocessor syntax color |
| `hl_number` | Color | Number syntax color |
| `hl_keyword` | Color | Keyword syntax color |
| `hl_statement` | Color | Statement syntax color |
| `hl_type` | Color | Type syntax color |
| `hl_constant` | Color | Constant syntax color |
| `hl_function` | Color | Function and method syntax color |
| `hl_variable` | Color | Variable and field syntax color |

### Dimensions
| Variable | Type | Description |
|----------|------|-------------|
| `dim_padding_width` | int | Padding width in pixels |
| `dim_indicator_width` | int | Indicator width in pixels |
| `dim_border_size` | int | Border size in pixels |
| `dim_tab_to_space` | int | Spaces per tab character |
| `dim_page_up_down` | int | Lines per page scroll |
| `dim_scrollbar_width` | int | Scrollbar thickness in pixels |
| `dim_font_size` | int | Font size in pixels (runtime) |
| `dim_max_history` | int | Prompt command-history size |
| `dim_max_undo` | int | Undo/redo history depth (1-4096) |
| `dim_osk_height` | int | On-screen keyboard height, in percent of the window height |
| `dim_osk_key_gap` | int | Gap between on-screen keyboard keys, in pixels |

### Usage
- Get value: `cvar <name>`
- Set value: `cvar <name> <value>`
- Example: `cvar col_editor_background 250 250 250 255`

## Dependencies

### Core Libraries
- **SDL2**: Input handling and window management
- **OpenGL 4.3+**: Graphics rendering
- **glad**: Dynamic OpenGL function loader
- **Freetype**: Font glyph rendering
- **utfcpp**: UTF-8/UTF-16 string conversion

### Syntax Highlighting
- **tree-sitter**: General parser framework
- **tree-sitter-cpp**: C/C++ language parser
- **tree-sitter-json**: JSON language parser
- **tree-sitter-ini**: INI language parser
- **tree-sitter-yaml**: YAML language parser
- **tree-sitter-toml**: TOML language parser

### Fonts
- **JetBrains Mono**: Default font included in repository

## Building

### Linux (VCPKG + CLion)
1. Import project in CLion
2. Configure VCPKG as toolchain
3. Import necessary libraries from VCPKG
4. To run it, set working directory to `$ProjectFileDir$`
5. Compile and debug

### Nintendo Switch (Manual Build)
Requires:
- devkitpro
- devkitA64
- SDL2 with specific patch (`src/platform/SDL2-2.28.5.patch_usbkbd.diff`)
- Manual compilation of utfcpp, tree-sitter, and parsers

On Switch, the Joy-Cons drive the editor through the game controller bindings, text is typed on the built-in on-screen keyboard or the touchscreen, and a USB keyboard is partially supported. The light or dark theme is selected at startup from the console color set (system theme setting).

```bash
mkdir nx && cd nx
source $DEVKITPRO/switchvars.sh
cmake -G"Unix Makefiles" -DCMAKE_C_FLAGS="$CFLAGS $CPPFLAGS" -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake ..
make
```

## Screenshots

**Default Theme**
![Default theme](./misc/capture.png)

**Light Theme**
![Light theme](./misc/light_theme.png)

**Dark Theme**
![Dark theme](./misc/dark_theme.png)

## Technical Implementation Details

### Rendering Pipeline
- OpenGL 4.5 (4.3 on Nintendo Switch) Core profile with double-buffered rendering; the two backends are separate CMake-selected source sets (DSA on desktop, bind-based on Switch)
- Batched quad rendering via QuadBuffer: batches are staged CPU-side and uploaded per view, the GPU buffer starts at 8192 quads and grows on demand (no truncation)
- Custom QuadProgram shader for textured quad drawing
- Orthogonal projection matrix for UI coordinates
- Scissor test to confine rendering per view

### Texture Atlas
- FreeType-generated layered glyph atlas (256x256x256 pixels)
- Lazy glyph loading (generated on-demand)
- AtlasArray for tracking character positions and layers

### Performance Optimization
- Texture atlas caching for glyphs
- Batched rendering (single draw call per view)
- Delta time calculation via high-resolution performance counters
- Metrics tracking for render and command times

### UTF-8/UTF-16 Conversion
- utfcpp library for bidirectional string conversion
- Consistent use of UTF-16 internally for prompt system
- UTF-8 for file I/O operations

### State Management
- Single-threaded event loop architecture
- CursorContext for runtime cursor state, one per open buffer, managed by CursorContextManager (views render the active one)
- ViewState hierarchy for view-specific data
- FocusTarget for input routing
- CommandFeedback for interactive prompts

### Error Handling
- Optional error messages from command execution
- Type conversion validation in CVar operations
- Exception handling for initialization failures
- Graceful handling of malformed input

## Project Status

### Implemented
- Text editing with cursor management
- Syntax highlighting (C++, JSON, INI, YAML, TOML)
- Command-driven interface with auto-completion
- Real-time CVar configuration
- Customizable key bindings
- Tab handling (space expansion)
- Selection and clipboard operations
- Undo/redo (linear, snapshot-based, 64 entries deep)
- Multiple open buffers with per-buffer scroll, search, undo, and highlight state
- Dirty-flag tracking with close/quit confirmation on unsaved changes
- Mouse support: caret placement, drag selection, wheel scrolling, and scrollbar interactions
- Touch support: single-finger caret/selection/taps, two-finger scrolling
- Game controller support with rebindable buttons/axes and shoulder modifier layers
- On-screen keyboard with sticky modifiers, hold auto-repeat, and international layouts

### Known Limitations
- Undoing back to the last saved state still shows the buffer as modified

### Future Enhancements (no ordering)
- Additional language support

## Contributing

This is a hobby project. Contributions are not yet open.
If you use it on Nintendo Switch and see bugs, reporting them is welcome.

### Code Style

Code style is checked with clang-tidy ([.clang-tidy](./.clang-tidy)) on every commit through a pre-commit hook.
Activate the hook once after cloning:

```bash
git config core.hooksPath misc
```
