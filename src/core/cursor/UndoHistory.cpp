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
    : m_retained_characters(0),
      m_at_boundary(true) {}

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

void UndoHistory::dropOldest(std::deque<Snapshot> &stack) {
    m_retained_characters -= stack.front().text.length();
    stack.pop_front();
}

void UndoHistory::trim() {
    // Count cap: each stack keeps at most the live capacity, oldest first out
    while (m_undo_stack.size() > capacity()) {
        dropOldest(m_undo_stack);
    }

    while (m_redo_stack.size() > capacity()) {
        dropOldest(m_redo_stack);
    }

    // Character cap: a snapshot weighs the whole buffer, so a handful of them on a large file is
    // enough to exhaust memory. Drop the oldest history first, but never the last snapshot left.
    while (m_retained_characters > MAX_HISTORY_CHARACTERS) {
        if (m_undo_stack.size() > 1) {
            dropOldest(m_undo_stack);
        } else if (!m_redo_stack.empty()) {
            dropOldest(m_redo_stack);
        } else {
            // One snapshot bigger than the whole budget: keep it, undo must stay usable
            break;
        }
    }
}

void UndoHistory::applyMaxDepth() {
    trim();
}

bool UndoHistory::isAtBoundary() const {
    return m_at_boundary;
}

bool UndoHistory::canUndo() const {
    return !m_undo_stack.empty();
}

bool UndoHistory::canRedo() const {
    return !m_redo_stack.empty();
}

void UndoHistory::push(Snapshot snapshot) {
    // An edit is coming: whatever could be redone is now unreachable
    for (const auto &redo_snapshot : m_redo_stack) {
        m_retained_characters -= redo_snapshot.text.length();
    }
    m_redo_stack.clear();

    if (!m_undo_stack.empty() && m_undo_stack.back().text == snapshot.text) {
        // Same text as the snapshot on top: undo would restore the very same buffer, so keep the
        // older one (its cursor position is the one to restore) instead of retaining a second copy
        m_at_boundary = false;
        return;
    }

    m_retained_characters += snapshot.text.length();
    m_undo_stack.emplace_back(std::move(snapshot));
    trim();

    m_at_boundary = false;
}

std::optional<UndoHistory::Snapshot> UndoHistory::undo(Snapshot current) {
    if (m_undo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_undo_stack.back());
    m_retained_characters -= snapshot.text.length();
    m_undo_stack.pop_back();

    m_retained_characters += current.text.length();
    m_redo_stack.emplace_back(std::move(current));
    trim();

    return snapshot;
}

std::optional<UndoHistory::Snapshot> UndoHistory::redo(Snapshot current) {
    if (m_redo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_redo_stack.back());
    m_retained_characters -= snapshot.text.length();
    m_redo_stack.pop_back();

    m_retained_characters += current.text.length();
    m_undo_stack.emplace_back(std::move(current));
    trim();

    return snapshot;
}

void UndoHistory::clear() {
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_retained_characters = 0;
    m_at_boundary = true;
}
