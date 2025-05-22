#ifndef COMMAND_CALLBACK_H
#define COMMAND_CALLBACK_H

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "../CursorContext.h"


/**
 * @brief Type alias for a command callback function.
 *
 * A CommandCallback is a function that takes a CursorContext reference and a list of UTF-16 string arguments.
 * It performs a command-related operation and may optionally return a UTF-16 string message (e.g., for errors or output).
 *
 * @param context Reference to the cursor context executing this command.
 * @param args List of arguments parsed from user input, represented as UTF-16 string views.
 * @return An optional UTF-16 string message (e.g., success/failure message, output).
 */
using CommandCallback = std::function<
    std::optional<std::u16string>(CursorContext &context, const std::vector<std::u16string_view> &args)
>;


#endif //COMMAND_CALLBACK_H
