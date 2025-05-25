#ifndef COMMAND_H
#define COMMAND_H

#include <optional>
#include <string>
#include <vector>

#include "AutoCompleteCallback.h"


/**
 * @brief Abstract base class for all console commands in the text editor.
 *
 * This templated class provides the interface for defining custom commands that
 * can be executed with a specific payload and optional arguments. It also supports
 * command-line auto-completion and runtime validation.
 *
 * @tparam TPayload The type of the payload passed to the command at execution.
 */
template <typename TPayload>
class Command {
public:
    /** @brief Deleted copy constructor. */
    Command(const Command &) = delete;

    /** @brief Deleted copy assignment operator. */
    Command &operator=(const Command &) = delete;

    /** @brief Constructs the Command with default values. */
    explicit Command() = default;

    /** @brief Virtual destructor for inheritance */
    virtual ~Command() = default;

    /**
     * @brief Checks whether the command is allowed to run with the given payload.
     * Override this in derived classes if specific constraints are needed. Returns true by default.
     * @param payload The payload to test.
     * @return true if the command can be run.
     */
    [[nodiscard]] virtual bool isRunnable(const TPayload &payload);

    /**
     * @brief Executes the command using the provided payload and arguments.
     * @param payload The payload to use for running this command.
     * @param args The arguments to pass to this command.
     * @return An optional informative or error message.
     */
    [[nodiscard]] virtual std::optional<std::u16string> run(TPayload &payload, const std::vector<std::u16string_view> &args) = 0;

    /**
     * @brief Command-line completion function used to provide completion suggestions for command arguments.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    virtual void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const = 0;
};

template<class TPayload>
bool Command<TPayload>::isRunnable(const TPayload &payload) {
    // By default, an action is not restricted to run.
    return true;
}


#endif //COMMAND_H
