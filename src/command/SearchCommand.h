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
#ifndef SEARCH_COMMAND_H
#define SEARCH_COMMAND_H

#include <memory>
#include <optional>
#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/base/LineScanner.h"
#include "../core/cvar/CVarBool.h"


/**
 * @brief Command implementing the search and replace features of the editor.
 *
 * A single class parameterized by an Action selected at construction, registered once per action.
 * Matches are recomputed on every navigation rather than cached, so edits never leave stale offsets behind.
 */
class SearchCommand final : public Command<CursorContext> {
public:
    /** @brief The concrete behaviour a given instance performs. */
    enum class Action {
        Search,       ///< Store a new search term and select its first match.
        FindNext,     ///< Select the next match of the stored term.
        FindPrev,     ///< Select the previous match of the stored term.
        Replace,      ///< Replace the next match of a term with a replacement.
        ReplaceAll    ///< Replace every match of a term with a replacement.
    };

private:
    /** @brief The line and starting column of a single match. */
    struct MatchLocation final {
        uint32_t line;   ///< Line where the match starts and ends (matches never span lines).
        uint32_t column; ///< Column where the match starts.
    };

    /** @brief The ordinal of a match among all occurrences, and the total number of occurrences. */
    struct MatchStats final {
        int32_t index; ///< Zero-based ordinal of the current match, or -1 when it was not found.
        int32_t total; ///< Total number of occurrences in the buffer.
    };

    /** The action performed by this instance. */
    const Action m_action;

    /** CVar controlling whether comparisons are case-sensitive. */
    const std::shared_ptr<CVarBool> m_case_sensitive;

    /**
     * @brief Renders an unsigned value as a UTF-16 decimal string.
     * @param value The value to render.
     * @return The decimal representation.
     */
    [[nodiscard]] static std::u16string toU16(uint32_t value);

    /**
     * @brief Scans forward for the first match at or after a position, without wrapping.
     * @param cursor The cursor whose buffer is scanned.
     * @param scanner The scanner holding the term and the case-sensitivity mode; stateful, its setLine is
     *                called on every line walked.
     * @param startLine The line to start scanning from.
     * @param startColumn The column to start scanning from on the first line.
     * @return The match location, or std::nullopt when none is found.
     */
    [[nodiscard]] static std::optional<MatchLocation> searchForward(const Cursor &cursor, LineScanner &scanner, uint32_t startLine, uint32_t startColumn);

    /**
     * @brief Scans backward for the last match starting before a position, without wrapping.
     * @param cursor The cursor whose buffer is scanned.
     * @param scanner The scanner holding the term and the case-sensitivity mode; stateful, its setLine is
     *                called on every line walked.
     * @param beforeLine The line to start scanning from.
     * @param beforeColumn The exclusive column bound on the first line.
     * @return The match location, or std::nullopt when none is found.
     */
    [[nodiscard]] static std::optional<MatchLocation> searchBackward(const Cursor &cursor, LineScanner &scanner, uint32_t beforeLine, uint32_t beforeColumn);

    /**
     * @brief Scans every non-overlapping occurrence of a term in a single pass.
     *
     * Counts the total number of occurrences and records the ordinal of the one matching @p current.
     *
     * @param cursor The cursor whose buffer is scanned.
     * @param scanner The scanner holding the term and the case-sensitivity mode; stateful, its setLine is
     *                called on every line walked.
     * @param current The match whose ordinal is looked up.
     * @return The ordinal of @p current (or -1 when absent) and the total occurrence count.
     */
    [[nodiscard]] static MatchStats scanMatches(const Cursor &cursor, LineScanner &scanner, const MatchLocation &current);

    /**
     * @brief Stores the match statistics along with the match they describe.
     *
     * @param payload The cursor context to update.
     * @param stats The ordinal and total to publish.
     * @param match The match the ordinal describes.
     * @param caseSensitive The case-sensitivity mode the statistics were computed under.
     * @param scanned true when the ordinal is the exact one a full scan gives for @p match.
     */
    static void storeMatchStats(CursorContext &payload, const MatchStats &stats, const MatchLocation &match, bool caseSensitive, bool scanned);

    /**
     * @brief Tells whether the stored total can be stepped instead of recounted.
     *
     * The total survives as long as nothing invalidated it (every edit and every cursor move call
     * SearchState::resetMatches) and the selection is still exactly the match the stored ordinal
     * describes, so the neighbouring match is the neighbouring ordinal.
     *
     * @param payload The cursor context holding the stored statistics.
     * @param scanner The scanner built for the current term and case-sensitivity mode.
     * @param caseSensitive The case-sensitivity mode the lookup runs under.
     * @param backward true when stepping to the previous match.
     * @return true when the stored total and ordinal can be reused.
     */
    [[nodiscard]] static bool canStepMatchStats(const CursorContext &payload, const LineScanner &scanner, bool caseSensitive, bool backward);

    /**
     * @brief Selects a match and requests the view to follow the cursor.
     * @param payload The cursor context to update.
     * @param match The match to select.
     * @param termLength The length of the matched term, in code units.
     */
    static void selectMatch(CursorContext &payload, const MatchLocation &match, uint32_t termLength);

    /**
     * @brief Replaces a selected match with a replacement string, reusing the editor's text-input sequence.
     * @param payload The cursor context to update.
     * @param replacement The text inserted in place of the current selection.
     */
    static void replaceSelection(CursorContext &payload, std::u16string_view replacement);

    /**
     * @brief Runs the Search action: stores the term and selects its first match.
     * @param payload The cursor context to update.
     * @param args The command arguments forming the term.
     * @return A status or error message.
     */
    [[nodiscard]] std::optional<std::u16string> runSearch(CursorContext &payload, std::span<const std::u16string_view> args) const;

    /**
     * @brief Runs the FindNext or FindPrev action using the stored term.
     * @param payload The cursor context to update.
     * @return A status or error message.
     */
    [[nodiscard]] std::optional<std::u16string> runFind(CursorContext &payload) const;

    /**
     * @brief Runs the Replace or ReplaceAll action.
     * @param payload The cursor context to update.
     * @param args The command arguments: the term and its replacement.
     * @return A status or error message.
     */
    [[nodiscard]] std::optional<std::u16string> runReplace(CursorContext &payload, std::span<const std::u16string_view> args) const;

public:
    /**
     * @brief Constructs a SearchCommand bound to a single action.
     * @param action The action this instance performs.
     * @param caseSensitive The CVar controlling whether comparisons are case-sensitive.
     */
    explicit SearchCommand(Action action, std::shared_ptr<CVarBool> caseSensitive);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not auto-complete.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the search or replace action bound to this instance.
     * @param payload The cursor context that will be modified by this command.
     * @param args Command arguments, whose meaning depends on the action.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;

    /**
     * @brief Recomputes the persistent match counter for the stored term under a new case-sensitivity mode.
     *
     * A pure counter refresh: neither the cursor position nor the active selection is moved. The counter is
     * anchored on the current selection start (or the cursor when nothing is selected) so it stays meaningful
     * even when the previously-selected match no longer matches under @p caseSensitive.
     *
     * @param payload The cursor context whose search counter is refreshed.
     * @param caseSensitive Whether comparisons are case-sensitive.
     */
    static void refreshMatchStats(CursorContext &payload, bool caseSensitive);
};


#endif //SEARCH_COMMAND_H
