#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include "cursor/Cursor.h"
#include "cursor/PromptCursor.h"
#include "base/CommandFeedback.h"
#include "highlighter/HighLighter.h"
#include "theme/Theme.h"
#include "FocusTarget.h"
#include "base/CommandRunner.h"


/**
 * @brief Holds runtime context for a cursor instance, including rendering state, input focus, command execution, and more.
 *
 * This structure is passed around to update or interact with a cursor, allowing access to his state,
 * command feedback, highlighting, prompt, and UI preferences.
 */
struct CursorContext {
    /** Runtime objects */
    CommandRunner &command_runner;  ///< The command runner object of the application.
    Theme &theme;                   ///< The theme object of the application.
    PromptCursor &prompt_cursor;    ///< The prompt cursor object of the application.
    Cursor cursor;                  ///< The Cursor who is tied to this context.
    HighLighter highlighter;        ///< The highlighter used to highlight the text.
    FocusTarget focus_target;       ///< The currently focused input target.

    /** Dynamic variable meant to manipulate the views */
    int32_t scroll_x;               ///< Horizontal scroll value of cursor.
    int32_t scroll_y;               ///< Vertical scroll value of this cursor.
    bool follow_indicator;          ///< Indicate that the view should scroll to the indicator when rendering.
    bool wants_redraw;              ///< indicate that the view and parents or sibling should redraw due to state change.

    bool stick_to_column;           ///< Flag indicating that the next move (up or down) must place the cursor column
    uint32_t stick_column_index;    ///< Column where the cursor column must "stick".

    /** The feedback prompt state. */
    std::optional<CommandFeedback> command_feedback;

    /**
     * @brief Constructs a CursorContext with the required runtime object and a buffer.
     *
     * Initializes cursor and highlighter from the provided text buffer.
     * Sets the default focus to the editor and enables redraw.
     *
     * @param commandRunner The CommandRunner used to execute text commands.
     * @param theme The Theme instance applied to this context.
     * @param promptCursor The PromptCursor used for command-line input interaction.
     * @param buffer The text buffer to be owned and manipulated by the Cursor.
     */
    explicit CursorContext(CommandRunner &commandRunner, Theme &theme, PromptCursor &promptCursor, std::unique_ptr<TextBuffer> buffer)
        : command_runner(commandRunner),
          theme(theme),
          prompt_cursor(promptCursor),
          cursor(std::move(buffer)),
          highlighter(cursor),
          focus_target(FocusTarget::Editor),
          scroll_x(0),
          scroll_y(0),
          stick_to_column(false),
          stick_column_index(0),
          follow_indicator(false),
          wants_redraw(true) {
    }
};


#endif //CURSOR_CONTEXT_H
