/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CursorContextManager.h"

#include <algorithm>

#include "cursor/buffer/LineBuffer.h"


CursorContextManager::CursorContextManager(CommandRunner &commandRunner, Theme &theme, PromptCursor &promptCursor, std::shared_ptr<CVarInt> maxUndo)
    : m_command_runner(commandRunner),
      m_theme(theme),
      m_prompt_cursor(promptCursor),
      m_max_undo(std::move(maxUndo)),
      m_active_index(0) {
    // The manager guarantees one context always exists: create the startup scratch now.
    createContext();
}

CursorContext &CursorContextManager::active() {
    return *m_contexts[m_active_index];
}

CursorContext &CursorContextManager::get(const size_t index) {
    return *m_contexts[index];
}

size_t CursorContextManager::getActiveIndex() const {
    return m_active_index;
}

size_t CursorContextManager::getCount() const {
    return m_contexts.size();
}

std::unique_ptr<CursorContext> CursorContextManager::makeContext() const {
    auto context = std::make_unique<CursorContext>(m_command_runner, m_theme, m_prompt_cursor, std::make_unique<LineBuffer>());

    // Every cursor shares the same history depth CVar, so dim_max_undo applies globally.
    context->cursor.shareMaxHistoryDepth(m_max_undo);
    context->cursor.setMaxHistoryDepth();
    return context;
}

CursorContext &CursorContextManager::createContext() {
    auto &context = m_contexts.emplace_back(makeContext());

    // The count changed: the active view must redraw its buffer position indicator.
    refreshIndices();
    active().wants_redraw = true;
    return *context;
}

void CursorContextManager::activate(const size_t index) {
    if (index >= m_contexts.size()) {
        return;
    }

    // A stale interactive prompt must not survive a switch: the next prompt
    // input would be consumed as its answer against another buffer.
    active().command_feedback.reset();
    m_active_index = index;
    active().wants_redraw = true;
}

void CursorContextManager::next() {
    activate((m_active_index + 1) % m_contexts.size());
}

void CursorContextManager::prev() {
    activate((m_active_index + m_contexts.size() - 1) % m_contexts.size());
}

bool CursorContextManager::close() {
    // The pending feedback of the closed context dies with it.
    m_contexts.erase(m_contexts.begin() + static_cast<ptrdiff_t>(m_active_index));
    const auto had_others = !m_contexts.empty();
    if (!had_others) {
        // The last context was closed: replace it with a fresh scratch so one always exists.
        m_contexts.emplace_back(makeContext());
    }

    // The next buffer takes the closed one's place, or the new last one when the end was closed.
    m_active_index = std::min(m_active_index, m_contexts.size() - 1);
    refreshIndices();
    active().wants_redraw = true;
    return had_others;
}

std::optional<size_t> CursorContextManager::indexOf(const std::string_view name) const {
    for (size_t index = 0; index < m_contexts.size(); ++index) {
        if (m_contexts[index]->cursor.getName() == name) {
            return index;
        }
    }

    return std::nullopt;
}

void CursorContextManager::applyMaxHistoryDepth() {
    for (const auto &context : m_contexts) {
        context->cursor.setMaxHistoryDepth();
    }
}

void CursorContextManager::refreshIndices() {
    for (size_t index = 0; index < m_contexts.size(); ++index) {
        m_contexts[index]->buffer_index = index + 1;
        m_contexts[index]->buffer_count = m_contexts.size();
    }
}
