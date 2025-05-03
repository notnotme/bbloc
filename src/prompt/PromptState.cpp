#include "PromptState.h"

#include <algorithm>
#include <utility>


PromptState::PromptState(std::shared_ptr<CVarInt> historyMaxSize)
    : m_prompt_text(PROMPT_READY),
      m_completion_index(0),
      m_command_history_index(-1),
      m_history_max_size(std::move(historyMaxSize)),
      m_running_state(RunningState::Idle) {
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
    if (!m_command_history.empty()) {
        while (m_command_history.size() - 1 > m_history_max_size->m_value) {
            // The history list is full, pop the front items.
            // Do it in a loop, so if the history is resized at runtime,
            // this will keep everything on track
            m_command_history.pop_front();
        }
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
