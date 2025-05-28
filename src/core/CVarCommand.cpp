#include "CVarCommand.h"

#include <SDL_keyboard.h>
#include <utf8.h>

#include "CursorContext.h"


void CVarCommand::registerCvar(std::u16string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    const auto name_str = std::u16string(name.begin(), name.end());
    if (m_cvars.contains(name_str)) {
        throw std::runtime_error(std::string("CVar already registered: ").append(utf8::utf16to8(name)));
    }

    const auto &[new_entry, success] = m_cvars.insert({name_str, { std::move(cvar), callback } });
    if (!success) {
        throw std::runtime_error(std::string("Unable to register  CVar: ").append(utf8::utf16to8(name)));
    }
}

void CVarCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex > 0) {
        // Only auto-complete the names of CVars
        return;
    }

    const auto input_is_empty = input.empty();
    for (const auto &name : std::views::keys(m_cvars)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

std::optional<std::u16string> CVarCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (args.empty()) {
        return u"Usage: cvar <name> [value1] [value2] ...";
    }

    const auto cvar_name_str = std::u16string(args[0].begin(), args[0].end());
    if (!m_cvars.contains(cvar_name_str)) {
        return std::u16string(u"Unknown cvar: ").append(cvar_name_str);
    }

    const auto &cvar_entry = m_cvars[cvar_name_str];
    auto *cvar = cvar_entry.cvar.get();

    if (args.size() == 1) {
        // Print the value of this cvar
        return std::u16string(cvar_name_str).append(u": ").append(cvar->getStringValue());
    }

    // Se the value of this cvar if not read-only
    if (cvar->isReadOnly()) {
        return std::u16string(u"CVar is read-only: ").append(cvar_name_str);
    }

    if (const auto &error = cvar->setValueFromStrings({ args.begin() + 1, args.end() })) {
        // Something wrong happened
        return std::u16string(cvar_name_str).append(u": ").append(error.value());
    }

    // Eventually invoke the associated callback
    if (const auto &callback = cvar_entry.callback) {
        callback();
    }

    return std::nullopt;
}
