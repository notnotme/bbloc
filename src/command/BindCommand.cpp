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
#include "BindCommand.h"

#include <ranges>
#include <unordered_set>

#include <SDL_keyboard.h>
#include <utf8.h>

#include "../core/base/CommandLine.h"
#include "../core/base/PadInput.h"
#include "../core/CommandManager.h"


const U16StringMap<uint16_t> BindCommand::MODIFIER_MAP = {
    { u"Ctrl", KMOD_CTRL },
    { u"Shift", KMOD_SHIFT },
    { u"Alt", KMOD_ALT },
    { u"L", PadInput::KMOD_PAD_L },
    { u"R", PadInput::KMOD_PAD_R },
    { u"None", KMOD_NONE }
};

BindCommand::BindCommand(CommandManager &commandManager)
    : m_command_manager(commandManager) {}

void BindCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex == 0) {
        // Complete the last modifier of an eventual "+" separated combo, keeping what precedes it.
        const auto last_plus_index = input.rfind(u'+');
        const auto combo_prefix = last_plus_index == std::u16string_view::npos ? std::u16string_view{} : input.substr(0, last_plus_index + 1);
        const auto modifier_input = last_plus_index == std::u16string_view::npos ? input : input.substr(last_plus_index + 1);

        for (const auto &name : std::views::keys(MODIFIER_MAP)) {
            if (name.starts_with(modifier_input)) {
                itemCallback(std::u16string(combo_prefix).append(name));
            }
        }
    } else if (argumentIndex == 1) {
        // Enumerate every key name known to SDL, skipping unnamed keys and duplicates.
        // The walk starts at the first real scancode: 1 to 3 are not SDL_Scancode values at all,
        // and SDL maps them to SDLK_UNKNOWN, so they were already skipped below.
        auto seen_names = std::unordered_set<std::string>{};
        for (int32_t scancode = SDL_SCANCODE_A; scancode < SDL_NUM_SCANCODES; ++scancode) {
            const auto key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
            if (key == SDLK_UNKNOWN) {
                continue;
            }

            const auto key_name = std::string(SDL_GetKeyName(key));
            if (key_name.empty() || !seen_names.insert(key_name).second) {
                continue;
            }

            const auto utf16_key_name = utf8::utf8to16(key_name);
            if (utf16_key_name.starts_with(input)) {
                itemCallback(utf16_key_name);
            }
        }

        // Pad inputs ("pad:...") complete alongside the SDL key names.
        PadInput::forEachName([&input, &itemCallback](const std::u16string_view padName) {
            if (padName.starts_with(input)) {
                itemCallback(padName);
            }
        });
    } else if (argumentIndex == 2) {
        // Hidden commands are included: binding them to a keystroke is their intended use.
        m_command_manager.getCommandCompletions(input, true, itemCallback);
    }
}

std::optional<std::u16string> BindCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    (void) payload;
    if (args.size() != 3 || args[1].empty()) {
        return u"Usage: bind <modifiers> <key> <command>";
    }

    // Split the first argument which should be the modifier keys.
    const auto split_modifiers = CommandLine::split(args[0], u'+');

    // Normalize the modifiers, as the app does not make the difference between left and right.
    auto modifier = 0;
    for (const auto string_modifier : split_modifiers) {
        const auto mapped_modifier = mapModifier(string_modifier);
        if (mapped_modifier == -1) {
            return std::u16string(u"Unknown modifier: ").append(string_modifier);
        }
        modifier |= mapped_modifier;
    }

    // Pad input names ("pad:...") resolve first, into their reserved negative keycode space;
    // everything else must be a key name SDL knows.
    auto key = PadInput::keycodeFromName(args[1]);
    if (key == SDLK_UNKNOWN) {
        const auto keycode_utf8 = utf8::utf16to8(args[1]);
        key = SDL_GetKeyFromName(keycode_utf8.data());
    }

    if (key == SDLK_UNKNOWN) {
        return std::u16string(u"Unknown key: ").append(args[1]);
    }

    // Insert the command "as-it".
    m_bindings[key].insert_or_assign(modifier, args[2]);
    return std::nullopt;
}

std::optional<std::u16string_view> BindCommand::getBinding(const SDL_Keycode keycode, const uint16_t modifiers) {
    const auto normalized_modifiers = normalizeModifiers(modifiers);
    if (const auto &map_entry = m_bindings.find(keycode); map_entry != m_bindings.end()) {
        if (const auto &binding = map_entry->second.find(normalized_modifiers); binding != map_entry->second.end()) {
            return binding->second;
        }
    }

    return std::nullopt;
}

uint16_t BindCommand::normalizeModifiers(const uint16_t modifiers) {
    auto result = 0;
    if (modifiers & (KMOD_LCTRL | KMOD_RCTRL)) {
        result |= KMOD_CTRL;
    }

    if (modifiers & (KMOD_LSHIFT | KMOD_RSHIFT)) {
        result |= KMOD_SHIFT;
    }

    if (modifiers & (KMOD_LALT | KMOD_RALT)) {
        result |= KMOD_ALT;
    }

    if (modifiers & (KMOD_LGUI | KMOD_RGUI)) {
        result |= KMOD_GUI;
    }

    // The pad modifier bits (held shoulders) have no left/right variants: pass them through.
    result |= modifiers & (PadInput::KMOD_PAD_L | PadInput::KMOD_PAD_R);

    return result;
}

int32_t BindCommand::mapModifier(const std::u16string_view modifier) {
    if (const auto &mapped_modifier = MODIFIER_MAP.find(modifier); mapped_modifier != MODIFIER_MAP.end()) {
        return mapped_modifier->second;
    }

    return -1;
}
