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
#ifndef RESET_CVAR_FLOAT_COMMAND_H
#define RESET_CVAR_FLOAT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/cvar/CVarFloat.h"


/**
 * @brief Command for resetting a floating-point configuration variable to its default value.
 *
 * This class implements the Command interface for resetting float-type CVars to 0.
 */
class ResetCVarFloatCommand final : public Command<CursorContext> {
private:
    /** Reference to the float CVar that this command will manage. */
    std::shared_ptr<CVarFloat> m_cvar;

public:
    /** @brief Constructs a ResetCVarFloatCommand with default initialization. */
    explicit ResetCVarFloatCommand(std::shared_ptr<CVarFloat> cvar);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not auto-complete.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the CVar reset operation.
     *
     * Resets the specified float-type CVar to its default value.
     * This command expect no argument (empty vector).
     *
     * @param payload The cursor context (not directly used for CVar operations).
     * @param args Command arguments; must be empty (the target CVar is bound at construction).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //RESET_CVAR_FLOAT_COMMAND_H
