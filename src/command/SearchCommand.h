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
        SEARCH,       ///< Store a new search term and select its first match.
        FIND_NEXT,    ///< Select the next match of the stored term.
        FIND_PREV,    ///< Select the previous match of the stored term.
        REPLACE,      ///< Replace the next match of a term with a replacement.
        REPLACE_ALL   ///< Replace every match of a term with a replacement.
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

    /**
     * @brief Scans buffer lines for one term, folding case once per term and once per line.
     *
     * The case-insensitive comparison is the ASCII-only fold of toLowerAscii, applied to both sides
     * exactly as the per-position comparison it replaces did; matching then runs on the folded copies,
     * so a single find/rfind replaces the hand-rolled quadratic scan.
     */
    class LineScanner final {
    private:
        /** The term to look for, ASCII-folded when the comparison is case-insensitive. */
        std::u16string m_term;

        /** Scratch holding the folded copy of the current line; unused when the comparison is case-sensitive. */
        std::u16string m_folded_line;

        /** The line being scanned: the caller's view, or a view over m_folded_line. */
        std::u16string_view m_line;

        /** true when the comparison is case-sensitive and no folding happens. */
        const bool m_case_sensitive;

    public:
        /**
         * @brief Builds a scanner for one term under one case-sensitivity mode.
         * @param term The term to look for.
         * @param caseSensitive Whether the comparison is case-sensitive.
         */
        explicit LineScanner(std::u16string_view term, bool caseSensitive);

        /**
         * @brief Sets the line the next lookups run on, folding it once when needed.
         * @param line The line content to scan.
         */
        void setLine(std::u16string_view line);

        /**
         * @brief Finds the first occurrence of the term at or after an offset on the current line.
         * @param from The column offset to start scanning from.
         * @return The starting column, or std::u16string_view::npos when absent.
         */
        [[nodiscard]] size_t indexOf(size_t from) const;

        /**
         * @brief Finds the last occurrence of the term starting before a bound on the current line.
         * @param limit The exclusive upper bound for the match start column.
         * @return The starting column, or std::u16string_view::npos when absent.
         */
        [[nodiscard]] size_t lastIndexOf(size_t limit) const;

        /** @return The term length in code units. */
        [[nodiscard]] size_t termLength() const;

        /**
         * @brief Tells whether the term can overlap itself, i.e. a proper prefix of it is also a suffix.
         *
         * scanMatches enumerates non-overlapping occurrences, so a term that cannot overlap itself has
         * every one of its occurrences in that enumeration. Backward stepping relies on it.
         *
         * @return true when two occurrences of the term can overlap.
         */
        [[nodiscard]] bool isSelfOverlapping() const;
    };

    /** The action performed by this instance. */
    const Action m_action;

    /** CVar controlling whether comparisons are case-sensitive. */
    const std::shared_ptr<CVarBool> m_case_sensitive;

    /**
     * @brief Lower-cases an ASCII letter, leaving other code units untouched.
     *
     * Only A-Z are folded; this is the deliberate ASCII-only limitation of the v1 case-insensitive matching.
     *
     * @param character The code unit to fold.
     * @return The lower-cased code unit.
     */
    [[nodiscard]] static char16_t toLowerAscii(char16_t character);

    /**
     * @brief Renders an unsigned value as a UTF-16 decimal string.
     * @param value The value to render.
     * @return The decimal representation.
     */
    [[nodiscard]] static std::u16string toU16(uint32_t value);

    /**
     * @brief Scans forward for the first match at or after a position, without wrapping.
     * @param cursor The cursor whose buffer is scanned.
     * @param term The term to look for.
     * @param startLine The line to start scanning from.
     * @param startColumn The column to start scanning from on the first line.
     * @param caseSensitive Whether the comparison is case-sensitive.
     * @return The match location, or std::nullopt when none is found.
     */
    [[nodiscard]] static std::optional<MatchLocation> searchForward(const Cursor &cursor, LineScanner &scanner, uint32_t startLine, uint32_t startColumn);

    /**
     * @brief Scans backward for the last match starting before a position, without wrapping.
     * @param cursor The cursor whose buffer is scanned.
     * @param term The term to look for.
     * @param beforeLine The line to start scanning from.
     * @param beforeColumn The exclusive column bound on the first line.
     * @param caseSensitive Whether the comparison is case-sensitive.
     * @return The match location, or std::nullopt when none is found.
     */
    [[nodiscard]] static std::optional<MatchLocation> searchBackward(const Cursor &cursor, LineScanner &scanner, uint32_t beforeLine, uint32_t beforeColumn);

    /**
     * @brief Scans every non-overlapping occurrence of a term in a single pass.
     *
     * Counts the total number of occurrences and records the ordinal of the one matching @p current.
     *
     * @param cursor The cursor whose buffer is scanned.
     * @param term The term to look for.
     * @param current The match whose ordinal is looked up.
     * @param caseSensitive Whether the comparison is case-sensitive.
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
     * @brief Runs the SEARCH action: stores the term and selects its first match.
     * @param payload The cursor context to update.
     * @param args The command arguments forming the term.
     * @return A status or error message.
     */
    [[nodiscard]] std::optional<std::u16string> runSearch(CursorContext &payload, std::span<const std::u16string_view> args) const;

    /**
     * @brief Runs the FIND_NEXT or FIND_PREV action using the stored term.
     * @param payload The cursor context to update.
     * @return A status or error message.
     */
    [[nodiscard]] std::optional<std::u16string> runFind(CursorContext &payload) const;

    /**
     * @brief Runs the REPLACE or REPLACE_ALL action.
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
