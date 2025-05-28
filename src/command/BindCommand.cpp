#include "BindCommand.h"

#include <SDL_keyboard.h>
#include <utf8.h>

#include "../core/CommandManager.h"


const std::unordered_map<std::u16string, uint16_t> BindCommand::MODIFIER_MAP = {
    { u"Ctrl", KMOD_CTRL },
    { u"Shift", KMOD_SHIFT },
    { u"Alt", KMOD_ALT },
    { u"None", KMOD_NONE }
};

void BindCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    // TODO
}

std::optional<std::u16string> BindCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (args.size() < 3 || (!args.empty() && args[1].empty())) {
        return u"Usage: bind <modifiers> <key> <command>";
    }

    // Split the first argument which should be the modifier keys.
    const auto split_modifiers = CommandManager::split(args[0], u'+');

    // Normalize the modifiers, as the app does not make the difference between left and right.
    auto modifier = 0;
    for (const auto string_modifier : split_modifiers) {
        const auto mapped_modifier = mapModifier(string_modifier);
        if (mapped_modifier == -1) {
            return std::u16string(u"Unknown modifier: ").append(string_modifier);
        }
        modifier |= mapped_modifier;
    }

    // Check the keycode name and if we have it in the map.
    const auto keycode_utf8 = utf8::utf16to8(args[1]);
    const auto key = SDL_GetKeyFromName(keycode_utf8.data());
    if (! m_bindings.contains(key)) {
        // If the binding to this key does not exist yet, create a map for this key.
        m_bindings.emplace(key, std::unordered_map<uint16_t, std::u16string>());
    }

    // Insert the command "as-it".
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

    return result;
}

int32_t BindCommand::mapModifier(const std::u16string_view modifier) {
    // Need to convert back to a string, since this is originally a split string (with no \0).
    const auto modifier_str = std::u16string(modifier.begin(), modifier.end());
    if (const auto &mapped_modifier = MODIFIER_MAP.find(modifier_str); mapped_modifier != MODIFIER_MAP.end()) {
        return mapped_modifier->second;
    }

    return -1;
}
