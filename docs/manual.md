# bbloc manual

Reference of the default key bindings, the commands, and the configuration variables.
Inside the editor the `help` command shows `romfs/manual.txt`, an ASCII mirror of this
document — keep both in sync, along with the section list in the `help` completions.

## Keyboard

Default bindings, all set in `autoexec` (see Configuration for where it lives).

### Navigation

| Keys | Command | Description |
|------|---------|-------------|
| Up / Down / Left / Right | move up/down/left/right | Move the cursor |
| Shift+Arrow | move ... true | Move extending the selection |
| Home / End | move bol / move eol | Beginning / end of line |
| Shift+Home / Shift+End | move bol/eol true | Select to beginning / end of line |
| PageUp / PageDown | move page_up / move page_down | Move one page |
| Shift+PageUp / Shift+PageDown | move page_up/page_down true | Select one page |

### Editing

| Keys | Command | Description |
|------|---------|-------------|
| Ctrl+C | copy | Copy selection to clipboard |
| Ctrl+X | cut | Cut selection to clipboard |
| Ctrl+V | paste | Paste from clipboard |
| Ctrl+Z | undo | Undo the last text modification |
| Ctrl+Shift+Z | redo | Redo the last undone modification |
| Tab | auto_complete forward | Cycle completions forward (prompt) |
| Shift+Tab | auto_complete backward | Cycle completions backward (prompt) |
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
| Ctrl+Shift+S | save | Save the current buffer (prompts for a name when it has none) |
| Ctrl+Shift+Space | activate_prompt | Open the command prompt |
| Ctrl+Shift+Q | quit | Quit (asks confirmation when any buffer has unsaved changes) |
| Ctrl+Shift+L | exec romfs/light_theme | Load the light theme |
| Ctrl+Shift+D | exec romfs/dark_theme | Load the dark theme |

## Mouse and touch

| Input | Action |
|-------|--------|
| Left click (text area) | Place the caret at the clicked character and focus the editor |
| Left drag (text area) | Select from the pressed position to the pointer |
| Left drag (scrollbar thumb) | Scroll the view, vertically or horizontally |
| Left click (scrollbar track) | Jump the scroll by one page toward the click |
| Wheel | Scroll vertically (horizontal wheel scrolls horizontally) |
| Left click/drag (OSK) | Tap a key; holding it auto-repeats |
| One finger | Acts as the left mouse button |
| Two fingers | Scroll the editor |

## Controller

Bound in `autoexec` as `pad:*` pseudo-keys. The shoulders are modifier layers.

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

Held inputs auto-repeat. Controllers can be plugged and unplugged at any time.

## On-screen keyboard

Shown by `osk toggle` (pad: left stick click); takes `dim_osk_height` percent of the window.

| | |
|---|---|
| Pages | Two pages, letters and symbols, swapped by the `#+=` key |
| Sticky keys | Ctrl/Shift/Alt/AltGr cycle idle → latched (dot, next key only) → held (wider dot, until pressed again). A long press by touch or mouse goes straight to held |
| L as Shift | While the OSK is visible, holding the controller's L shoulder shifts its keys |
| Auto-repeat | Holding a repeatable key re-emits it |
| Pad keys | D-pad moves the key cursor, A presses, B hands the pad back (or cancels the prompt instead, while one is active) |
| Layouts | `osk layout <name>`: qwerty, azerty, qwertz, uk, spanish, spanish_latin, italian, portuguese, russian |

## Commands

### Files and buffers

| Command | Description |
|---------|-------------|
| `open <filename>` | Open a file (prompts for the path when bound to a key); switches to the existing buffer when the file is already open |
| `buffer <next\|prev\|name>` | Cycle through the open buffers, or switch to one by name |
| `buffer close [-f]` | Close the active buffer; `-f` skips the unsaved-changes confirmation |
| `save <filename> [-f]` | Save the buffer (prompts for a name when it has none); `-f` skips the overwrite confirmation |
| `quit [-f]` | Exit; `-f` skips the unsaved-changes confirmation |
| `exec <filename>` | Execute commands from a file, line by line |
| `help [section]` | Open this manual, optionally jumping to a section |

### Cursor, search and clipboard

| Command | Description |
|---------|-------------|
| `move <direction> [true]` | Move the cursor (up/down/left/right/bol/eol/bof/eof/page_up/page_down); `true` extends the selection |
| `goto_line <line>` | Jump to a 1-based line (clamped to range) |
| `search <term>` | Store the term and select its first match, reporting the match count |
| `find_next` / `find_prev` | Select the next / previous match (wraps around) |
| `replace <from> <to>` | Replace the next occurrence of `from` with `to` |
| `replace_all <from> <to>` | Replace every occurrence of `from` with `to` |
| `copy` / `cut` / `paste` | Clipboard operations on the selection |
| `undo` / `redo` | Linear undo/redo (`dim_max_undo` entries deep) |

### Configuration and system

| Command | Description |
|---------|-------------|
| `cvar <name> [values...]` | Print or set a configuration variable |
| `bind <modifiers> <key> "<command>"` | Bind a key or `pad:*` pseudo-key to a command |
| `set_font_size <size\|+\|->` | Set the font size, absolute or relative |
| `set_hl_mode <mode>` | Force the syntax highlight mode (cpp, json, ini, yaml, toml, markdown) |
| `activate_prompt` | Open the command prompt |
| `prompt <confirm\|cancel>` | Confirm or cancel the prompt |
| `auto_complete <forward\|backward>` | Cycle the prompt completions |
| `osk <show\|hide\|toggle>` | Control the on-screen keyboard |
| `osk layout <name>` | Select the OSK layout |
| `reset_draw_time` / `reset_command_time` | Reset the performance metric CVars |

## Configuration

```
cvar <name>                                 print the current value
cvar col_editor_background 250 250 250 255  set it (colors: r g b a, 0-255)
```

Put the commands in `romfs/autoexec` to make them permanent; `romfs/light_theme` and
`romfs/dark_theme` are command scripts setting the colors below.

On Switch `romfs/` is packaged inside the NRO and read-only, so the editable `autoexec` is the
copy bbloc writes next to `bbloc.nro` on the first run and runs from then on. Edit that one;
delete it to get the shipped defaults back. The console light/dark color set is applied just
before it, so colors set in `autoexec` override the system scheme.

### Settings

| Variable | Type | Description |
|----------|------|-------------|
| `tab_to_space` | bool | Insert spaces instead of a tab character |
| `search_case_sensitive` | bool | Whether search and replace match case |
| `show_scrollbar` | bool | Show editor scrollbars when content overflows |
| `inf_draw_time` | float | Maximum render time in seconds (read-only) |
| `inf_command_time` | float | Maximum command processing time (read-only) |

### Interface colors

| Variable | Description |
|----------|-------------|
| `col_margin_background` | Margin background |
| `col_info_bar_background` | Info bar background |
| `col_editor_background` | Editor background |
| `col_prompt_background` | Prompt background |
| `col_current_line_background` | Current line highlight |
| `col_selected_text_background` | Selected text background |
| `col_line_number` | Line numbers |
| `col_info_bar_text` | Info bar text |
| `col_prompt_text` | Prompt label text |
| `col_prompt_input_text` | Prompt input text |
| `col_border` | View borders |
| `col_cursor_indicator` | Caret |
| `col_scrollbar` | Scrollbar track |
| `col_scrollbar_thumb` | Scrollbar thumb |
| `col_osk_background` | OSK strip background |
| `col_osk_key_background` | OSK key background |
| `col_osk_key_text` | OSK key labels |
| `col_osk_key_cursor` | Tint over the pad-selected OSK key; keep it translucent, an opaque value hides the key color underneath |
| `col_osk_key_pressed` | Pressed/latched OSK key |

### Syntax colors

| Variable | Description |
|----------|-------------|
| `hl_text` | Plain text |
| `hl_comment` | Comments |
| `hl_string` | Strings |
| `hl_preprocessor` | Preprocessor directives |
| `hl_number` | Numbers |
| `hl_keyword` | Keywords |
| `hl_statement` | Statements |
| `hl_type` | Types |
| `hl_constant` | Constants |
| `hl_function` | Functions and methods |
| `hl_variable` | Variables and fields |

### Dimensions

| Variable | Description |
|----------|-------------|
| `dim_padding_width` | Padding width in pixels |
| `dim_indicator_width` | Caret width in pixels |
| `dim_border_size` | Border size in pixels |
| `dim_tab_to_space` | Spaces per tab character |
| `dim_page_up_down` | Lines per page scroll |
| `dim_scrollbar_width` | Scrollbar thickness in pixels |
| `dim_font_size` | Font size in pixels |
| `dim_max_history` | Prompt command-history size |
| `dim_max_undo` | Undo/redo history depth (1-4096) |
| `dim_osk_height` | On-screen keyboard height, in percent of the window height |
| `dim_osk_key_gap` | Gap between on-screen keyboard keys, in pixels |
