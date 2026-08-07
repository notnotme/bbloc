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
#include <utility>


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

std::size_t UndoHistory::weigh(const Group &group) {
    auto characters = std::size_t{0};
    for (const auto &edit : group.edits) {
        characters += edit.removed.length() + edit.inserted.length();
    }
    return characters;
}

void UndoHistory::shareMaxDepth(std::shared_ptr<CVarInt> maxDepth) {
    m_max_undo = std::move(maxDepth);
    applyMaxDepth();
}

void UndoHistory::dropOldest(std::deque<Group> &stack) {
    m_retained_characters -= weigh(stack.front());
    stack.pop_front();
}

void UndoHistory::clearRedo() {
    for (const auto &group : m_redo_stack) {
        m_retained_characters -= weigh(group);
    }
    m_redo_stack.clear();
}

void UndoHistory::trim() {
    // Count cap: each stack keeps at most the live capacity, oldest first out
    while (m_undo_stack.size() > capacity()) {
        dropOldest(m_undo_stack);
    }

    while (m_redo_stack.size() > capacity()) {
        dropOldest(m_redo_stack);
    }

    // Character cap: an edit weighs what it replaced, so this is now reached by a genuinely long
    // editing session rather than by a handful of steps on a large file. Drop the oldest history
    // first, but never the newest group of either stack: undo() and redo() hand out a pointer into
    // one of them and this runs while that pointer is still live.
    while (m_retained_characters > MAX_HISTORY_CHARACTERS) {
        if (m_undo_stack.size() > 1) {
            dropOldest(m_undo_stack);
        } else if (m_redo_stack.size() > 1) {
            dropOldest(m_redo_stack);
        } else {
            // One group per stack left and still over budget: keep them, undo must stay usable
            break;
        }
    }
}

void UndoHistory::applyMaxDepth() {
    trim();
}

bool UndoHistory::coalesce(Group &group, const Edit &edit) {
    auto &previous = group.edits.back();

    if (previous.removed.empty() && edit.removed.empty()) {
        // Typing: the new text starts exactly where the previous insert ended
        if (advancePosition(previous.start, previous.inserted).line == edit.start.line
            && advancePosition(previous.start, previous.inserted).column == edit.start.column) {
            previous.inserted.append(edit.inserted);
            return true;
        }
        return false;
    }

    if (previous.inserted.empty() && edit.inserted.empty()) {
        // Backspacing: the new erase ends exactly where the previous one began, so it belongs
        // in front of it — the text has to come back in reading order, not in typing order
        const auto edit_end = advancePosition(edit.start, edit.removed);
        if (edit_end.line == previous.start.line && edit_end.column == previous.start.column) {
            previous.removed.insert(0, edit.removed);
            previous.start = edit.start;
            return true;
        }

        // Deleting forward: the caret never moves, so both erases start at the same place
        if (edit.start.line == previous.start.line && edit.start.column == previous.start.column) {
            previous.removed.append(edit.removed);
            return true;
        }
    }

    return false;
}

void UndoHistory::record(Edit edit, const BufferEdit::Position &cursorBefore, const BufferEdit::Position &cursorAfter) {
    // An edit is coming: whatever could be redone is now unreachable
    clearRedo();

    if (edit.removed.empty() && edit.inserted.empty()) {
        // Replacing nothing with nothing leaves no trace to undo, but it still closes the
        // boundary the way any other edit would
        m_at_boundary = false;
        return;
    }

    if (m_at_boundary || m_undo_stack.empty()) {
        m_undo_stack.emplace_back(Group{.edits = {}, .cursor_before = cursorBefore, .cursor_after = cursorAfter});
        m_at_boundary = false;
    }

    auto &group = m_undo_stack.back();
    m_retained_characters += edit.removed.length() + edit.inserted.length();
    if (group.edits.empty() || !coalesce(group, edit)) {
        group.edits.emplace_back(std::move(edit));
    }
    group.cursor_after = cursorAfter;

    trim();
}

const UndoHistory::Group *UndoHistory::undo() {
    if (m_undo_stack.empty()) {
        return nullptr;
    }

    // The group only changes stacks, so the retained total is unchanged
    m_redo_stack.emplace_back(std::move(m_undo_stack.back()));
    m_undo_stack.pop_back();
    trim();

    return &m_redo_stack.back();
}

const UndoHistory::Group *UndoHistory::redo() {
    if (m_redo_stack.empty()) {
        return nullptr;
    }

    m_undo_stack.emplace_back(std::move(m_redo_stack.back()));
    m_redo_stack.pop_back();
    trim();

    return &m_undo_stack.back();
}

void UndoHistory::clear() {
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_retained_characters = 0;
    m_at_boundary = true;
}
