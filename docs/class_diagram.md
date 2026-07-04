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
    class VectorBuffer {
        note: "one u16string per line"
    }
    class StringBuffer {
        note: "single contiguous u16string"
    }
    class LineBuffer {
        note: "current line extracted for fast edits"
    }
    class BufferEdit {
        <<struct>>
    }

    TextBuffer <|-- VectorBuffer
    TextBuffer <|-- StringBuffer
    TextBuffer <|-- LineBuffer
    TextBuffer ..> BufferEdit : produces
```

---

## 4. Cursors (`core/cursor`)

```mermaid
classDiagram
    class Cursor {
        note: "multi-line, selection support, undo/redo"
    }
    class PromptCursor {
        note: "single-line command input"
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

    QuadProgram ..> QuadBuffer : binds & draws
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
    }
    class PromptState
    class View~TState~ {
        <<abstract>>
    }
    class Editor
    class InfoBar
    class Prompt

    ViewState <|-- PromptState
    View~TState~ <|-- Editor
    View~TState~ <|-- InfoBar
    View~TState~ <|-- Prompt
    Editor ..> ViewState : TState
    InfoBar ..> ViewState : TState
    Prompt ..> PromptState : TState
```

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
    class OpenFileCommand
    class SaveFileCommand
    class ExecCommand
    class QuitCommand
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
    class CursorContext {
        <<runtime struct>>
    }
    class ScrollState {
        <<struct>>
        note: "scroll x/y + follow_indicator"
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
    ApplicationWindow *-- CursorContext
    ApplicationWindow *-- View~TState~
    CommandManager o-- Command~T~
    CursorContext *-- TextBuffer
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
