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
#ifndef UNDO_HISTORY_H
#define UNDO_HISTORY_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "buffer/BufferEdit.h"
#include "../cvar/CVarInt.h"


/**
 * @brief Stores the text each edit replaced, so undo and redo can apply the inverse.
 *
 * An entry weighs the edit, not the document: a one-character insert costs one character, whatever
 * the size of the buffer. Entries are gathered into groups through a single boundary flag — a new
 * group opens only when the history is at a boundary — so a typed run undoes as one step.
 *
 * Both stacks are capped at the live entry capacity, dropping the oldest group when full, and the
 * characters they retain together are capped by MAX_HISTORY_CHARACTERS.
 */
class UndoHistory final {
public:
    /**
     * @brief One text replacement, in the coordinates valid at the moment it is applied.
     *
     * Groups are applied strictly in LIFO order, so a stored position never needs rebasing: by the
     * time an edit's turn comes, the buffer is back in the state its coordinates describe.
     */
    struct Edit final {
        BufferEdit::Position start; ///< Where the replaced range begins.
        std::u16string removed;     ///< Text present before the change, empty for a pure insert.
        std::u16string inserted;    ///< Text present after the change, empty for a pure erase.
    };

    /** @brief A run of edits undone and redone as a single step. */
    struct Group final {
        std::vector<Edit> edits;            ///< Edits in the order they were applied.
        BufferEdit::Position cursor_before; ///< Caret at the group's start, restored by undo.
        BufferEdit::Position cursor_after;  ///< Caret after the last edit, restored by redo.
    };

private:
    /** Default maximum number of groups kept in each stack. */
    static constexpr uint32_t DEFAULT_MAX_HISTORY_DEPTH = 64u;

    /** Maximum number of characters retained by both stacks together (64 MiB of char16_t). */
    static constexpr std::size_t MAX_HISTORY_CHARACTERS = 64u * 1024u * 1024u / sizeof(char16_t);

    /** Groups available for undo. */
    std::deque<Group> m_undo_stack;

    /** Groups available for redo. */
    std::deque<Group> m_redo_stack;

    /** Shared CVar holding the maximum number of groups kept in each stack. */
    std::shared_ptr<CVarInt> m_max_undo;

    /** Number of characters retained by both stacks together. */
    std::size_t m_retained_characters;

    /** Flag indicating that the next edit must open a new group. */
    bool m_at_boundary;

private:
    /**
     * @brief Returns the live group cap read from the shared CVar.
     *
     * Falls back to DEFAULT_MAX_HISTORY_DEPTH while no CVar has been shared.
     *
     * @return The maximum number of groups kept in each stack, at least 1.
     */
    [[nodiscard]] uint32_t capacity() const;

    /** @return The number of characters a group retains across all of its edits. */
    [[nodiscard]] static std::size_t weigh(const Group &group);

    /**
     * @brief Drops the oldest group of a stack and discounts the characters it retained.
     *
     * @param stack The stack to drop the front of; must not be empty.
     */
    void dropOldest(std::deque<Group> &stack);

    /**
     * @brief Enforces both caps, dropping the oldest groups first.
     *
     * The count cap trims each stack independently. The character cap then drops the oldest undo
     * groups, then the oldest redo ones, and always leaves one group in each stack: undo() and
     * redo() hand out a pointer into a stack, and trimming runs while that pointer is still live.
     */
    void trim();

    /** @brief Empties the redo stack and discounts the characters it retained. */
    void clearRedo();

    /**
     * @brief Merges an edit into the last one of a group when the two are adjacent.
     *
     * Typing, backspacing and deleting forward each arrive one character at a time. Left as
     * separate entries they would cost one buffer operation and one tree-sitter edit apiece on
     * undo, so a run that extends the previous edit is folded into it instead.
     *
     * @param group The group to merge into; its edits must not be empty.
     * @param edit The edit to merge.
     * @return true when the edit was merged, false when it must be appended on its own.
     */
    [[nodiscard]] static bool coalesce(Group &group, const Edit &edit);

public:
    /** @brief Deleted copy constructor. */
    UndoHistory(const UndoHistory &) = delete;

    /** @brief Deleted copy assignment operator. */
    UndoHistory &operator=(const UndoHistory &) = delete;

    /** @brief Constructs an empty history, starting at a boundary. */
    explicit UndoHistory();

    /** @brief Marks a boundary so that the next edit opens a new group. */
    void markBoundary();

    /**
     * @brief Shares the CVar that caps both stacks and trims them to its current value.
     *
     * The cap is read live from this CVar afterwards, so no separate copy is kept.
     *
     * @param maxDepth The shared CVar holding the maximum history depth.
     */
    void shareMaxDepth(std::shared_ptr<CVarInt> maxDepth);

    /** @brief Trims both stacks down to the current capacity, dropping the oldest groups. */
    void applyMaxDepth();

    /**
     * @brief Records an edit that was just applied to the buffer.
     *
     * Opens a new group when the history is at a boundary, otherwise extends the open one, and
     * clears the redo stack: an edit makes whatever could be redone unreachable. An edit that
     * replaces nothing with nothing consumes the boundary without being retained.
     *
     * @param edit The replacement the buffer just underwent.
     * @param cursorBefore The caret position before the group's first edit; used when it opens one.
     * @param cursorAfter The caret position after this edit.
     */
    void record(Edit edit, const BufferEdit::Position &cursorBefore, const BufferEdit::Position &cursorAfter);

    /**
     * @brief Moves the most recent group onto the redo stack and hands it back for reverting.
     *
     * The group is returned by pointer rather than by value: it has to survive for the caller to
     * apply it, and it now lives on the redo stack. The pointer stays valid until the next call
     * that mutates the history.
     *
     * @return The group whose edits must be reverted, or nullptr when the undo stack is empty.
     */
    [[nodiscard]] const Group *undo();

    /**
     * @brief Moves the most recently undone group back onto the undo stack and hands it back.
     *
     * @return The group whose edits must be re-applied, or nullptr when the redo stack is empty.
     */
    [[nodiscard]] const Group *redo();

    /** @brief Wipes both stacks and resets the history to a boundary. */
    void clear();
};


#endif //UNDO_HISTORY_H
