/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include <cstddef>
#include <optional>
#include <string>

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
struct CursorContext final {
private:
    /**
     * @brief Scroll state of a cursor's view.
     */
    struct ScrollState final {
        int32_t x = 0;                  ///< Horizontal scroll value of cursor.
        int32_t y = 0;                  ///< Vertical scroll value of cursor.
        bool follow_indicator = false;  ///< View should scroll to the indicator on next render.
    };

    /**
     * @brief Column-sticking state used by vertical cursor moves.
     */
    struct ColumnStick final {
        bool active = false;            ///< Next up/down move must place the cursor at `index`.
        uint32_t index = 0;             ///< Column where the cursor must "stick".
    };

    /**
     * @brief State of the current search session.
     */
    struct SearchState final {
        std::optional<std::u16string> term;  ///< The last searched term, if any.
        int32_t match_index = -1;            ///< Zero-based ordinal of current match, -1 when none.
        int32_t match_count = 0;             ///< Total matches for the current term, 0 when none.

        /** @brief Forgets the match statistics while keeping the term, so find_next/find_prev still work. */
        void resetMatches() {
            match_index = -1;
            match_count = 0;
        }
    };

public:
    /** Runtime objects */
    CommandRunner &command_runner;  ///< The command runner object of the application.
    Theme &theme;                   ///< The theme object of the application.
    PromptCursor &prompt_cursor;    ///< The prompt cursor object of the application.
    Cursor cursor;                  ///< The Cursor who is tied to this context.
    HighLighter highlighter;        ///< The highlighter used to highlight the text.
    FocusTarget focus_target = FocusTarget::Editor;  ///< The currently focused input target.

    /** Dynamic variables meant to manipulate the views */
    ScrollState scroll;             ///< Scroll state of this cursor's view.
    ColumnStick stick;              ///< Column-sticking state for vertical moves.
    SearchState search;             ///< State of the current search session.

    /** The feedback prompt state. */
    std::optional<CommandFeedback> command_feedback;

    bool wants_redraw = true;       ///< indicate that the view and parents or sibling should redraw due to state change.
    bool from_prompt = false;       ///< True when the current command was invoked from the interactive prompt.

    /** Position of this context among the open ones, maintained by CursorContextManager. */
    size_t buffer_index = 1;        ///< 1-based index of this context among the open contexts.
    size_t buffer_count = 1;        ///< Total number of open contexts.

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
          highlighter(cursor) {}
};


#endif //CURSOR_CONTEXT_H
