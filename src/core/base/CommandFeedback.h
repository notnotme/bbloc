#ifndef COMMAND_FEEDBACK_H
#define COMMAND_FEEDBACK_H

#include <string>
#include <vector>

#include "FeedbackCallback.h"


/**
 * @brief Represents a feedback interaction triggered by a command.
 *
 * Used when a command requires user confirmation or additional input after initial execution
 * (e.g., "Are you sure? [y/n]"). This structure holds the prompt message, expected completions,
 * the command string to run next, and a callback to handle the user's input.
 */
struct CommandFeedback final {
    std::u16string prompt_message;                  ///< Prompt message displayed to the user.
    std::u16string command_string;                  ///< Command associated with the feedback.
    std::vector<std::u16string> completions_list;   ///< List of valid input options (e.g., "y", "n").
    FeedbackCallback on_validate_callback;          ///< Callback to run after receiving user input.
};


#endif //COMMAND_FEEDBACK_H
