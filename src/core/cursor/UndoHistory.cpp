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
#include "UndoHistory.h"

#include <algorithm>


UndoHistory::UndoHistory()
    : m_at_boundary(true) {}

void UndoHistory::markBoundary() {
    m_at_boundary = true;
}

uint32_t UndoHistory::capacity() const {
    if (m_max_undo) {
        return static_cast<uint32_t>(std::max(1, m_max_undo->m_value));
    }

    return DEFAULT_MAX_HISTORY_DEPTH;
}

void UndoHistory::shareMaxDepth(std::shared_ptr<CVarInt> maxDepth) {
    m_max_undo = std::move(maxDepth);
    applyMaxDepth();
}

void UndoHistory::applyMaxDepth() {
    while (m_undo_stack.size() > capacity()) {
        m_undo_stack.pop_front();
    }

    while (m_redo_stack.size() > capacity()) {
        m_redo_stack.pop_front();
    }
}

bool UndoHistory::isAtBoundary() const {
    return m_at_boundary;
}

void UndoHistory::push(Snapshot snapshot) {
    m_redo_stack.clear();
    m_undo_stack.emplace_back(std::move(snapshot));
    while (m_undo_stack.size() > capacity()) {
        m_undo_stack.pop_front();
    }

    m_at_boundary = false;
}

std::optional<UndoHistory::Snapshot> UndoHistory::undo(Snapshot current) {
    if (m_undo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();

    m_redo_stack.emplace_back(std::move(current));
    while (m_redo_stack.size() > capacity()) {
        m_redo_stack.pop_front();
    }

    return snapshot;
}

std::optional<UndoHistory::Snapshot> UndoHistory::redo(Snapshot current) {
    if (m_redo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();

    m_undo_stack.emplace_back(std::move(current));
    while (m_undo_stack.size() > capacity()) {
        m_undo_stack.pop_front();
    }

    return snapshot;
}

void UndoHistory::clear() {
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_at_boundary = true;
}
