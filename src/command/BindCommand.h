#ifndef BIND_COMMAND_H
#define BIND_COMMAND_H

#include <SDL_keycode.h>

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class BindCommand final : public Command<CursorContext> {
private:
    /** Map of key binding to commands */
    std::unordered_map<SDL_Keycode, std::unordered_map<uint16_t, std::u16string>> m_bindings;

    /** @brief Normalize input modifiers from raw sdl input modifiers*/
    static uint16_t normalizeModifiers(uint16_t modifiers);

public:
    explicit BindCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    std::optional<std::u16string_view> getBinding(SDL_Keycode keycode, uint16_t modifiers);
};



#endif //BIND_COMMAND_H
