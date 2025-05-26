#ifndef BIND_COMMAND_H
#define BIND_COMMAND_H

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL_keycode.h>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for managing key bindings in the text editor.
 *
 * This class implements the Command interface for creating and managing keyboard shortcuts.
 * It allows binding specific key combinations to editor commands.
 */
class BindCommand final : public Command<CursorContext> {
private:
    /** Map of key binding to commands.
     *
     * The outer map uses SDL_Keycode as keys, and the inner map uses modifier combinations
     * (stored as uint16_t) to map to command strings.
     */
    std::unordered_map<SDL_Keycode, std::unordered_map<uint16_t, std::u16string>> m_bindings;

    /**
     * @brief Normalize input modifiers from raw SDL input modifiers.
     *
     * This basically converts SDL modifiers Left/Right to universal modifier position (LSHIFT -> SHIFT).
     *
     * @param modifiers The raw SDL modifier flags.
     * @return Normalized modifier flags.
     */
    static uint16_t normalizeModifiers(uint16_t modifiers);

public:
    /** @brief Constructs a BindCommand with default initialization. */
    explicit BindCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for the bind command arguments.
     *
     * TODO Suggests completions for keys, modifiers, and available commands.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the bind command to create a new key binding.
     *
     * Creates or updates a binding between a key combination and a command.
     * This expect 3 arguments:
     * - <modifiers> "+" separated
     * - <key> the Key name
     * - <command string> The command string, quoted if it contains spaces.
     *
     * @param payload The cursor context (not directly used for binding).
     * @param args Command arguments specifying the key, modifiers, and target command.
     * @return An optional message indicating the result of the binding operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Retrieves the command associated with a specific key combination.
     *
     * Looks up the command string bound to the given key and modifier combination.
     *
     * @param keycode The SDL keycode of the pressed key.
     * @param modifiers The modifier keys that were active.
     * @return The bound command string if found, or std::nullopt if no binding exists.
     */
    std::optional<std::u16string_view> getBinding(SDL_Keycode keycode, uint16_t modifiers);
};



#endif //BIND_COMMAND_H
