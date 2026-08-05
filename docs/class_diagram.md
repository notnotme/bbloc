# Class Diagram — C++ Text Editor

---

## 1. Command System (`core/base` + `core/`)

```mermaid
classDiagram
    class Command~TPayload~ {
        <<abstract>>
    }
    class CommandRegistry~TPayload~ {
        <<interface>>
    }
    class CVarRegistry {
        <<interface>>
    }
    class GlobalRegistry~TPayload~ {
        <<abstract>>
    }
    class CommandRunner {
        <<interface>>
    }
    class CommandManager
    class CVarCommand
    class CommandFeedback {
        <<struct>>
        note: "prompt message + optional completion provider + validate callback"
    }

    CommandRegistry~TPayload~ <|-- GlobalRegistry~TPayload~
    CVarRegistry <|-- GlobalRegistry~TPayload~
    GlobalRegistry~TPayload~ <|-- CommandManager
    Command~TPayload~ <|-- CVarCommand
    CVarRegistry <|-- CVarCommand
    CommandManager *-- CVarCommand
    CommandManager o-- Command~TPayload~
    Command~TPayload~ ..> CommandFeedback : may request
```

---

## 2. Configuration Variables (`core/cvar`)

```mermaid
classDiagram
    class CVar {
        <<abstract>>
    }
    class TypedCVar~T~ {
        <<abstract>>
    }
    class CVarBool
    class CVarInt
    class CVarFloat
    class CVarColor

    CVar <|-- TypedCVar~T~
    TypedCVar~T~ <|-- CVarBool
    TypedCVar~T~ <|-- CVarInt
    TypedCVar~T~ <|-- CVarFloat
    TypedCVar~T~ <|-- CVarColor
```

---

## 3. Text Buffers (`core/cursor/buffer`)

```mermaid
classDiagram
    class TextBuffer {
        <<abstract>>
    }
    class LineBuffer {
        note: "single contiguous u16string, current line extracted for fast edits"
    }
    class LongestLineTracker {
        note: "incrementally tracks the longest line of a TextBuffer"
    }
    class BufferEdit {
        <<struct>>
    }

    TextBuffer <|-- LineBuffer
    LineBuffer *-- LongestLineTracker
    TextBuffer ..> BufferEdit : produces
```

---

## 4. Cursors (`core/cursor`)

```mermaid
classDiagram
    class Cursor {
        note: "multi-line, selection support, undo/redo, modified flag; uint32 line/column"
    }
    class PromptCursor {
        note: "single-line command input; uint32 column"
    }
    class SurrogatePair {
        <<free functions>>
        note: "charLengthBefore / charLengthAfter, shared surrogate-pair stepping"
    }
    class TextBuffer {
        <<abstract>>
    }
    class TextRange {
        <<struct>>
    }
    class UndoHistory {
        note: "linear snapshot stacks, cvar-capped depth"
    }
    class Snapshot {
        <<struct>>
    }
    class CVarInt

    Cursor *-- TextBuffer
    Cursor *-- UndoHistory
    Cursor ..> SurrogatePair : uses
    PromptCursor ..> SurrogatePair : uses
    UndoHistory *-- Snapshot : nested
    UndoHistory o-- CVarInt : shared dim_max_undo
    Cursor ..> TextRange : returns
    Cursor ..> BufferEdit : produces
```

---

## 5. Syntax Highlighting (`core/highlighter`)

```mermaid
classDiagram
    class HighLighter {
        note: "Tree-sitter based"
    }
    class HighLightId {
        <<enum>>
        None
        Cpp
        Json
        Ini
        Yaml
        Toml
    }
    class TokenId {
        <<enum>>
        None / Keyword / Statement
        String / Number / Comment
        Preprocessor / Type / Constant
    }
    class Parser {
        note: "owns the compiled TSQuery for one language"
    }
    class ParserDescriptor {
        <<struct>>
    }
    class ParserCatalog {
        note: "static factory for parsers and descriptors"
    }
    class Cursor

    HighLighter o-- Cursor : const ref
    HighLighter *-- Parser : map by HighLightId
    Parser --> ParserDescriptor : const ref
    ParserCatalog ..> Parser : creates
    HighLighter --> HighLightId
    HighLighter ..> TokenId : produces
```

---

## 6. OpenGL Renderer (`core/renderer`)

```mermaid
classDiagram
    class QuadProgram
    class QuadBuffer
    class QuadTexture
    class AtlasArray
    class AtlasEntry {
        <<struct>>
    }
    class QuadVertex {
        <<struct>>
    }
    class Shader {
        <<free functions>>
        note: "compileShader / checkProgram helpers"
    }

    QuadProgram ..> QuadBuffer : binds & draws
    QuadProgram ..> Shader : uses
    QuadBuffer *-- QuadVertex
    AtlasArray *-- AtlasEntry
    AtlasArray ..> QuadTexture : writes via blit
```

---

## 7. Theme (`core/theme`)

```mermaid
classDiagram
    class Theme {
        note: "FreeType fonts + CVars + atlas"
    }
    class TabStop {
        <<free functions>>
        note: "nextTabStop / visualColumns, shared tab-stop arithmetic for the text walks"
    }
    class ColorId {
        <<enum>>
    }
    class DimensionId {
        <<enum>>
    }
    class AtlasArray
    class QuadTexture
    class CVarColor
    class CVarInt

    Theme *-- AtlasArray
    Theme *-- QuadTexture
    Theme o-- CVarColor : ColorId + TokenId map
    Theme o-- CVarInt : DimensionId + font size map
    Theme --> ColorId
    Theme --> DimensionId
```

---

## 8. Views & States (`core/` + `editor/` + `infobar/` + `prompt/`)

```mermaid
classDiagram
    class ViewState {
        <<abstract>>
        note: "int32 window rectangle (position + size)"
    }
    class PromptState
    class View~TState~ {
        <<abstract>>
        +render(context, viewState, quadBuffer, dt)*
        +onKeyDown(context, viewState, keyCode, keyModifier)*
        +onTextInput(context, viewState, text)*
        +onMouseDown(context, viewState, x, y)
        +onMouseMotion(context, viewState, x, y)
        +onMouseUp(context, viewState, x, y)
    }
    class Editor {
        note: "overrides the mouse handlers: caret placement, drag selection, scrollbar thumb drags and track page jumps"
    }
    class InfoBar
    class Prompt
    class QuadBuffer {
        note: "single shared instance, passed to render()"
    }
    class TabStop {
        <<free functions>>
    }

    ViewState <|-- PromptState
    View~TState~ <|-- Editor
    View~TState~ <|-- InfoBar
    View~TState~ <|-- Prompt
    Editor ..> ViewState : TState
    InfoBar ..> ViewState : TState
    Prompt ..> PromptState : TState
    View~TState~ ..> QuadBuffer : stages one batch per render()
    Editor ..> TabStop : uses
    InfoBar ..> TabStop : uses
    Prompt ..> TabStop : uses
```

The mouse handlers have empty default implementations; `InfoBar` and `Prompt` keep them.
`ApplicationWindow::mainLoop` routes a left `SDL_MOUSEBUTTONDOWN` to the view whose rectangle
contains the point, then captures that view: `SDL_MOUSEMOTION` and `SDL_MOUSEBUTTONUP` keep
going to it until the button is released, even when the pointer leaves the view.

---

## 9. Concrete Commands (`command/`)

```mermaid
classDiagram
    class Command~CursorContext~ {
        <<abstract>>
    }
    class BindCommand
    class MoveCursorCommand
    class AutoCompleteCommand
    class ActivatePromptCommand
    class FontSizeCommand
    class ResetCVarFloatCommand
    class OpenFileCommand {
        note: "activates the existing buffer when the file is already open, via CursorContextManager"
    }
    class SaveFileCommand
    class ExecCommand
    class QuitCommand {
        note: "confirms when any open buffer is modified, via CursorContextManager"
    }
    class SetHighLightCommand
    class CopyTextCommand
    class CutTextCommand
    class PasteTextCommand
    class UndoCommand
    class RedoCommand
    class SearchCommand {
        note: "search / find_next / find_prev / replace / replace_all"
    }
    class GotoLineCommand
    class BufferCommand {
        note: "buffer next / prev / close / name, via CursorContextManager"
    }

    Command~CursorContext~ <|-- BindCommand
    Command~CursorContext~ <|-- MoveCursorCommand
    Command~CursorContext~ <|-- AutoCompleteCommand
    Command~CursorContext~ <|-- ActivatePromptCommand
    Command~CursorContext~ <|-- FontSizeCommand
    Command~CursorContext~ <|-- ResetCVarFloatCommand
    Command~CursorContext~ <|-- OpenFileCommand
    Command~CursorContext~ <|-- SaveFileCommand
    Command~CursorContext~ <|-- ExecCommand
    Command~CursorContext~ <|-- QuitCommand
    Command~CursorContext~ <|-- SetHighLightCommand
    Command~CursorContext~ <|-- CopyTextCommand
    Command~CursorContext~ <|-- CutTextCommand
    Command~CursorContext~ <|-- PasteTextCommand
    Command~CursorContext~ <|-- UndoCommand
    Command~CursorContext~ <|-- RedoCommand
    Command~CursorContext~ <|-- SearchCommand
    Command~CursorContext~ <|-- GotoLineCommand
    Command~CursorContext~ <|-- BufferCommand
```

---

## 10. Global Overview

```mermaid
classDiagram
    class ApplicationWindow {
        <<entry point>>
    }
    class CommandRunner {
        <<interface>>
    }
    class CommandManager {
        <<global registry>>
    }
    class Theme
    class CursorContextManager {
        note: "owns all open contexts, one active; never empty"
    }
    class CursorContext {
        <<runtime struct>>
        note: "one per open file; buffer_index / buffer_count"
    }
    class Cursor
    class PromptCursor
    class ScrollState {
        <<struct>>
        note: "scroll x/y (int64 content-space pixels) + follow_indicator"
    }
    class ColumnStick {
        <<struct>>
        note: "sticky column for vertical moves"
    }
    class SearchState {
        <<struct>>
        note: "term + match index/count"
    }
    class CommandFeedback {
        <<struct>>
        note: "pending interactive prompt"
    }
    class View~TState~ {
        <<abstract>>
    }
    class Command~T~ {
        <<abstract>>
    }
    class TextBuffer {
        <<abstract>>
    }
    class CVar {
        <<abstract>>
    }
    class Renderer {
        <<OpenGL module>>
    }
    class HighLighter

    ApplicationWindow --|> CommandRunner
    ApplicationWindow *-- CommandManager
    ApplicationWindow *-- Theme
    ApplicationWindow *-- PromptCursor
    ApplicationWindow *-- CursorContextManager
    ApplicationWindow *-- View~TState~
    CommandManager o-- Command~T~
    CursorContextManager *-- CursorContext : one per open file
    CursorContext *-- Cursor
    CursorContext o-- PromptCursor : shared ref
    CursorContext o-- Theme : shared ref
    Cursor *-- TextBuffer
    CursorContext *-- HighLighter
    CursorContext *-- ScrollState
    CursorContext *-- ColumnStick
    CursorContext *-- SearchState
    CursorContext *-- CommandFeedback : optional
    Theme o-- CVar
    View~TState~ o-- Renderer
    View~TState~ ..> CursorContext : receives as parameter
    Command~T~ ..> CursorContext : execution payload
```
