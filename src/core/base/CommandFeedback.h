#ifndef COMMAND_FEEDBACK_H
#define COMMAND_FEEDBACK_H

#include <string>
#include <vector>

#include "FeedbackCallback.h"


/** @brief Structure for managing command feedback interactions (e.g., confirmation prompts). */
struct CommandFeedback final {
    std::u16string prompt_message;                  ///< Prompt message displayed to the user.
    std::u16string command_string;                  ///< Command associated with the feedback.
    std::vector<std::u16string> completions_list;   ///< List of valid input options (e.g., "y", "n").
    FeedbackCallback on_validate_callback;          ///< Callback to run after receiving user input.
};


#endif //COMMAND_FEEDBACK_H
