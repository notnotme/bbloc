#ifndef CVAR_COMMAND_H
#define CVAR_COMMAND_H

#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

#include "base/CVar.h"
#include "base/CVarCallback.h"
#include "base/CVarRegistry.h"
#include "base/Command.h"
#include "CursorContext.h"


class CVarCommand final : public Command<CursorContext>, public CVarRegistry {
    /** @brief Internal structure representing a configuration variable entry. */
    struct CVarEntry final {
        std::shared_ptr<CVar> cvar; ///< Pointer to the CVar instance.
        CVarCallback callback;      ///< Callback executed when the CVar value is changed.
    };

    /** Registered configuration variables. */
    std::unordered_map<std::string, CVarEntry> m_cvars;

public:
    CVarCommand() = default;

    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
    void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) override;
};


#endif //CVAR_COMMAND_H
