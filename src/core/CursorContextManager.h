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
#ifndef CURSOR_CONTEXT_MANAGER_H
#define CURSOR_CONTEXT_MANAGER_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "cvar/CVarInt.h"
#include "CursorContext.h"


/**
 * @brief Owns every open CursorContext (one per file) and tracks which one is active.
 *
 * Contexts are created, activated, cycled, and closed through this manager, which
 * guarantees that at least one context always exists. On every activation change it
 * clears the outgoing context's pending command feedback (a stale interactive prompt
 * must not survive a switch) and requests a redraw of the newly active context. It
 * also maintains the buffer_index / buffer_count fields of each context, used by the
 * views to show the active buffer position.
 */
class CursorContextManager final {
private:
    /** The command runner passed to every created context. */
    CommandRunner &m_command_runner;

    /** The theme passed to every created context. */
    Theme &m_theme;

    /** The prompt cursor passed to every created context. */
    PromptCursor &m_prompt_cursor;

    /** CVar capping the undo/redo history depth, shared with every context's cursor. */
    std::shared_ptr<CVarInt> m_max_undo;

    /** The open contexts; never empty. */
    std::vector<std::unique_ptr<CursorContext>> m_contexts;

    /** Index of the active context inside m_contexts. */
    size_t m_active_index;

    /** @brief Updates the buffer_index / buffer_count fields of every open context. */
    void refreshIndices();

    /**
     * @brief Builds a context wired to the shared runtime objects, without touching the open ones.
     *
     * Kept free of any bookkeeping so close() can use it too: the indices cannot be refreshed
     * while the vector is momentarily empty, which is exactly when close() needs a fresh context.
     *
     * @return The newly built context, owned by the caller.
     */
    [[nodiscard]] std::unique_ptr<CursorContext> makeContext() const;

public:
    /** @brief Deleted copy constructor. */
    CursorContextManager(const CursorContextManager &) = delete;

    /** @brief Deleted copy assignment operator. */
    CursorContextManager &operator=(const CursorContextManager &) = delete;

    /**
     * @brief Constructs the manager with one fresh scratch context.
     *
     * @param commandRunner The CommandRunner used to execute text commands.
     * @param theme The Theme instance applied to every context.
     * @param promptCursor The PromptCursor used for command-line input interaction.
     * @param maxUndo The shared CVar holding the maximum undo/redo history depth.
     */
    explicit CursorContextManager(CommandRunner &commandRunner, Theme &theme, PromptCursor &promptCursor, std::shared_ptr<CVarInt> maxUndo);

    /** @brief Returns the active context. */
    [[nodiscard]] CursorContext &active();

    /** @brief Returns the context at the given index. */
    [[nodiscard]] CursorContext &get(size_t index);

    /** @brief Returns the index of the active context. */
    [[nodiscard]] size_t getActiveIndex() const;

    /** @brief Returns the number of open contexts. */
    [[nodiscard]] size_t getCount() const;

    /**
     * @brief Appends a fresh scratch context without activating it.
     *
     * The new context shares the maximum undo/redo history depth CVar.
     *
     * @return Reference to the created context, appended last.
     */
    CursorContext &createContext();

    /**
     * @brief Makes the context at the given index the active one.
     *
     * Clears the outgoing context's pending command feedback and requests a
     * redraw of the newly active context. Out-of-range indices are ignored.
     *
     * @param index Index of the context to activate.
     */
    void activate(size_t index);

    /** @brief Activates the next open context, wrapping around. */
    void next();

    /** @brief Activates the previous open context, wrapping around. */
    void prev();

    /**
     * @brief Closes the active context.
     *
     * When the closed context was the last one remaining, a fresh scratch context
     * replaces it so exactly one context always exists.
     *
     * @return true when another open context became active, false when a fresh scratch replaced the last one.
     */
    bool close();

    /**
     * @brief Finds the index of the open context whose cursor name matches.
     *
     * @param name UTF-8 encoded cursor name (file path) to look up.
     * @return The index of the matching context, or std::nullopt when none matches.
     */
    [[nodiscard]] std::optional<size_t> indexOf(std::string_view name) const;

    /** @brief Trims the undo/redo history of every open context down to the shared maximum depth. */
    void applyMaxHistoryDepth();
};


#endif //CURSOR_CONTEXT_MANAGER_H
