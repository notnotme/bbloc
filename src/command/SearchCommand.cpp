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
#include <algorithm>

#include "SearchCommand.h"


SearchCommand::SearchCommand(const Action action, std::shared_ptr<CVarBool> caseSensitive)
    : m_action(action),
      m_case_sensitive(std::move(caseSensitive)) {}

void SearchCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> SearchCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    switch (m_action) {
        case Action::Search:
            return runSearch(payload, args);
        case Action::FindNext:
        case Action::FindPrev:
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }
            return runFind(payload);
        case Action::Replace:
        case Action::ReplaceAll:
            return runReplace(payload, args);
    }

    return std::nullopt;
}

std::optional<std::u16string> SearchCommand::runSearch(CursorContext &payload, const std::span<const std::u16string_view> args) const {
    if (args.empty()) {
        // From the prompt the search term is mandatory; from the editor, ask for it interactively.
        if (payload.from_prompt) {
            return u"Usage: search <term>";
        }

        payload.command_feedback = requestArgument(u"search ", u"search", payload.command_runner);

        return std::nullopt;
    }

    // Join the arguments back with single spaces, reserving the final length up front.
    auto joined_term = std::u16string{};
    auto term_length = args.size() - 1;
    for (const auto &argument : args) {
        term_length += argument.length();
    }

    joined_term.reserve(term_length);
    for (auto index = 0u; index < args.size(); ++index) {
        if (index > 0) {
            joined_term.push_back(u' ');
        }
        joined_term.append(args[index]);
    }

    // Stored before the not-found return below, so find_next keeps working after a failed search.
    payload.search.term = std::move(joined_term);
    const auto &term = payload.search.term.value();

    const auto &cursor = payload.cursor;
    const auto case_sensitive = m_case_sensitive->m_value;
    auto scanner = LineScanner(term, case_sensitive);

    // Look from the cursor to the end, then wrap around from the top of the buffer.
    auto match = searchForward(cursor, scanner, cursor.getLine(), cursor.getColumn());
    if (!match) {
        match = searchForward(cursor, scanner, 0, 0);
    }

    if (!match) {
        payload.search.resetMatches();
        return u"not found";
    }

    const auto current_matches = scanMatches(cursor, scanner, match.value());
    storeMatchStats(payload, current_matches, match.value(), case_sensitive, current_matches.index >= 0);
    selectMatch(payload, match.value(), static_cast<uint32_t>(term.length()));
    return std::nullopt;
}

std::optional<std::u16string> SearchCommand::runFind(CursorContext &payload) const {
    if (!payload.search.term) {
        return u"no search term";
    }

    const auto &term = payload.search.term.value();
    const auto &cursor = payload.cursor;
    const auto case_sensitive = m_case_sensitive->m_value;
    const auto backward = m_action == Action::FindPrev;
    auto scanner = LineScanner(term, case_sensitive);
    const auto selection = cursor.getSelectedRange();

    // Read the stored statistics before the lookup moves the selection they are anchored on.
    const auto can_step = canStepMatchStats(payload, scanner, case_sensitive, backward);
    const auto stored_index = payload.search.match_index;
    const auto stored_total = payload.search.match_count;

    std::optional<MatchLocation> match;
    auto wrapped = false;
    if (!backward) {
        // Resume just after the current selection so the same match is not returned again.
        const auto from_line = selection ? selection->line_end : cursor.getLine();
        const auto from_column = selection ? selection->column_end : cursor.getColumn();

        match = searchForward(cursor, scanner, from_line, from_column);
        if (!match) {
            wrapped = true;
            match = searchForward(cursor, scanner, 0, 0);
        }
    } else {
        const auto before_line = selection ? selection->line_start : cursor.getLine();
        const auto before_column = selection ? selection->column_start : cursor.getColumn();

        match = searchBackward(cursor, scanner, before_line, before_column);
        if (!match) {
            wrapped = true;
            const auto last_line = cursor.getLineCount() - 1;
            const auto last_column = static_cast<uint32_t>(cursor.getString(last_line).length()) + 1;
            match = searchBackward(cursor, scanner, last_line, last_column);
        }
    }

    if (!match) {
        payload.search.resetMatches();
        return u"not found";
    }

    if (can_step) {
        // The buffer and the term are unchanged and the stored ordinal is exact, so the neighbouring
        // match carries the neighbouring ordinal; a wrap lands on the last or the first one.
        const auto index = wrapped
            ? (backward ? stored_total - 1 : 0)
            : stored_index + (backward ? -1 : 1);

        if (index >= 0 && index < stored_total) {
            storeMatchStats(payload, MatchStats{.index = index, .total = stored_total}, match.value(), case_sensitive, true);
            selectMatch(payload, match.value(), static_cast<uint32_t>(term.length()));
            return std::nullopt;
        }
    }

    const auto current_matches = scanMatches(cursor, scanner, match.value());
    storeMatchStats(payload, current_matches, match.value(), case_sensitive, current_matches.index >= 0);
    selectMatch(payload, match.value(), static_cast<uint32_t>(term.length()));
    return std::nullopt;
}

std::optional<std::u16string> SearchCommand::runReplace(CursorContext &payload, const std::span<const std::u16string_view> args) const {
    if (args.size() != 2) {
        return u"Usage: replace <from> <to>";
    }

    const auto from = args[0];
    const auto to = args[1];
    if (from.empty()) {
        return u"Search term is empty.";
    }

    const auto &cursor = payload.cursor;
    const auto case_sensitive = m_case_sensitive->m_value;
    auto scanner = LineScanner(from, case_sensitive);
    payload.search.term = std::u16string(from);

    if (m_action == Action::Replace) {
        auto match = searchForward(cursor, scanner, cursor.getLine(), cursor.getColumn());
        if (!match) {
            match = searchForward(cursor, scanner, 0, 0);
        }

        if (!match) {
            payload.search.resetMatches();
            return u"not found";
        }

        const auto current_matches = scanMatches(cursor, scanner, match.value());
        storeMatchStats(payload, current_matches, match.value(), case_sensitive, current_matches.index >= 0);
        selectMatch(payload, match.value(), static_cast<uint32_t>(from.length()));
        replaceSelection(payload, to);
        return std::u16string(u"replaced ").append(toU16(1)).append(u" occurrence(s)");
    }

    // REPLACE_ALL: resume scanning past each replacement so freshly inserted text is never re-matched.
    auto count = 0u;
    auto start_line = 0u;
    auto start_column = 0u;
    while (const auto match = searchForward(cursor, scanner, start_line, start_column)) {
        selectMatch(payload, match.value(), static_cast<uint32_t>(from.length()));
        replaceSelection(payload, to);
        start_line = cursor.getLine();
        start_column = cursor.getColumn();
        ++count;
    }

    // Every match was consumed, so the persistent indicator has nothing left to show.
    payload.search.resetMatches();
    return std::u16string(u"replaced ").append(toU16(count)).append(u" occurrence(s)");
}

void SearchCommand::selectMatch(CursorContext &payload, const MatchLocation &match, const uint32_t termLength) {
    auto &cursor = payload.cursor;

    // Reset any active selection so the anchor snaps to the start of the new match.
    cursor.activateSelection(false);
    cursor.setPosition(match.line, match.column);
    cursor.activateSelection(true);
    cursor.setPosition(match.line, match.column + termLength);

    payload.scroll.follow_indicator = true;
    payload.wants_redraw = true;
}

void SearchCommand::replaceSelection(CursorContext &payload, const std::u16string_view replacement) {
    auto &cursor = payload.cursor;

    // The buffer is about to change under the stored ordinal: keep the counter on screen, but make
    // the next find_next/find_prev recount instead of stepping a total that no longer holds.
    payload.search.match_scanned = false;

    payload.eraseSelectionIfAny();

    const auto &edit = cursor.insert(replacement);
    payload.stick.index = cursor.getColumn();
    payload.scroll.follow_indicator = true;
    payload.highlighter.edit(edit);
    payload.wants_redraw = true;
}

std::optional<SearchCommand::MatchLocation> SearchCommand::searchForward(const Cursor &cursor, LineScanner &scanner, const uint32_t startLine, const uint32_t startColumn) {
    const auto line_count = cursor.getLineCount();
    for (auto line = startLine; line < line_count; ++line) {
        scanner.setLine(cursor.getString(line));
        const auto from = line == startLine ? startColumn : 0u;
        if (const auto position = scanner.indexOf(from); position != std::u16string_view::npos) {
            return MatchLocation{.line = line, .column = static_cast<uint32_t>(position)};
        }
    }

    return std::nullopt;
}

std::optional<SearchCommand::MatchLocation> SearchCommand::searchBackward(const Cursor &cursor, LineScanner &scanner, const uint32_t beforeLine, const uint32_t beforeColumn) {
    // Decrement in the condition so line 0 is scanned too, then stops before wrapping around.
    for (auto line = beforeLine + 1; line-- > 0;) {
        const auto text = cursor.getString(line);
        scanner.setLine(text);
        const auto limit = line == beforeLine
            ? beforeColumn
            : text.length() + 1;

        if (const auto position = scanner.lastIndexOf(limit); position != std::u16string_view::npos) {
            return MatchLocation{.line = line, .column = static_cast<uint32_t>(position)};
        }
    }

    return std::nullopt;
}

SearchCommand::MatchStats SearchCommand::scanMatches(const Cursor &cursor, LineScanner &scanner, const MatchLocation &current) {
    const auto term_length = scanner.termLength();
    if (term_length == 0) {
        return MatchStats{.index = -1, .total = 0};
    }

    auto index = -1;
    auto total = 0;
    const auto line_count = cursor.getLineCount();
    for (auto line = 0u; line < line_count; ++line) {
        scanner.setLine(cursor.getString(line));
        size_t from = 0;
        while (true) {
            const auto position = scanner.indexOf(from);
            if (position == std::u16string_view::npos) {
                break;
            }
            if (line == current.line && static_cast<uint32_t>(position) == current.column) {
                index = total;
            }
            ++total;
            from = position + term_length;
        }
    }

    return MatchStats{.index = index, .total = total};
}

void SearchCommand::storeMatchStats(CursorContext &payload, const MatchStats &stats, const MatchLocation &match, const bool caseSensitive, const bool scanned) {
    payload.search.match_index = stats.index;
    payload.search.match_count = stats.total;
    payload.search.match_line = match.line;
    payload.search.match_column = match.column;
    payload.search.match_case_sensitive = caseSensitive;
    payload.search.match_scanned = scanned;
}

bool SearchCommand::canStepMatchStats(const CursorContext &payload, const LineScanner &scanner, const bool caseSensitive, const bool backward) {
    const auto &search = payload.search;
    if (!search.match_scanned || search.match_index < 0 || search.match_count <= 0) {
        // Nothing counted, or the stored ordinal is not the one a full scan would give.
        return false;
    }

    if (search.match_case_sensitive != caseSensitive) {
        // The mode changed since the count was taken, so both the total and the ordinal may differ.
        return false;
    }

    if (backward && scanner.isSelfOverlapping()) {
        // scanMatches enumerates non-overlapping occurrences; a term overlapping itself lets the
        // backward lookup land between two of them, where only a rescan can rank the result.
        return false;
    }

    // The selection must still be exactly the match the stored ordinal describes.
    const auto selection = payload.cursor.getSelectedRange();
    return selection.has_value()
        && selection->line_start == search.match_line
        && selection->line_end == search.match_line
        && selection->column_start == search.match_column
        && selection->column_end == search.match_column + static_cast<uint32_t>(scanner.termLength());
}

void SearchCommand::refreshMatchStats(CursorContext &payload, const bool caseSensitive) {
    payload.wants_redraw = true;

    if (!payload.search.term || payload.search.term->empty()) {
        payload.search.resetMatches();
        return;
    }

    const auto &term = payload.search.term.value();
    const auto &cursor = payload.cursor;
    auto scanner = LineScanner(term, caseSensitive);

    // Anchor on the current selection start, or the bare cursor when nothing is selected.
    const auto selection = cursor.getSelectedRange();
    const auto anchor_line = selection ? selection->line_start : cursor.getLine();
    const auto anchor_column = selection ? selection->column_start : cursor.getColumn();

    auto total = 0;
    auto before = 0;
    auto exact = -1;
    const auto line_count = cursor.getLineCount();
    for (auto line = 0u; line < line_count; ++line) {
        scanner.setLine(cursor.getString(line));
        size_t from = 0;
        while (true) {
            const auto position = scanner.indexOf(from);
            if (position == std::u16string_view::npos) {
                break;
            }
            const auto column = static_cast<uint32_t>(position);
            if (line == anchor_line && column == anchor_column) {
                exact = total;
            }
            if (line < anchor_line || (line == anchor_line && column < anchor_column)) {
                ++before;
            }
            ++total;
            from = position + scanner.termLength();
        }
    }

    if (total == 0) {
        payload.search.resetMatches();
        return;
    }

    // Prefer the ordinal of a match landing on the anchor; otherwise place the counter just past
    // the matches preceding it, clamped in case the anchor sits after the final match.
    const auto index = exact >= 0 ? exact : before;
    const auto stats = MatchStats{.index = std::clamp(index, 0, total - 1), .total = total};

    // Only an anchor sitting on a match got its exact ordinal, so only that one can be stepped.
    storeMatchStats(payload, stats, MatchLocation{.line = anchor_line, .column = anchor_column}, caseSensitive, exact >= 0);
}

SearchCommand::LineScanner::LineScanner(const std::u16string_view term, const bool caseSensitive)
    : m_term(term),
      m_case_sensitive(caseSensitive) {
    if (!m_case_sensitive) {
        // Fold the term once, instead of folding both sides at every candidate position.
        for (auto &character : m_term) {
            character = toLowerAscii(character);
        }
    }
}

void SearchCommand::LineScanner::setLine(const std::u16string_view line) {
    if (m_case_sensitive) {
        m_line = line;
        return;
    }

    // Fold the line once into the scratch; every lookup on it then runs on the folded copy.
    m_folded_line.assign(line);
    for (auto &character : m_folded_line) {
        character = toLowerAscii(character);
    }

    m_line = m_folded_line;
}

size_t SearchCommand::LineScanner::indexOf(const size_t from) const {
    // Both sides are folded, so find carries the same semantics as the per-position comparison:
    // in particular an empty term matches at from whenever it fits within the line.
    return m_line.find(m_term, from);
}

size_t SearchCommand::LineScanner::lastIndexOf(const size_t limit) const {
    if (limit == 0) {
        return std::u16string_view::npos;
    }

    // rfind returns the greatest start position <= limit - 1, i.e. strictly before the limit.
    return m_line.rfind(m_term, limit - 1);
}

size_t SearchCommand::LineScanner::termLength() const {
    return m_term.length();
}

bool SearchCommand::LineScanner::isSelfOverlapping() const {
    // A shift that leaves the term matching itself is exactly an overlap of two occurrences.
    const auto term = std::u16string_view { m_term };
    for (size_t shift = 1; shift < term.length(); ++shift) {
        if (term.substr(shift) == term.substr(0, term.length() - shift)) {
            return true;
        }
    }

    return false;
}

char16_t SearchCommand::toLowerAscii(const char16_t character) {
    if (character >= u'A' && character <= u'Z') {
        return static_cast<char16_t>(character - u'A' + u'a');
    }

    return character;
}

std::u16string SearchCommand::toU16(const uint32_t value) {
    const auto digits = std::to_string(value);
    return {digits.begin(), digits.end()};
}
