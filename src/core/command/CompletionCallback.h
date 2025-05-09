#ifndef COMPLETION_CALLBACK_H
#define COMPLETION_CALLBACK_H

#include <cstdint>
#include <functional>
#include <string_view>

#include "ItemCallback.h"


/**
 * @brief Type alias for a command-line completion callback function.
 *
 * A CompletionCallback is used to provide completion suggestions for command arguments.
 * It is typically invoked during user input to suggest possible completions based on the current input.
 *
 * @param argumentIndex The index of the argument currently being completed.
 * @param input The current partial input from the user for this argument.
 * @param itemCallback A callback to be invoked with each completion suggestion.
 */
using CompletionCallback = std::function<
    void(int32_t argumentIndex, std::string_view input, const ItemCallback<char> &itemCallback)
>;


#endif //COMPLETION_CALLBACK_H
