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
#include "CVarCommand.h"

#include <SDL_keyboard.h>
#include <utf8.h>

#include "CursorContext.h"


void CVarCommand::registerCvar(const std::u16string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    const auto &[new_entry, success] = m_cvars.insert({ std::u16string(name), CVarEntry{.cvar = std::move(cvar), .callback = callback} });
    if (!success) {
        throw std::runtime_error(std::string("CVar already registered: ").append(utf8::utf16to8(name)));
    }
}

void CVarCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex > 0) {
        // Let the CVar being set suggest candidates for the component being completed
        if (previousArgs.empty()) {
            return;
        }

        const auto &cvar_entry = m_cvars.find(previousArgs[0]);
        if (cvar_entry == m_cvars.end()) {
            return;
        }

        cvar_entry->second.cvar->provideValueCompletion(argumentIndex - 1,
            [&](const std::u16string_view completion) {
                if (completion.starts_with(input)) {
                    itemCallback(completion);
                }
            });

        return;
    }

    for (const auto &name : std::views::keys(m_cvars)) {
        if (name.starts_with(input)) {
            itemCallback(name);
        }
    }
}

std::optional<std::u16string> CVarCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    (void) payload;
    if (args.empty()) {
        return u"Usage: cvar <name> [value1] [value2] ...";
    }

    const auto &cvar_entry = m_cvars.find(args[0]);
    if (cvar_entry == m_cvars.end()) {
        return std::u16string(u"Unknown cvar: ").append(args[0]);
    }

    auto *cvar = cvar_entry->second.cvar.get();

    if (args.size() == 1) {
        // Print the value of this cvar
        return std::u16string(args[0]).append(u": ").append(cvar->getStringValue());
    }

    // Se the value of this cvar if not read-only
    if (cvar->isReadOnly()) {
        return std::u16string(u"CVar is read-only: ").append(args[0]);
    }

    if (const auto &error = cvar->setValueFromStrings(args.subspan(1))) {
        // Something wrong happened
        return std::u16string(args[0]).append(u": ").append(error.value());
    }

    // Eventually invoke the associated callback
    if (const auto &callback = cvar_entry->second.callback) {
        callback();
    }

    return std::nullopt;
}
