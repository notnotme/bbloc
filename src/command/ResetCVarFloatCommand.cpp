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
#include "ResetCVarFloatCommand.h"


ResetCVarFloatCommand::ResetCVarFloatCommand(std::shared_ptr<CVarFloat> cvar)
    : m_cvar(std::move(cvar)) {}

void ResetCVarFloatCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ResetCVarFloatCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    (void) payload;
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    m_cvar->m_value = 0.0f;
    return std::nullopt;
}
