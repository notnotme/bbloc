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
#ifndef MOVE_CURSOR_COMMAND_H
#define MOVE_CURSOR_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/base/U16StringMap.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command for moving the cursor within the text editor.
 *
 * This class implements the Command interface for cursor navigation,
 * allowing users to move the cursor to predefined positions within the document.
 */
class MoveCursorCommand final : public Command<CursorContext> {
private:
    enum class Movement {
        UNKNOWN,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        BEGIN_LINE,
        END_LINE,
        PAGE_UP,
        PAGE_DOWN,
        BEGIN_FILE,
        END_FILE
    };

    enum class Boolean {
        UNKNOWN,
        TRUE,
        FALSE
    };

private:
    /** Lookup map to ease mapping Movement. */
    static const U16StringMap<Movement> MOVEMENT_MAP;

    /** Lookup map to ease mapping Boolean. */
    static const U16StringMap<Boolean> BOOLEAN_MAP;

    /** Reference to the prompt state. */
    PromptState &m_prompt_state;

    /**
     * @brief Map a movement string representation to the enum representation.
     * @param movement The movement string to map.
     * @return The Movement value, UNKNOWN otherwise.
     */
    static Movement mapMovement(std::u16string_view movement);

    /**
     * @brief Map a boolean string representation to the enum representation.
     * @param value The boolean string to map.
     * @return The Boolean value, UNKNOWN otherwise.
     */
    static Boolean mapBoolean(std::u16string_view value);

    /**
     * @brief Update the payload's cursor column position to its "sticky" value, if possible. Or does not move at all.
     * @param payload The payload with the runtime information allowing to compute the sticky state and update the cursor.
     */
    static void stickToColumn(CursorContext &payload);

public:
    /**
     * @brief Constructs a MoveCursorCommand with a reference to the prompt state.
     *
     * @param promptState Reference to the current prompt state for context-aware navigation.
     */
    explicit MoveCursorCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first and second arguments.
     * The first is the direction which can one of: "up", "down", "left", "right", "page_up", "page_down", "bof", "eof",
     * "bol" or "eol".
     * The second is "true" or "false", which activates / extends the selection or cancels it.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the cursor movement.
     *
     * Moves the cursor within the document based on the provided arguments.
     * The first is the direction that can one of: "up", "down", "left", "right", "page_up", "page_down", "bof", "eof",
     * "bol" or "eol".
     * The second is "true" or "false", which activates / extends the selection or cancels it.
     *
     * @param payload The cursor context that will be modified by this command.
     * @param args Command arguments specifying how to move the cursor.
     * @return An optional message indicating the new cursor position or the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //MOVE_CURSOR_COMMAND_H
