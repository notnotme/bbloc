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


/** @brief Tells whether a boundary at the given index falls between the two units of a surrogate pair. */
static bool splitsSurrogatePair(const std::u16string_view text, const size_t index) {
    return index > 0 && index < text.length()
        && (text[index] & 0xFC00) == 0xDC00
        && (text[index - 1] & 0xFC00) == 0xD800;
}

/**
 * @brief Walks a flat text from a known point up to an index, counting the line ends it crosses.
 *
 * The buffer is line-structured, so a flat character offset only becomes a BufferEdit position by
 * walking the text; walking from an already known point keeps that to one pass per boundary.
 *
 * @param text The text to walk, with its lines joined by line ends.
 * @param from The index the walk starts at.
 * @param fromPoint The position the index `from` designates.
 * @param to The index to walk up to, never before `from`.
 * @return The position the index `to` designates.
 */
static BufferEdit::Position advanceToIndex(const std::u16string_view text, const size_t from, const BufferEdit::Position &fromPoint, const size_t to) {
    auto point = fromPoint;
    for (auto index = from; index < to; ++index) {
        if (text[index] == u'\n') {
            ++point.line;
            point.column = 0;
        } else {
            ++point.column;
        }
    }

    return point;
}

Cursor::Cursor(std::unique_ptr<TextBuffer> buffer)
    : m_buffer(std::move(buffer)),
      m_column(0),
      m_line(0),
      m_is_selection_active(false),
      m_selected_line_start(0),
      m_selected_column_start(0),
      m_is_modified(false) {}

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
    m_column = snapToCharBoundary(m_line, m_column);
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
    m_column = snapToCharBoundary(m_line, m_column);
}

void Cursor::setName(const std::string_view name) {
    m_name = name;
}

std::string_view Cursor::getName() const {
    return m_name;
}

bool Cursor::isModified() const {
    return m_is_modified;
}

void Cursor::setModified(const bool modified) {
    m_is_modified = modified;
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
        m_column = snapToCharBoundary(m_line - 1, m_column);
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
        m_column = snapToCharBoundary(m_line + 1, m_column);
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

    m_column = snapToCharBoundary(line, column);
    m_line = line;
}

BufferEdit Cursor::insert(const std::u16string_view characters) {
    recordBeforeEdit();
    m_is_modified = true;
    const auto previous_line = m_line;
    const auto &edit = m_buffer->insert(m_line, m_column, characters);
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;
    if (m_line != previous_line) {
        m_history.markBoundary();
    }
    return edit;
}

BufferEdit Cursor::erase(const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) const {
    return m_buffer->erase(lineStart, columnStart, lineEnd, columnEnd);
}

BufferEdit Cursor::newLine() {
    recordBeforeEdit();
    m_is_modified = true;
    const auto &edit = m_buffer->insert(m_line, m_column, u"\n");
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;
    // The cursor always changes line
    m_history.markBoundary();
    return edit;
}

std::optional<BufferEdit> Cursor::eraseLeft() {
    if (m_column > 0) {
        // We can erase on the left since column > 0
        recordBeforeEdit();
        m_is_modified = true;
        const auto &edit = m_buffer->erase(m_line, m_column, m_line, m_column - charLengthBefore(m_buffer->getString(m_line), m_column));
        m_column = edit.new_end.column;
        return edit;
    }

    if (m_line > 0) {
        // We can't erase left because column = 0, so we move the remainder of this line to the end of the line above
        recordBeforeEdit();
        m_is_modified = true;
        const auto string_above_length = static_cast<uint32_t>(m_buffer->getString(m_line - 1).length());
        const auto &edit =  m_buffer->erase(m_line, m_column, m_line - 1, string_above_length);
        m_line = edit.new_end.line;
        m_column = edit.new_end.column;
        // The cursor always changes line
        m_history.markBoundary();
        return edit;
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseRight() {
    if (m_column < m_buffer->getString(m_line).length()) {
        // We can erase on the right since column < string_length
        recordBeforeEdit();
        m_is_modified = true;
        return m_buffer->erase(m_line, m_column, m_line, m_column + charLengthAfter(m_buffer->getString(m_line), m_column));
    }

    if (m_line < m_buffer->getStringCount() - 1) {
        // We can't erase right because column >= string_length, so we move the line below and append it to this line
        recordBeforeEdit();
        m_is_modified = true;
        return m_buffer->erase(m_line, m_column, m_line + 1, 0);
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

    recordBeforeEdit();
    m_is_modified = true;
    const auto previous_line = m_line;
    const auto &edit = m_buffer->erase(range->line_start, range->column_start, range->line_end, range->column_end);
    m_line = edit.new_end.line;
    m_column = edit.new_end.column;
    if (m_line != previous_line) {
        m_history.markBoundary();
    }
    return edit;
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
    m_is_modified = true;

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

void Cursor::recordBeforeEdit() {
    if (m_history.isAtBoundary()) {
        m_history.push(UndoHistory::Snapshot{.text = getText(), .line = m_line, .column = m_column});
    }
}

uint32_t Cursor::snapToCharBoundary(const uint32_t line, const uint32_t column) const {
    const auto &string = m_buffer->getString(line);
    // The length bound is mandatory: the view must never be indexed at its own size.
    if (column > 0 && column < string.length() && (string[column] & 0xFC00) == 0xDC00 && (string[column - 1] & 0xFC00) == 0xD800) {
        // The column sits inside a surrogate pair, step back to its lead unit
        return column - 1;
    }
    return column;
}

BufferEdit Cursor::restore(const UndoHistory::Snapshot &snapshot, const std::u16string_view currentText) {
    const auto &new_text = snapshot.text;

    // Only the differing middle is rewritten. Replacing the whole buffer would hand the highlighter
    // an edit spanning the entire document, so a one-character undo would cost a from-scratch
    // re-parse and a full longest-line re-measure.
    const auto &prefix_mismatch = std::mismatch(currentText.begin(), currentText.end(), new_text.begin(), new_text.end());
    auto prefix_length = static_cast<size_t>(prefix_mismatch.first - currentText.begin());

    // The two ends must never meet: over a repeating region ("aaaa" -> "aaa") the common suffix
    // reaches back into the common prefix, and the range between them would then run backwards.
    const auto suffix_limit = std::min(currentText.length(), new_text.length()) - prefix_length;
    const auto &suffix_mismatch = std::mismatch(currentText.rbegin(), currentText.rbegin() + static_cast<ptrdiff_t>(suffix_limit), new_text.rbegin());
    auto suffix_length = static_cast<size_t>(suffix_mismatch.first - currentText.rbegin());

    // Neither boundary may land inside a surrogate pair. Widening the range by one code unit is
    // always safe; splitting the pair would leave both sides of the edit unencodable.
    if (splitsSurrogatePair(currentText, prefix_length) || splitsSurrogatePair(new_text, prefix_length)) {
        --prefix_length;
    }
    if (splitsSurrogatePair(currentText, currentText.length() - suffix_length)
        || splitsSurrogatePair(new_text, new_text.length() - suffix_length)) {
        --suffix_length;
    }

    // Both points describe the buffer as it stands, which is exactly what currentText holds.
    const auto start = advanceToIndex(currentText, 0, BufferEdit::Position{.line = 0, .column = 0}, prefix_length);
    const auto old_end = advanceToIndex(currentText, prefix_length, start, currentText.length() - suffix_length);
    const auto middle = std::u16string_view(new_text).substr(prefix_length, new_text.length() - suffix_length - prefix_length);

    // The two halves carry the offsets and the points of the combined edit: the erase leaves the
    // start of the range where it was, so the insert that follows starts at the very same offset.
    // Each half degenerates to a no-op edit at start when its side is empty, identical texts included.
    const auto &erase_edit = m_buffer->erase(start.line, start.column, old_end.line, old_end.column);
    const auto &insert_edit = m_buffer->insert(start.line, start.column, middle);

    m_line = snapshot.line;
    m_column = snapshot.column;
    activateSelection(false);
    m_history.markBoundary();

    // An undo/redo rewrites the text: it may well restore the very saved state, but the flag is a
    // plain boolean, so the restore conservatively counts as a modification (accepted simplification).
    m_is_modified = true;

    return BufferEdit {
        .start_byte = erase_edit.start_byte,
        .old_end_byte = erase_edit.old_end_byte,
        .new_end_byte = insert_edit.new_end_byte,
        .start = erase_edit.start,
        .old_end = erase_edit.old_end,
        .new_end = insert_edit.new_end
    };
}

std::optional<BufferEdit> Cursor::undo() {
    if (!m_history.canUndo()) {
        // Ask before building the current state: an empty stack would only discard it, and under
        // key repeat that is a full-buffer copy per event
        return std::nullopt;
    }

    // The current text is needed twice, as the state handed to the history and as the left side of
    // the diff below: joining the lines once and copying it is cheaper than building it twice.
    const auto current_text = getText();
    const auto &snapshot = m_history.undo(UndoHistory::Snapshot{.text = current_text, .line = m_line, .column = m_column});
    if (!snapshot) {
        return std::nullopt;
    }

    return restore(snapshot.value(), current_text);
}

std::optional<BufferEdit> Cursor::redo() {
    if (!m_history.canRedo()) {
        // Same as undo: nothing to restore means nothing to capture
        return std::nullopt;
    }

    const auto current_text = getText();
    const auto &snapshot = m_history.redo(UndoHistory::Snapshot{.text = current_text, .line = m_line, .column = m_column});
    if (!snapshot) {
        return std::nullopt;
    }

    return restore(snapshot.value(), current_text);
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
