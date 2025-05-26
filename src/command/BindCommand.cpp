#include "BindCommand.h"

#include <SDL_keyboard.h>
#include <utf8.h>

#include "../core/CommandManager.h"


void BindCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    // TODO
}

std::optional<std::u16string> BindCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (args.size() < 3 || (!args.empty() && args[1].empty())) {
        return u"Usage: bind <modifiers> <key> <command>";
    }

    const auto split_modifiers = CommandManager::split(args[0], u'+');

    auto modifier = 0;
    for (const auto &string_modifier : split_modifiers) {
        if (string_modifier == u"Ctrl") {
            modifier |= KMOD_CTRL;
        } else if (string_modifier == u"Alt") {
            modifier |= KMOD_ALT;
        } else if (string_modifier == u"Shift") {
            modifier |= KMOD_SHIFT;
        } else if (string_modifier == u"None") {
            // Dummy op
        } else {
            return std::u16string(u"Unknown modifier: ").append(string_modifier);
        }
    }

    const auto keycode_utf8 = utf8::utf16to8(args[1]);
    const auto key = SDL_GetKeyFromName(keycode_utf8.data());
    if (! m_bindings.contains(key)) {
        m_bindings.emplace(key, std::unordered_map<uint16_t, std::u16string>());
    }

    m_bindings.at(key).insert_or_assign(modifier, args[2]);
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
    uint16_t result = 0;
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

    return result;
}
