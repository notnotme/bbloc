#include "AutoCompleteCommand.h"

#include "../core/CommandManager.h"
#include "../core/FocusTarget.h"


const std::unordered_map<std::u16string, AutoCompleteCommand::Direction> AutoCompleteCommand::DIRECTION_MAP = {
    { u"forward", Direction::FORWARD },
    { u"backward", Direction::BACKWARD }
};

AutoCompleteCommand::AutoCompleteCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void AutoCompleteCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> AutoCompleteCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() > 1) {
        return u"Expected 0 or 1 argument.";
    }

    // Determine which direction to use for the completion
    const auto direction = !args.empty()
        // User decide.
        ? mapDirection(args[0])
        // Goes forward by default.
        : Direction::FORWARD;

    if (direction == Direction::UNKNOWN) {
        // Return an error if we can't determine the direction
        return std::u16string(u"Unknown direction argument: ").append(args[0]);
    }

    // Get and tokenize the input string
    const auto input = payload.prompt_cursor.getString();
    const auto tokens = CommandManager::tokenize(input);

    // Reset the history index if we were browsing it
    m_prompt_state.clearHistoryIndex();
    if (m_prompt_state.getCompletionCount() > 0) {
        // The viewState completion list is not empty, loop inside
        const auto completion = direction == Direction::FORWARD
            ? m_prompt_state.nextCompletion()
            : m_prompt_state.previousCompletion();

        payload.prompt_cursor.clear();
        payload.prompt_cursor.insert(completion);
        payload.wants_redraw = true;
        return std::nullopt;
    }

    if (payload.command_feedback) {
        // If feedback is active, try to gather arguments
        for (const auto &item : payload.command_feedback->completions_list) {
            m_prompt_state.addCompletion(item);
        }
    } else {
        // Find command name, argument to complete, and argument index from the user input
        const auto command_name = tokens.empty() ? u"" : tokens.front();
        const auto argument_to_complete = tokens.size() <= 1 ? u"" : tokens.back();
        const auto argument_index = static_cast<int32_t>(tokens.size() <= 1
            // Zero or one token = command argument
            ? tokens.size() - 1
            : input.ends_with(' ')
                // If the input ends with ' ', argument index is tokens.size() - 1
                ? tokens.size() - 1
                // Otherwise, argument index is tokens.size() - 2
                : tokens.size() - 2);

        // Reconstitute the original input, first find the left part of the input.
        auto skip = 2;
        auto left_index = input.rfind(u" \"");
        if (left_index == std::string::npos) {
            // Take the latest argument with quotes, or fallback to a space.
            left_index = input.rfind(' ');
            skip = 1;
        }

        const auto reconstituted_command = left_index == std::string::npos
            // Nothing was found, we return the entire input + space.
            ? std::u16string(input).append(u" ")
            // Return the left part of the input, before the eventually incomplete argument.
            : input.substr(0, left_index + skip);

        // Try to complete commands arguments first, if the command name is incomplete, this will return an empty list
        payload.command_runner.getArgumentsCompletions(command_name, argument_index, argument_to_complete,
            [&](const std::u16string_view completion) {
                // Create a copy of the original strings
                const auto completion_str = std::u16string(reconstituted_command).append(completion);
                m_prompt_state.addCompletion(completion_str);
            });

        if (m_prompt_state.getCompletionCount() == 0 && tokens.size() <= 1) {
            // Auto-complete commands names then
            payload.command_runner.getCommandCompletions(command_name,
                [&](const std::u16string_view completion) {
                    m_prompt_state.addCompletion(completion);
                });
        }
    }

    // Populate the viewState list and insert the first item in the cursor
    const auto completion_count = m_prompt_state.getCompletionCount();
    if (completion_count > 0) {
        m_prompt_state.sortCompletions();

        const auto completion = m_prompt_state.getCurrentCompletion();
        payload.prompt_cursor.clear();
        payload.prompt_cursor.insert(completion);

        if (completion_count == 1) {
            // If we got only a single result, then append a space at the end of the command line
            // and clear the actual completion, so the next argument completion can occur.
            payload.prompt_cursor.insert(u" ");
            m_prompt_state.clearCompletions();
        }

        payload.wants_redraw = true;
    }

    return std::nullopt;
}

bool AutoCompleteCommand::isRunnable(const CursorContext &payload) {
    // Auto complete is only available from the prompt perspective
    return payload.focus_target == FocusTarget::Prompt;
}

AutoCompleteCommand::Direction AutoCompleteCommand::mapDirection(const std::u16string_view direction) {
    const auto direction_str = std::u16string(direction.begin(), direction.end());
    if (const auto &mapped_direction = DIRECTION_MAP.find(direction_str); mapped_direction != DIRECTION_MAP.end()) {
        return mapped_direction->second;
    }

    return Direction::UNKNOWN;
}
