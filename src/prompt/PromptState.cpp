#include "PromptState.h"

#include <algorithm>
#include <utility>
#include <oneapi/tbb/task_group.h>


PromptState::PromptState(CommandManager& commandManager)
    : m_command_manager(commandManager),
      m_prompt_text(PROMPT_READY),
      m_completion_index(0),
      m_command_history_index(-1),
      m_max_history(std::make_shared<CVarInt>(MAX_COMMAND_HISTORY)),
      m_running_state(RunningState::Idle) {
    // Register cvars
    registerMaxHistoryCVar();
}

std::u16string_view PromptState::getPromptText() const {
    return m_prompt_text;
}

std::u16string_view PromptState::getCurrentCompletion() const {
    return m_completions.empty() ? std::u16string_view{} : m_completions[m_completion_index];
}

int32_t PromptState::getCompletionCount() const {
    return static_cast<int32_t>(m_completions.size());
}

int32_t PromptState::getCompletionIndex() const {
    return m_completion_index;
}

std::u16string_view PromptState::nextHistory() {
    if (m_command_history_index < 0) {
        // Always return the last item if the index was invalidated
        m_command_history_index = static_cast<int32_t>(m_command_history.size()) - 1;
    } else {
        const auto max_size = static_cast<int32_t>(m_command_history.size());
        m_command_history_index = (m_command_history_index + 1) % max_size;
    }
    return *std::next(m_command_history.begin(), m_command_history_index);
}

std::u16string_view PromptState::previousHistory() {
    if (m_command_history_index < 0) {
        // Always return the last item if the index was invalidated
        m_command_history_index = static_cast<int32_t>(m_command_history.size()) - 1;
    } else {
        const auto max_size = static_cast<int32_t>(m_command_history.size());
        m_command_history_index = ((m_command_history_index-1) % max_size + max_size ) % max_size;
    }
    return *std::next(m_command_history.begin(), m_command_history_index);
}

int32_t PromptState::getHistoryCount() const {
    return static_cast<int32_t>(m_command_history.size());
}

int32_t PromptState::getHistoryIndex() const {
    return m_command_history_index;
}

bool PromptState::isNavigatingHistory() const {
    return m_command_history_index >= 0;
}

PromptState::RunningState PromptState::getRunningState() const {
    return m_running_state;
}

void PromptState::setPromptText(const std::u16string_view text) {
    m_prompt_text = text;
}

void PromptState::addCompletion(const std::u16string_view item) {
    m_completions.insert(m_completions.end(), item.data());
}

void PromptState::addHistory(const std::u16string_view command) {
    if (!m_command_history.empty() && m_command_history.size() - 1 > m_max_history->m_value) {
        // The history list is full, pop the front items.
        m_command_history.pop_front();
    }

    // Does not filter duplicate commands because it can be useful at some point
    m_command_history.insert(m_command_history.end(), command.data());
    m_command_history_index = -1;
}

void PromptState::clearHistoryIndex() {
    m_command_history_index = -1;
}

void PromptState::clearCompletions() {
    m_completions.clear();
    m_completion_index = 0;
}

void PromptState::sortCompletions() {
    std::ranges::sort(m_completions);
    m_completion_index = 0;
}

std::u16string_view PromptState::nextCompletion() {
    const auto max_size = static_cast<int32_t>(m_completions.size());
    m_completion_index = (m_completion_index + 1) % max_size;
    return m_completions[m_completion_index];
}

std::u16string_view PromptState::previousCompletion() {
    const auto max_size = static_cast<int32_t>(m_completions.size());
    m_completion_index = ((m_completion_index-1) % max_size + max_size ) % max_size;
    return m_completions[m_completion_index];
}

void PromptState::setRunningState(const RunningState state) {
    m_running_state = state;
}

void PromptState::registerMaxHistoryCVar() {
    // register the max_history cvar
    m_command_manager.registerCvar("dim_max_history", m_max_history, [&] {
        // Clamp history so the user cannot enter funny numbers
        const auto new_size = std::clamp(m_max_history->m_value, 8, 255);
        m_max_history->m_value = new_size;
        if (!m_command_history.empty() && m_command_history.size() > new_size) {
            // Resize the container if needed
            m_command_history.resize(new_size);
        }
    });
}
