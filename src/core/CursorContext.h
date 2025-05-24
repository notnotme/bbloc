#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include "cursor/Cursor.h"
#include "cursor/PromptCursor.h"
#include "base/CommandFeedback.h"
#include "highlighter/HighLighter.h"
#include "theme/Theme.h"
#include "FocusTarget.h"
#include "base/CommandRunner.h"


struct CursorContext {
    CommandRunner &command_runner;  ///< The command runner object of the application.
    Theme &theme;                   ///< The theme object of the application.
    PromptCursor &prompt_cursor;    ///< The prompt cursor object of the application.
    Cursor cursor;                  ///< The Cursor who is tied to this context.
    HighLighter highlighter;        ///< The highlighter used to highlight the text.
    FocusTarget focus_target;       ///< The currently focused input target.
    int32_t scroll_x;               ///< Horizontal scroll value of cursor.
    int32_t scroll_y;               ///< Vertical scroll value of this cursor.
    bool follow_indicator;          ///< Indicate that the view should scroll to the indicator when rendering.
    bool wants_redraw;              ///< indicate that the view and parents or sibling should redraw due to state change.

    /** Active feedback prompt state. */
    std::optional<CommandFeedback> command_feedback;

    explicit CursorContext(CommandRunner &commandRunner, Theme &theme, PromptCursor &promptCursor, std::unique_ptr<TextBuffer> buffer)
        : command_runner(commandRunner),
          theme(theme),
          prompt_cursor(promptCursor),
          cursor(std::move(buffer)),
          highlighter(cursor),
          focus_target(FocusTarget::Editor),
          scroll_x(0),
          scroll_y(0),
          follow_indicator(false),
          wants_redraw(true) {}
};


#endif //CURSOR_CONTEXT_H
