#include "CVarCommand.h"

#include <SDL_keyboard.h>
#include <utf8.h>

#include "CursorContext.h"


void CVarCommand::registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    const auto c_string = name.data();
    if (m_cvars.contains(c_string)) {
        throw std::runtime_error(std::string("CVar already registered: ").append(name));
    }

    const auto &[new_entry, success] = m_cvars.insert({c_string, { std::move(cvar), callback } });
    if (!success) {
        throw std::runtime_error(std::string("Unable to register  CVar: ").append(name));
    }
}

void CVarCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
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

    const auto cvar_name = utf8::utf16to8(args[0]);
    if (!m_cvars.contains(cvar_name)) {
        return std::u16string(u"Unknown cvar: ").append(args[0]);
    }

    const auto &cvar_entry = m_cvars[cvar_name];
    auto *cvar = cvar_entry.cvar.get();

    if (args.size() == 1) {
        // Print the value of this cvar
        return std::u16string(args[0]).append(u": ").append(cvar->getStringValue());
    }

    // Se the value of this cvar if not read-only
    if (cvar->isReadOnly()) {
        return std::u16string(u"CVar is read-only: ").append(args[0]);
    }

    if (const auto &error = cvar->setValueFromStrings({args.begin() + 1, args.end()})) {
        // Something wrong happened
        return std::u16string(args[0]).append(u": ").append(error.value());
    }

    // Eventually invoke the associated callback
    if (const auto &callback = cvar_entry.callback) {
        callback();
    }

    return std::nullopt;
}
