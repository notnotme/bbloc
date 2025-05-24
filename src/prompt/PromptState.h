#ifndef PROMPT_STATE_H
#define PROMPT_STATE_H

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "../core/cvar/CVarInt.h"
#include "../core/CommandManager.h"
#include "../core/CursorContext.h"
#include "../core/ViewState.h"

/**
 * @brief Stores the layout, input state, and completions of the command prompt view.
 *
 * Tracks prompt state including active status, input text, autocompletions, and
 * command history navigation.
 */
class PromptState final : public ViewState {
public:
    /** @brief Represents the current operational state of the prompt. */
    enum class RunningState {
        Idle,       ///< Prompt is hidden or inactive.
        Running,    ///< Prompt is actively receiving user input.
        Validated,  ///< Input has been submitted and processed.
        Message     ///< Prompt displays an error or informational message.
    };

    /** @brief Default prompt label when idle. */
    static constexpr auto PROMPT_READY = u"Ready.";

    /** @brief Default prompt label when active. */
    static constexpr auto PROMPT_ACTIVE = u":";

    /** Amount of command history (can be changed at runtime). */
    static constexpr auto MAX_COMMAND_HISTORY = 32;

private:
    /** Reference to the command manager (used to register cvars and commands). */
    CommandManager &m_command_manager;

    /** Current label displayed on the prompt line. */
    std::u16string m_prompt_text;

    /** List of auto-completion suggestions. */
    std::vector<std::u16string> m_completions;

    /** Index of the currently selected completion. */
    int32_t m_completion_index;

    /** Command history (most recent last). */
    std::list<std::u16string> m_command_history;

    /** Index used to navigate through the command history. */
    int32_t m_command_history_index;

    /** CVar tracking the max history size of the prompt. */
    std::shared_ptr<CVarInt> m_max_history;

    /** Current operational state of the prompt. */
    RunningState m_running_state;

    /** @brief Registers the max history CVar, to control history size. */
    void registerMaxHistoryCVar();

public:
    /** @brief Deleted copy constructor. */
    PromptState(const PromptState &) = delete;

    /** @brief Deleted copy assignment operator. */
    PromptState &operator=(const PromptState &) = delete;

    /** @brief Constructs a PromptState with default values. */
    explicit PromptState(CommandManager &commandManager);

    /** @brief Gets the current prompt label text. */
    [[nodiscard]] std::u16string_view getPromptText() const;

    /** @brief Gets the currently selected auto-completion string. */
    [[nodiscard]] std::u16string_view getCurrentCompletion() const;

    /** @brief Moves to the next completion in the list and returns it. */
    [[nodiscard]] std::u16string_view nextCompletion();

    /** @brief Moves to the previous completion in the list and returns it. */
    [[nodiscard]] std::u16string_view previousCompletion();

    /** @brief Returns the number of available completions. */
    [[nodiscard]] int32_t getCompletionCount() const;

    /** @brief Gets the current completion index. */
    [[nodiscard]] int32_t getCompletionIndex() const;

    /** @brief Navigates to the next command in history. */
    [[nodiscard]] std::u16string_view nextHistory();

    /** @brief Navigates to the previous command in history. */
    [[nodiscard]] std::u16string_view previousHistory();

    /** @brief Returns the total number of stored commands. */
    [[nodiscard]] int32_t getHistoryCount() const;

    /** @brief Returns the current index in the history navigation. */
    [[nodiscard]] int32_t getHistoryIndex() const;

    /** @brief Checks whether the prompt is navigating command history. */
    [[nodiscard]] bool isNavigatingHistory() const;

    /** @brief Gets the current running state of the prompt. */
    [[nodiscard]] RunningState getRunningState() const;

    /**
     * @brief Sets the visible prompt label text.
     * @param text New label text.
     */
    void setPromptText(std::u16string_view text);

    /**
     * @brief Adds a new auto-completion candidate.
     * @param item Completion string to append.
     */
    void addCompletion(std::u16string_view item);

    /**
     * @brief Adds a command string to the history list.
     * @param command The command string to store.
     */
    void addHistory(std::u16string_view command);

    /** @brief Resets history navigation index to default. */
    void clearHistoryIndex();

    /** @brief Removes all auto-completion candidates. */
    void clearCompletions();

    /** @brief Sorts the completion list alphabetically. */
    void sortCompletions();

    /**
     * @brief Updates the prompt's operational state.
     * @param state New running state to apply.
     */
    void setRunningState(RunningState state);
};


#endif //PROMPT_STATE_H
