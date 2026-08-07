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
#ifndef OSK_COMMAND_H
#define OSK_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../osk/OskState.h"


/**
 * @brief Command controlling the on-screen keyboard: visibility and layout.
 *
 * "osk show|hide|toggle" changes the visibility — the views relayout through the resize
 * path, and follow_indicator keeps the caret in view. Showing never grabs anything (the
 * OSK takes the pad lazily, on the first pad press routed to it); hiding releases the pad
 * focus if the OSK held it. "osk layout <name>" switches the key layout at runtime,
 * overriding the platform default.
 */
class OskCommand final : public Command<CursorContext> {
private:
    /** Reference to the on-screen keyboard state this command drives. */
    OskState &m_osk_state;

public:
    /**
     * @brief Constructs an OskCommand with the OSK state it drives.
     *
     * @param oskState Reference to the on-screen keyboard state.
     */
    explicit OskCommand(OskState &oskState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * Suggests the subcommands for the first argument, and the layout names after "layout".
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the osk command.
     *
     * Expects "show", "hide", "toggle", or "layout <name>".
     *
     * @param payload The cursor context; visibility changes touch its scroll and redraw state.
     * @param args Command arguments.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //OSK_COMMAND_H
