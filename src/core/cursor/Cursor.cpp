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
#include "Cursor.h"

#include <algorithm>

#include "SurrogatePair.h"


/**
 * @brief Applies a stored replacement to the buffer and describes it as one edit.
 *
 * The range to remove is the stored text turned back into a span, so both halves of an undo step
 * are exact: nothing is diffed and no boundary is ever computed, which is why neither side can
 * land inside a surrogate pair. Each half degenerates to a no-op at start when its text is empty.
 *
 * @param buffer The buffer to rewrite.
 * @param start The position the replacement begins at.
 * @param remove The text currently occupying the range, removed by this call.
 * @param insert The text to put in its place.
 * @return A BufferEdit covering the replacement, for the incremental re-parse.
 */
static BufferEdit replaceRange(TextBuffer &buffer, const BufferEdit::Position &start, const std::u16string_view remove, const std::u16string_view insert) {
    const auto end = advancePosition(start, remove);

    // The two halves carry the offsets and the points of the combined edit: the erase leaves the
    // start of the range where it was, so the insert that follows starts at the very same offset.
    const auto &erase_edit = buffer.erase(start.line, start.column, end.line, end.column);
    const auto &insert_edit = buffer.insert(start.line, start.column, insert);

    return BufferEdit{
        .start_byte = erase_edit.start_byte,
        .old_end_byte = erase_edit.old_end_byte,
        .new_end_byte = insert_edit.new_end_byte,
        .start = erase_edit.start,
        .old_end = erase_edit.old_end,
        .new_end = insert_edit.new_end
    };
}

Cursor::Cursor(std::unique_ptr<TextBuffer> buffer)
    : m_buffer(std::move(buffer)),
      m_column(0),
      m_line(0),
      m_is_selection_active(false),
      m_selected_line_start(0),
      m_selected_column_start(0) {}

void Cursor::pageUp(const uint32_t lineCount) {
    m_history.markBoundary();

    // Don't go before 0
    if (m_line > lineCount) {
        m_line -= lineCount;
    } else {
        m_line = 0;
    }

    const auto cursor_string_length = static_cast<uint32_t>(m_buffer->getString(m_line).length());
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
    m_column = snapColumnOnLine(m_line, m_column);
}

void Cursor::pageDown(const uint32_t lineCount) {
    m_history.markBoundary();

    // Don't go after the end
    const auto cursor_line_count = m_buffer->getStringCount() - 1;
    const auto cursor_new_line = m_line + lineCount;
    if (cursor_new_line > cursor_line_count) {
        m_line = cursor_line_count;
    } else {
        m_line = cursor_new_line;
    }

    const auto cursor_string_length = static_cast<uint32_t>(m_buffer->getString(m_line).length());
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
    m_column = snapColumnOnLine(m_line, m_column);
}

void Cursor::setName(const std::string_view name) {
    m_name = name;
}

std::string_view Cursor::getName() const {
    return m_name;
}

bool Cursor::isModified() const {
    // Derived rather than latched: undoing back to the saved state answers false again
    return !m_history.isSaved();
}

void Cursor::setModified(const bool modified) {
    if (modified) {
        m_history.markUnsaved();
    } else {
        m_history.markSaved();
    }
}

uint32_t Cursor::getColumn() const {
    return m_column;
}

uint32_t Cursor::getLine() const {
    return m_line;
}

std::optional<TextRange> Cursor::getSelectedRange() const {
    if (!m_is_selection_active) {
        return std::nullopt;
    }

    auto start_line = m_selected_line_start;
    auto start_column = m_selected_column_start;
    auto end_line = m_line;
    auto end_column = m_column;

    if (start_line > end_line) {
        // Invert coordinates totally
        std::swap(start_line, end_line);
        std::swap(start_column, end_column);
    } else if (start_line == end_line && start_column > end_column) {
        // Invert column coordinates
        std::swap(start_column, end_column);
    } else if (start_line == end_line && start_column == end_column) {
        return std::nullopt;
    }

    return TextRange {
        .line_start = start_line,
        .column_start = start_column,
        .line_end = end_line,
        .column_end = end_column
    };
}

std::u16string_view Cursor::getString(const uint32_t line) const {
    return m_buffer->getString(line);
}

uint32_t Cursor::getLineCount() const {
    return m_buffer->getStringCount();
}

uint32_t Cursor::getLongestLineLength(const uint32_t tabWeight) const {
    return m_buffer->getLongestLineLength(tabWeight);
}

uint32_t Cursor::getLineTabCount(const uint32_t line) const {
    return m_buffer->getLineTabCount(line);
}

std::u16string_view Cursor::getString() const {
    return m_buffer->getString(m_line);
}

std::optional<std::vector<std::u16string_view>> Cursor::getSelectedText() const {
    const auto &range = getSelectedRange();
    if (!range) {
        return std::nullopt;
    }

    if (range->line_start == range->line_end) {
        // Results fit in one line
        return std::vector { m_buffer->getString(range->line_start).substr(range->column_start, range->column_end - range->column_start) };
    }

    auto result = std::vector<std::u16string_view>{};
    for (auto line = range->line_start; line <= range->line_end; ++line) {
        if (line == range->line_start) {
            result.emplace_back(m_buffer->getString(line).substr(range->column_start));
        } else if (line == range->line_end) {
            result.emplace_back(m_buffer->getString(line).substr(0, range->column_end));
        } else {
            result.emplace_back(m_buffer->getString(line));
        }
    }
    return result;
}

void Cursor::moveLeft() {
    m_history.markBoundary();

    if (m_column == 0) {
        // At the very beginning of a line, the cursor can't go left
        if (m_line > 0) {
            // The cursor can go above instead
            m_column = static_cast<uint32_t>(m_buffer->getString(m_line - 1).length());
            --m_line;
        }
    } else {
        m_column -= charLengthBefore(m_buffer->getString(m_line), m_column);
    }
}

void Cursor::moveRight() {
    m_history.markBoundary();

    if (m_column == m_buffer->getString(m_line).length()) {
        // At the very end of a line, the cursor can't go right
        if (m_line < m_buffer->getStringCount() - 1) {
            // The cursor can go below instead
            m_column = 0;
            ++m_line;
        }
    } else {
        m_column += charLengthAfter(m_buffer->getString(m_line), m_column);
    }
}

void Cursor::moveUp() {
    m_history.markBoundary();

    if (m_line > 0) {
        const auto string_above_length = static_cast<uint32_t>(m_buffer->getString(m_line - 1).length());
        if (m_column > string_above_length) {
            m_column = string_above_length;
        }
        m_column = snapColumnOnLine(m_line - 1, m_column);
        --m_line;
    } else {
        m_column = 0;
    }
}

void Cursor::moveDown() {
    m_history.markBoundary();

    if (m_line < m_buffer->getStringCount() - 1) {
        const auto string_below_length = static_cast<uint32_t>(m_buffer->getString(m_line + 1).length());
        if (m_column > string_below_length) {
            // The cursor can't stay at the same X position, put it at the end of the next line
            m_column = string_below_length;
        }
        m_column = snapColumnOnLine(m_line + 1, m_column);
        ++m_line;
    } else {
        m_column = static_cast<uint32_t>(m_buffer->getString(m_line).length());
    }
}

void Cursor::moveToStartOfLine() {
    m_history.markBoundary();
    m_column = 0;
}

void Cursor::moveToEndOfLine() {
    m_history.markBoundary();
    m_column = static_cast<uint32_t>(m_buffer->getString(m_line).length());
}

void Cursor::moveToStartOfFile() {
    m_history.markBoundary();
    m_line = 0;
    m_column = 0;
}

void Cursor::moveToEndOfFile() {
    m_history.markBoundary();
    const auto string_count = m_buffer->getStringCount() - 1;
    const auto string_length = static_cast<uint32_t>(m_buffer->getString(string_count).length());
    m_line = string_count;
    m_column = string_length;
}

void Cursor::activateSelection(const bool active) {
    if (active && !m_is_selection_active) {
        m_is_selection_active = true;
        m_selected_line_start = m_line;
        m_selected_column_start = m_column;
    } else if (!active) {
        m_is_selection_active = false;
        m_selected_line_start = 0;
        m_selected_column_start = 0;
    }
}

void Cursor::setPosition(const uint32_t line, const uint32_t column) {
    if (line > m_buffer->getStringCount() - 1) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    if (column > m_buffer->getString(line).length()) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    m_column = snapColumnOnLine(line, column);
    m_line = line;
}

BufferEdit Cursor::insert(const std::u16string_view characters) {
    const auto cursor_before = position();
    const auto previous_line = m_line;
    const auto &edit = m_buffer->insert(m_line, m_column, characters);
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;

    m_history.record(UndoHistory::Edit{.start = edit.start, .removed = {}, .inserted = std::u16string(characters)}, cursor_before, position());

    if (m_line != previous_line) {
        m_history.markBoundary();
    }
    return edit;
}

BufferEdit Cursor::erase(const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) const {
    return m_buffer->erase(lineStart, columnStart, lineEnd, columnEnd);
}

BufferEdit Cursor::newLine() {
    const auto cursor_before = position();
    const auto &edit = m_buffer->insert(m_line, m_column, u"\n");
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;

    m_history.record(UndoHistory::Edit{.start = edit.start, .removed = {}, .inserted = u"\n"}, cursor_before, position());

    // The cursor always changes line
    m_history.markBoundary();
    return edit;
}

std::optional<BufferEdit> Cursor::eraseLeft() {
    if (m_column > 0) {
        // We can erase on the left since column > 0
        const auto cursor_before = position();
            const auto erased_column = m_column - charLengthBefore(m_buffer->getString(m_line), m_column);
        auto removed = textInRange(m_line, erased_column, m_line, m_column);
        const auto &edit = m_buffer->erase(m_line, m_column, m_line, erased_column);
        m_column = edit.new_end.column;

        m_history.record(UndoHistory::Edit{.start = edit.start, .removed = std::move(removed), .inserted = {}}, cursor_before, position());
        return edit;
    }

    if (m_line > 0) {
        // We can't erase left because column = 0, so we move the remainder of this line to the end of the line above
        const auto cursor_before = position();
            const auto string_above_length = static_cast<uint32_t>(m_buffer->getString(m_line - 1).length());
        auto removed = textInRange(m_line - 1, string_above_length, m_line, m_column);
        const auto &edit =  m_buffer->erase(m_line, m_column, m_line - 1, string_above_length);
        m_line = edit.new_end.line;
        m_column = edit.new_end.column;

        m_history.record(UndoHistory::Edit{.start = edit.start, .removed = std::move(removed), .inserted = {}}, cursor_before, position());

        // The cursor always changes line
        m_history.markBoundary();
        return edit;
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseRight() {
    if (m_column < m_buffer->getString(m_line).length()) {
        // We can erase on the right since column < string_length
        const auto cursor_before = position();
            const auto erased_column = m_column + charLengthAfter(m_buffer->getString(m_line), m_column);
        auto removed = textInRange(m_line, m_column, m_line, erased_column);
        const auto &edit = m_buffer->erase(m_line, m_column, m_line, erased_column);

        m_history.record(UndoHistory::Edit{.start = edit.start, .removed = std::move(removed), .inserted = {}}, cursor_before, position());
        return edit;
    }

    if (m_line < m_buffer->getStringCount() - 1) {
        // We can't erase right because column >= string_length, so we move the line below and append it to this line
        const auto cursor_before = position();
            auto removed = textInRange(m_line, m_column, m_line + 1, 0);
        const auto &edit = m_buffer->erase(m_line, m_column, m_line + 1, 0);

        m_history.record(UndoHistory::Edit{.start = edit.start, .removed = std::move(removed), .inserted = {}}, cursor_before, position());
        return edit;
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseSelection() {
    const auto &range = getSelectedRange();
    if (!range) {
        // No selection, or a degenerate one: nothing to erase, nothing to record.
        // A degenerate selection (anchor == cursor) is still armed, so disarm it here:
        // left armed, its stale anchor line would outlive the edits the caller performs next.
        activateSelection(false);
        return std::nullopt;
    }

    const auto cursor_before = position();
    const auto previous_line = m_line;
    auto removed = textInRange(range->line_start, range->column_start, range->line_end, range->column_end);
    const auto &edit = m_buffer->erase(range->line_start, range->column_start, range->line_end, range->column_end);
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;

    m_history.record(UndoHistory::Edit{.start = edit.start, .removed = std::move(removed), .inserted = {}}, cursor_before, position());

    if (m_line != previous_line) {
        m_history.markBoundary();
    }
    return edit;
}

std::vector<BufferEdit> Cursor::loadContent(const std::u16string_view content) {
    auto edits = std::vector<BufferEdit>{};
    edits.reserve(2);

    // clear() already keeps itself out of the history; the insert goes straight to the buffer for
    // the same reason, so the file is never copied into a group
    edits.emplace_back(clear());
    edits.emplace_back(m_buffer->insert(0, 0, content));

    m_line = 0;
    m_column = 0;
    m_history.clear();
    return edits;
}

BufferEdit Cursor::clear() {
    // Close the current undo group; the wipe itself is intentionally not snapshotted,
    // callers replace the whole content afterwards and clear the history themselves.
    m_history.markBoundary();

    // Reset everything
    m_column = 0;
    m_line = 0;
    m_name = "";

    m_is_selection_active = false;
    m_selected_line_start = 0;
    m_selected_column_start = 0;

    return m_buffer->clear();
}

std::u16string Cursor::textInRange(const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) const {
    if (lineStart == lineEnd) {
        // Single line: one substring, no separator to weave in
        return std::u16string(m_buffer->getString(lineStart).substr(columnStart, columnEnd - columnStart));
    }

    // getByteCount already counts the line separators the joined text will carry, so this is the
    // exact size of the result: reserve it and grow the string only once.
    auto text = std::u16string{};
    text.reserve(m_buffer->getByteCount(lineStart, columnStart, lineEnd, columnEnd) / sizeof(char16_t));

    for (auto line = lineStart; line <= lineEnd; ++line) {
        if (line == lineStart) {
            text.append(m_buffer->getString(line).substr(columnStart));
        } else if (line == lineEnd) {
            text.append(m_buffer->getString(line).substr(0, columnEnd));
        } else {
            text.append(m_buffer->getString(line));
        }

        if (line < lineEnd) {
            text.append(u"\n");
        }
    }
    return text;
}

std::u16string Cursor::getText() const {
    // The whole buffer is just the widest range there is
    const auto last_line = m_buffer->getStringCount() - 1;
    const auto last_column = static_cast<uint32_t>(m_buffer->getString(last_line).length());
    return textInRange(0, 0, last_line, last_column);
}

BufferEdit::Position Cursor::position() const {
    return BufferEdit::Position{.line = m_line, .column = m_column};
}

uint32_t Cursor::snapColumnOnLine(const uint32_t line, const uint32_t column) const {
    return snapToCharBoundary(m_buffer->getString(line), column);
}

std::vector<BufferEdit> Cursor::undo() {
    const auto *const group = m_history.undo();
    if (group == nullptr) {
        return {};
    }

    // Reverting means walking the group backwards: each edit's coordinates describe the buffer as
    // it stood just before that edit, which is only true once the later ones are already undone.
    auto edits = std::vector<BufferEdit>{};
    edits.reserve(group->edits.size());
    for (auto it = group->edits.rbegin(); it != group->edits.rend(); ++it) {
        edits.emplace_back(replaceRange(*m_buffer, it->start, it->inserted, it->removed));
    }

    settleAfterHistoryStep(group->cursor_before);
    return edits;
}

std::vector<BufferEdit> Cursor::redo() {
    const auto *const group = m_history.redo();
    if (group == nullptr) {
        return {};
    }

    // Re-applying replays the group in its original order, the mirror image of undo
    auto edits = std::vector<BufferEdit>{};
    edits.reserve(group->edits.size());
    for (const auto &edit : group->edits) {
        edits.emplace_back(replaceRange(*m_buffer, edit.start, edit.removed, edit.inserted));
    }

    settleAfterHistoryStep(group->cursor_after);
    return edits;
}

void Cursor::settleAfterHistoryStep(const BufferEdit::Position &caret) {
    m_line = caret.line;
    m_column = caret.column;
    activateSelection(false);
    m_history.markBoundary();

    // An undo/redo rewrites the text: it may well restore the very saved state, but the flag is a
    // plain boolean, so the restore conservatively counts as a modification (accepted simplification).
}

void Cursor::clearHistory() {
    m_history.clear();
}

void Cursor::shareMaxHistoryDepth(std::shared_ptr<CVarInt> maxDepth) {
    m_history.shareMaxDepth(std::move(maxDepth));
}

void Cursor::setMaxHistoryDepth() {
    m_history.applyMaxDepth();
}
