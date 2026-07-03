#include "Cursor.h"


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

    const auto cursor_string_length = m_buffer->getString(m_line).length();
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
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

    const auto cursor_string_length = m_buffer->getString(m_line).length();
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
}

void Cursor::setName(const std::string_view name) {
    m_name = name;
}

std::string_view Cursor::getName() const {
    return m_name;
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

    auto result = std::vector<std::u16string_view>();
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
            m_column = m_buffer->getString(m_line - 1).length();
            --m_line;
        }
    } else {
        m_column -= charLengthBefore(m_column);
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
        m_column += charLengthAfter(m_column);
    }
}

void Cursor::moveUp() {
    m_history.markBoundary();

    if (m_line > 0) {
        const auto string_above_length = m_buffer->getString(m_line - 1).length();
        if (m_column > string_above_length) {
            m_column = string_above_length;
        }
        --m_line;
    } else {
        m_column = 0;
    }
}

void Cursor::moveDown() {
    m_history.markBoundary();

    if (m_line < m_buffer->getStringCount() - 1) {
        const auto string_below_length = m_buffer->getString(m_line + 1).length();
        if (m_column > string_below_length) {
            // The cursor can't stay at the same X position, put it at the end of the next line
            m_column = string_below_length;
        }
        ++m_line;
    } else {
        m_column = m_buffer->getString(m_line).length();
    }
}

void Cursor::moveToStartOfLine() {
    m_history.markBoundary();
    m_column = 0;
}

void Cursor::moveToEndOfLine() {
    m_history.markBoundary();
    m_column = m_buffer->getString(m_line).length();
}

void Cursor::moveToStartOfFile() {
    m_history.markBoundary();
    m_line = 0;
    m_column = 0;
}

void Cursor::moveToEndOfFile() {
    m_history.markBoundary();
    const auto string_count = m_buffer->getStringCount() - 1;
    const auto string_length = m_buffer->getString(string_count).length();
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

    m_column = column;
    m_line = line;
}

BufferEdit Cursor::insert(const std::u16string_view characters) {
    recordBeforeEdit();
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
        const auto &edit = m_buffer->erase(m_line, m_column, m_line, m_column - charLengthBefore(m_column));
        m_column = edit.new_end.column;
        return edit;
    }

    if (m_line > 0) {
        // We can't erase left because column = 0, so we move the remainder of this line to the end of the line above
        recordBeforeEdit();
        const auto string_above_length = m_buffer->getString(m_line - 1).length();
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
        return m_buffer->erase(m_line, m_column, m_line, m_column + charLengthAfter(m_column));
    }

    if (m_line < m_buffer->getStringCount() - 1) {
        // We can't erase right because column >= string_length, so we move the line below and append it to this line
        recordBeforeEdit();
        return m_buffer->erase(m_line, m_column, m_line + 1, 0);
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseSelection() {
    const auto &range = getSelectedRange();
    if (!range) {
        // No selection, or a degenerate one: nothing to erase, nothing to record
        return std::nullopt;
    }

    recordBeforeEdit();
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

    return m_buffer->clear();
}

std::u16string Cursor::getText() const {
    auto text = std::u16string();
    const auto line_count = m_buffer->getStringCount();
    for (auto line = 0u; line < line_count; ++line) {
        text.append(m_buffer->getString(line));
        if (line < line_count - 1) {
            text.append(u"\n");
        }
    }
    return text;
}

void Cursor::recordBeforeEdit() {
    if (m_history.isAtBoundary()) {
        m_history.push({getText(), m_line, m_column});
    }
}

uint32_t Cursor::charLengthBefore(const uint32_t column) const {
    const auto &string = m_buffer->getString(m_line);
    if (column >= 2 && (string[column - 1] & 0xFC00) == 0xDC00 && (string[column - 2] & 0xFC00) == 0xD800) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}

uint32_t Cursor::charLengthAfter(const uint32_t column) const {
    const auto &string = m_buffer->getString(m_line);
    if (column + 1 < string.length() && (string[column] & 0xFC00) == 0xD800 && (string[column + 1] & 0xFC00) == 0xDC00) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}

BufferEdit Cursor::restore(const UndoHistory::Snapshot &snapshot) {
    const auto &clear_edit = m_buffer->clear();

    auto new_end_byte = 0u;
    auto new_end = BufferEdit::Position {0, 0};
    if (!snapshot.text.empty()) {
        const auto &insert_edit = m_buffer->insert(0, 0, snapshot.text);
        new_end_byte = insert_edit.new_end_byte;
        new_end = insert_edit.new_end;
    }

    m_line = snapshot.line;
    m_column = snapshot.column;
    activateSelection(false);
    m_history.markBoundary();

    return BufferEdit {
        .start_byte = 0,
        .old_end_byte = clear_edit.old_end_byte,
        .new_end_byte = new_end_byte,
        .start = {0, 0},
        .old_end = clear_edit.old_end,
        .new_end = new_end
    };
}

std::optional<BufferEdit> Cursor::undo() {
    const auto &snapshot = m_history.undo({getText(), m_line, m_column});
    if (!snapshot) {
        return std::nullopt;
    }

    return restore(snapshot.value());
}

std::optional<BufferEdit> Cursor::redo() {
    const auto &snapshot = m_history.redo({getText(), m_line, m_column});
    if (!snapshot) {
        return std::nullopt;
    }

    return restore(snapshot.value());
}

void Cursor::clearHistory() {
    m_history.clear();
}
