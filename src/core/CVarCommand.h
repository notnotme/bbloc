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


/**
 * @brief Command that handles reading and modifying configuration variables (CVars).
 *
 * This class allows CVars to be manipulated as tt also functions as a CVar registry, holding references and change
 * callbacks for all registered CVars.
 */
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

    /**
     * @brief Provides auto-completion suggestions for CVar command arguments.
     *
     * @param argumentIndex Index of the argument being completed.
     * @param input Partial user input string.
     * @param itemCallback Callback to provide suggestions.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the CVar command.
     *
     * Can be used to get or set values of registered CVars via command-line.
     *
     * @param payload An instance of CursorContext.
     * @param args Arguments passed to the command.
     * @return Optional message indicating result or error.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Registers a new configuration variable.
     *
     * Associates a name with a CVar instance and optional modification callback.
     *
     * @param name Name of the variable. Must be unique.
     * @param cvar Shared pointer to the CVar.
     * @param callback Function to call on CVar change. Can be nullptr.
     */
    void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) override;
};


#endif //CVAR_COMMAND_H
