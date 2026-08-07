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
#ifndef CURSOR_H
#define CURSOR_H

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "buffer/TextBuffer.h"
#include "buffer/BufferEdit.h"
#include "TextRange.h"
#include "UndoHistory.h"
#include "../cvar/CVarInt.h"


/**
 * @brief Represents a text cursor and view into a text buffer.
 *
 * This class manages cursor movement, editing operations,
 * and links to an abstract text buffer for storage.
 */
class Cursor final {
private:
    /** Name of the buffer (filename). */
    std::string m_name;

    /** Pointer to the text buffer backend (owns it). */
    std::unique_ptr<TextBuffer> m_buffer;

    /** Current column (X) position of the cursor. */
    uint32_t m_column;

    /** Current line (Y) position of the cursor. */
    uint32_t m_line;

    /** Holds the state of the selection (active / not active) */
    bool m_is_selection_active;

    /** Holds the index of the line where the selection starts. */
    uint32_t m_selected_line_start;

    /** Holds the index of the column where the selection starts. */
    uint32_t m_selected_column_start;

    /** Undo/redo history of full-buffer snapshots. */
    UndoHistory m_history;

    /** True when the buffer text changed since it was last saved or loaded. */
    bool m_is_modified;

private:
    /**
     * @brief Erase a range of text inside the internal buffer. This does not move the cursor coordinates.
     *
     * @param lineStart The line index where the range starts.
     * @param columnStart The column index where the range starts.
     * @param lineEnd The line index where the range stops.
     * @param columnEnd The column index where the range stops.
     * @return Reference to the resulting BufferEdit describing the change.
     */
    [[nodiscard]] BufferEdit erase(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const;

    /**
     * @brief Returns a copy of the text between two positions, lines joined with line breaks.
     *
     * Costs the length of the range, not of the buffer. The coordinates must be ordered
     * (start before end), the way getSelectedRange always returns them.
     *
     * @param lineStart The line index where the range starts.
     * @param columnStart The column index where the range starts.
     * @param lineEnd The line index where the range stops.
     * @param columnEnd The column index where the range stops.
     * @return The text the range covers, empty when the range is degenerate.
     */
    [[nodiscard]] std::u16string textInRange(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const;

    /**
     * @brief Pushes a snapshot of the current state when the history is at a boundary.
     *
     * The history drops the snapshot when it repeats the text it already holds on top, so a
     * boundary crossed without any text change costs a comparison instead of a retained copy.
     */
    void recordBeforeEdit();

    /**
     * @brief Pulls a column off the trailing half of a surrogate pair.
     *
     * Moves clamping on a line length alone can land between the two code units of a non-BMP
     * character; editing from there would split the pair and leave the buffer unencodable.
     *
     * @param line The line the column belongs to.
     * @param column The column to snap.
     * @return The column moved one code unit back when it splits a surrogate pair, unchanged otherwise.
     */
    [[nodiscard]] uint32_t snapToCharBoundary(uint32_t line, uint32_t column) const;

    /**
     * @brief Replaces the buffer content and cursor position with a snapshot.
     *
     * Only the region where the two texts differ is rewritten: the common prefix and the common
     * suffix stay in place, so the returned edit is no wider than the change itself and the
     * incremental re-parse keeps its work proportional to it. Deactivates the selection and marks
     * a history boundary.
     *
     * @param snapshot The snapshot to restore.
     * @param currentText The buffer content before the restore, with its lines joined as getText() joins them.
     * @return A BufferEdit covering the differing region only, degenerate when both texts are equal.
     */
    [[nodiscard]] BufferEdit restore(const UndoHistory::Snapshot &snapshot, std::u16string_view currentText);

public:
    /** @brief Deleted copy constructor. */
    Cursor(const Cursor &) = delete;

    /** @brief Deleted copy assignment operator. */
    Cursor &operator=(const Cursor &) = delete;

    /**
     * @brief Constructs a new Cursor.
     * @param buffer Pointer to the text buffer to manage.
     */
    explicit Cursor(std::unique_ptr<TextBuffer> buffer);

    /** @brief Returns the entire buffer content as a single string, lines joined with line breaks. */
    [[nodiscard]] std::u16string getText() const;

    /** @brief Moves the cursor up by a number of lines. */
    void pageUp(uint32_t lineCount);

    /** @brief Moves the cursor down by a number of lines. */
    void pageDown(uint32_t lineCount);

    /** @brief Sets the name associated with this cursor. */
    void setName(std::string_view name);

    /** @brief Gets the name of the buffer. */
    [[nodiscard]] std::string_view getName() const;

    /**
     * @brief Tells whether the buffer text changed since it was last saved or loaded.
     *
     * Undoing back to the exact saved state still reads as modified: the flag is a
     * plain boolean, not a comparison against the saved content.
     */
    [[nodiscard]] bool isModified() const;

    /**
     * @brief Sets the modified flag.
     *
     * Every text mutation raises the flag itself; callers only lower it, after a
     * successful save or right after loading a file into the buffer.
     *
     * @param modified The new value of the flag.
     */
    void setModified(bool modified);

    /** @brief Returns the current column index. */
    [[nodiscard]] uint32_t getColumn() const;

    /** @brief Returns the current line index. */
    [[nodiscard]] uint32_t getLine() const;

    /** @brief Returns The SelectedRange, if available. Always in the right direction (start -> end). */
    [[nodiscard]] std::optional<TextRange> getSelectedRange() const;

    /**
     * @brief Gets the content of a specific line from the buffer.
     *
     * @param line The index of the line to fetch.
     */
    [[nodiscard]] std::u16string_view getString(uint32_t line) const;

    /** @brief Returns the total number of lines in the buffer. */
    [[nodiscard]] uint32_t getLineCount() const;

    /**
     * @brief Returns the weighted character length of the longest line in the buffer.
     *
     * With tab-stop rendering a tab expands to 1..tabWeight visual columns, so the weighted
     * length is an upper bound on the visual width: the max horizontal scroll it feeds is
     * slightly over-provisioned on tabby lines, which is accepted.
     *
     * @param tabWeight The number of character widths a tab character occupies at most.
     * @return The weighted length of the longest line, in characters.
     */
    [[nodiscard]] uint32_t getLongestLineLength(uint32_t tabWeight) const;

    /**
     * @brief Returns the number of tab characters in the given line.
     *
     * Read from the buffer's per-line metrics, without scanning the line.
     *
     * @param line The index of the line to query.
     * @return The tab count of the line.
     */
    [[nodiscard]] uint32_t getLineTabCount(uint32_t line) const;

    /** @brief Returns the current line at the cursor line position (from column 0). */
    [[nodiscard]] std::u16string_view getString() const;

    /** @brief Returns the portion of selected text, if any. */
    [[nodiscard]] std::optional<std::vector<std::u16string_view>> getSelectedText() const;

    /** @brief Moves the cursor one character to the left. Otherwise, move one line above.  */
    void moveLeft();

    /** @brief Moves the cursor one character to the right. Otherwise, move one line below. */
    void moveRight();

    /** @brief Moves the cursor one line up. Otherwise, goes to the start of the line. */
    void moveUp();

    /** @brief Moves the cursor one line down. Otherwise, goes to the end of the line. */
    void moveDown();

    /** @brief Moves the cursor to the start of the current line. */
    void moveToStartOfLine();

    /** @brief Moves the cursor to the end of the current line. */
    void moveToEndOfLine();

    /** @brief Moves the cursor to the start of the file. */
    void moveToStartOfFile();

    /** @brief Moves the cursor to the end of the file. */
    void moveToEndOfFile();

    /**
     * @brief Set the cursor in selection mode. Moves will grow or shrink the selection.
     *
     * @param active if true, the selection is activated, if false, the selection is deactivated and resets its state.
     */
    void activateSelection(bool active);

    /**
     * @brief Sets the new position of the cursor.
     *
     * A column landing inside a surrogate pair is snapped back to the start of that character.
     *
     * @param line New line index.
     * @param column New column index.
     * @throw std::runtime_error If coordinates are out of bounds.
     */
    void setPosition(uint32_t line, uint32_t column);

    /**
      * @brief Inserts UTF-16 text at the current cursor position.
      *
      * Moves the cursor forward by the number of inserted characters.
      *
      * @param characters The UTF-16 string to insert.
      * @return The resulting BufferEdit describing the change.
      */
    [[nodiscard]] BufferEdit insert(std::u16string_view characters);

    /**
     * @brief Inserts a line break at the current cursor position.
     *
     * Moves the cursor to the beginning of the next line.
     *
     * @return The resulting BufferEdit describing the change.
     */
    [[nodiscard]] BufferEdit newLine();

    /**
     * @brief Deletes the character immediately before the cursor.
     *
     * Move the cursor backward. It has no effect if the cursor is at the beginning of the buffer.
     *
     * @return An optional BufferEdit describing the change.
     */
    [[nodiscard]] std::optional<BufferEdit> eraseLeft();

    /**
     * @brief Deletes the character immediately after the cursor.
     *
     * It has no effect if the cursor is at the end of the buffer.
     *
     * @return An optional BufferEdit describing the change.
     */
    [[nodiscard]] std::optional<BufferEdit> eraseRight();

    /** @return An optional BufferEdit describing the change. */
    [[nodiscard]] std::optional<BufferEdit> eraseSelection();

    /**
     * @brief Clears the entire buffer and resets the cursor to the beginning.
     *
     * After this call, the buffer will be empty and the cursor will be at (0, 0).
     *
     * @return The resulting CursorEdit describing the change.
     */
    [[nodiscard]] BufferEdit clear();

    /**
     * @brief Restores the buffer to its state before the last recorded edit.
     *
     * @return An optional BufferEdit describing the change, or std::nullopt if there is nothing to undo.
     */
    [[nodiscard]] std::optional<BufferEdit> undo();

    /**
     * @brief Re-applies the last undone edit.
     *
     * @return An optional BufferEdit describing the change, or std::nullopt if there is nothing to redo.
     */
    [[nodiscard]] std::optional<BufferEdit> redo();

    /** @brief Wipes the undo/redo history. */
    void clearHistory();

    /**
     * @brief Shares the CVar capping the undo/redo history depth with the history.
     * @param maxDepth The shared CVar holding the maximum history depth.
     */
    void shareMaxHistoryDepth(std::shared_ptr<CVarInt> maxDepth);

    /** @brief Trims the undo/redo history down to the shared maximum depth. */
    void setMaxHistoryDepth();
};


#endif //CURSOR_H
