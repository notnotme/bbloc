#ifndef FEEDBACK_CALLBACK_H
#define FEEDBACK_CALLBACK_H

#include <functional>
#include <optional>
#include <string_view>
#include <vector>


/**
 * @brief Type alias for a feedback callback function.
 *
 * A FeedbackCallback is used to process or log the result of a command execution.
 * It receives both the answer/output and the original/modified command as UTF-16 string views.
 * The callback may return an optional modified message or feedback.
 *
 * @param answer The output or result of the executed command.
 * @param command The command input to be invoked next if the feedback succeeds.
 * @return An optional UTF-16 string message, such as modified output or additional feedback.
 */
using FeedbackCallback = std::function<
    std::optional<std::u16string>(std::u16string_view answer, std::u16string_view command)
>;


#endif //FEEDBACK_CALLBACK_H
