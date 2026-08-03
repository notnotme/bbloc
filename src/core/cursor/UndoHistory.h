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
#include <optional>
#include <string>

#include "../cvar/CVarInt.h"


/**
 * @brief Stores full-buffer snapshots used to undo and redo text edits.
 *
 * Snapshots are coalesced through a single boundary flag: a new snapshot is
 * pushed only when the history is at a boundary. Both stacks are capped at
 * the live capacity, dropping the oldest snapshot when full, and the characters
 * they retain together are capped by MAX_HISTORY_CHARACTERS: a snapshot weighs
 * the whole buffer, so a count cap alone cannot bound the memory used.
 */
class UndoHistory final {
public:
    /** @brief Represents the full state of a buffer at a given point in time. */
    struct Snapshot final {
        std::u16string text; ///< Full buffer content.
        uint32_t line;       ///< Cursor line at capture time.
        uint32_t column;     ///< Cursor column at capture time.
    };

private:
    /** Default maximum number of snapshots kept in each stack. */
    static constexpr uint32_t DEFAULT_MAX_HISTORY_DEPTH = 64u;

    /** Maximum number of characters retained by both stacks together (64 MiB of char16_t). */
    static constexpr std::size_t MAX_HISTORY_CHARACTERS = 64u * 1024u * 1024u / sizeof(char16_t);

    /** Snapshots available for undo. */
    std::deque<Snapshot> m_undo_stack;

    /** Snapshots available for redo. */
    std::deque<Snapshot> m_redo_stack;

    /** Shared CVar holding the maximum number of snapshots kept in each stack. */
    std::shared_ptr<CVarInt> m_max_undo;

    /** Number of characters retained by both stacks together. */
    std::size_t m_retained_characters;

    /** Flag indicating that the next edit must push a new snapshot. */
    bool m_at_boundary;

private:
    /**
     * @brief Returns the live snapshot cap read from the shared CVar.
     *
     * Falls back to DEFAULT_MAX_HISTORY_DEPTH while no CVar has been shared.
     *
     * @return The maximum number of snapshots kept in each stack, at least 1.
     */
    [[nodiscard]] uint32_t capacity() const;

    /**
     * @brief Drops the oldest snapshot of a stack and discounts the characters it retained.
     *
     * @param stack The stack to drop the front of; must not be empty.
     */
    void dropOldest(std::deque<Snapshot> &stack);

    /**
     * @brief Enforces both caps, dropping the oldest snapshots first.
     *
     * The count cap trims each stack independently. The character cap then drops the oldest undo
     * snapshots, then the oldest redo ones, and always leaves one snapshot behind so an undo in
     * the middle of an edit never silently becomes a no-op.
     */
    void trim();

public:
    /** @brief Deleted copy constructor. */
    UndoHistory(const UndoHistory &) = delete;

    /** @brief Deleted copy assignment operator. */
    UndoHistory &operator=(const UndoHistory &) = delete;

    /** @brief Constructs an empty history, starting at a boundary. */
    explicit UndoHistory();

    /** @brief Marks a boundary so that the next edit pushes a new snapshot. */
    void markBoundary();

    /**
     * @brief Shares the CVar that caps both stacks and trims them to its current value.
     *
     * The cap is read live from this CVar afterwards, so no separate copy is kept.
     *
     * @param maxDepth The shared CVar holding the maximum history depth.
     */
    void shareMaxDepth(std::shared_ptr<CVarInt> maxDepth);

    /** @brief Trims both stacks down to the current capacity, dropping the oldest snapshots. */
    void applyMaxDepth();

    /** @brief Returns true when the next edit must push a new snapshot. */
    [[nodiscard]] bool isAtBoundary() const;

    /** @brief Returns true when the undo stack holds at least one snapshot. */
    [[nodiscard]] bool canUndo() const;

    /** @brief Returns true when the redo stack holds at least one snapshot. */
    [[nodiscard]] bool canRedo() const;

    /**
     * @brief Pushes a snapshot onto the undo stack.
     *
     * Clears the redo stack, drops the oldest snapshots past the current caps,
     * and clears the boundary flag.
     *
     * A snapshot repeating the text already on top of the undo stack is dropped instead of being
     * retained: the two states are undistinguishable to undo, and keeping the older one keeps the
     * cursor position undo must restore. The boundary is consumed either way.
     *
     * @param snapshot The snapshot to store.
     */
    void push(Snapshot snapshot);

    /**
     * @brief Pops the most recent snapshot from the undo stack.
     *
     * The current state is moved onto the redo stack.
     *
     * @param current The state of the buffer before undoing.
     * @return The snapshot to restore, or std::nullopt if the undo stack is empty.
     */
    [[nodiscard]] std::optional<Snapshot> undo(Snapshot current);

    /**
     * @brief Pops the most recent snapshot from the redo stack.
     *
     * The current state is moved onto the undo stack.
     *
     * @param current The state of the buffer before redoing.
     * @return The snapshot to restore, or std::nullopt if the redo stack is empty.
     */
    [[nodiscard]] std::optional<Snapshot> redo(Snapshot current);

    /** @brief Wipes both stacks and resets the history to a boundary. */
    void clear();
};


#endif //UNDO_HISTORY_H
