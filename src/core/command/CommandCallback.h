#ifndef COMMAND_CALLBACK_H
#define COMMAND_CALLBACK_H

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "../cursor/Cursor.h"


/**
 * @brief Type alias for a command callback function.
 *
 * A CommandCallback is a function that takes a Cursor reference and a list of UTF-16 string arguments.
 * It performs a command-related operation and may optionally return a UTF-16 string message (e.g., for errors or output).
 *
 * @param cursor Reference to the cursor or context in which the command operates.
 * @param args List of arguments parsed from user input, represented as UTF-16 string views.
 * @return An optional UTF-16 string message (e.g., success/failure message, output).
 */
using CommandCallback = std::function<
    std::optional<std::u16string>(Cursor& cursor,const std::vector<std::u16string_view>& args)
>;


#endif //COMMAND_CALLBACK_H
