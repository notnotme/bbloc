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
        case Action::SEARCH:
            return runSearch(payload, args);
        case Action::FIND_NEXT:
        case Action::FIND_PREV:
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }
            return runFind(payload);
        case Action::REPLACE:
        case Action::REPLACE_ALL:
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

        payload.command_feedback = CommandFeedback {
            .prompt_message = u"search ",
            .command_string = u"search",
            .on_validate_callback = [&](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
                payload.command_runner.runCommand(std::u16string(command).append(u" ").append(input), true);
                return std::nullopt;
            }
        };

        return std::nullopt;
    }

    // Join the arguments back with single spaces, reserving the final length up front.
    auto joined_term = std::u16string();
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

    // Look from the cursor to the end, then wrap around from the top of the buffer.
    auto match = searchForward(cursor, term, cursor.getLine(), cursor.getColumn(), case_sensitive);
    if (!match) {
        match = searchForward(cursor, term, 0, 0, case_sensitive);
    }

    if (!match) {
        payload.search.match_index = -1;
        payload.search.match_count = 0;
        return u"not found";
    }

    const auto current_matches = scanMatches(cursor, term, match.value(), case_sensitive);
    payload.search.match_index = current_matches.index;
    payload.search.match_count = current_matches.total;
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
    const auto selection = cursor.getSelectedRange();

    std::optional<MatchLocation> match;
    if (m_action == Action::FIND_NEXT) {
        // Resume just after the current selection so the same match is not returned again.
        const auto from_line = selection ? selection->line_end : cursor.getLine();
        const auto from_column = selection ? selection->column_end : cursor.getColumn();

        match = searchForward(cursor, term, from_line, from_column, case_sensitive);
        if (!match) {
            match = searchForward(cursor, term, 0, 0, case_sensitive);
        }
    } else {
        const auto before_line = selection ? selection->line_start : cursor.getLine();
        const auto before_column = selection ? selection->column_start : cursor.getColumn();

        match = searchBackward(cursor, term, before_line, before_column, case_sensitive);
        if (!match) {
            const auto last_line = cursor.getLineCount() - 1;
            const auto last_column = static_cast<uint32_t>(cursor.getString(last_line).length()) + 1;
            match = searchBackward(cursor, term, last_line, last_column, case_sensitive);
        }
    }

    if (!match) {
        payload.search.match_index = -1;
        payload.search.match_count = 0;
        return u"not found";
    }

    const auto current_matches = scanMatches(cursor, term, match.value(), case_sensitive);
    payload.search.match_index = current_matches.index;
    payload.search.match_count = current_matches.total;
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
    payload.search.term = std::u16string(from);

    if (m_action == Action::REPLACE) {
        auto match = searchForward(cursor, from, cursor.getLine(), cursor.getColumn(), case_sensitive);
        if (!match) {
            match = searchForward(cursor, from, 0, 0, case_sensitive);
        }

        if (!match) {
            payload.search.match_index = -1;
            payload.search.match_count = 0;
            return u"not found";
        }

        const auto current_matches = scanMatches(cursor, from, match.value(), case_sensitive);
        payload.search.match_index = current_matches.index;
        payload.search.match_count = current_matches.total;
        selectMatch(payload, match.value(), static_cast<uint32_t>(from.length()));
        replaceSelection(payload, to);
        return std::u16string(u"replaced ").append(toU16(1)).append(u" occurrence(s)");
    }

    // REPLACE_ALL: resume scanning past each replacement so freshly inserted text is never re-matched.
    auto count = 0u;
    auto start_line = 0u;
    auto start_column = 0u;
    while (const auto match = searchForward(cursor, from, start_line, start_column, case_sensitive)) {
        selectMatch(payload, match.value(), static_cast<uint32_t>(from.length()));
        replaceSelection(payload, to);
        start_line = cursor.getLine();
        start_column = cursor.getColumn();
        ++count;
    }

    // Every match was consumed, so the persistent indicator has nothing left to show.
    payload.search.match_index = -1;
    payload.search.match_count = 0;
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

    if (const auto &selection = cursor.eraseSelection()) {
        payload.highlighter.edit(selection.value());
        cursor.setPosition(selection->new_end.line, selection->new_end.column);
        cursor.activateSelection(false);
    }

    const auto &edit = cursor.insert(replacement);
    payload.stick.index = cursor.getColumn();
    payload.scroll.follow_indicator = true;
    payload.highlighter.edit(edit);
    payload.wants_redraw = true;
}

std::optional<SearchCommand::MatchLocation> SearchCommand::searchForward(const Cursor &cursor, const std::u16string_view term, const uint32_t startLine, const uint32_t startColumn, const bool caseSensitive) {
    const auto line_count = cursor.getLineCount();
    for (auto line = startLine; line < line_count; ++line) {
        const auto text = cursor.getString(line);
        const auto from = line == startLine ? startColumn : 0u;
        if (const auto position = indexOf(text, term, from, caseSensitive); position != std::u16string_view::npos) {
            return MatchLocation { line, static_cast<uint32_t>(position) };
        }
    }

    return std::nullopt;
}

std::optional<SearchCommand::MatchLocation> SearchCommand::searchBackward(const Cursor &cursor, const std::u16string_view term, const uint32_t beforeLine, const uint32_t beforeColumn, const bool caseSensitive) {
    for (auto line = static_cast<uint32_t>(beforeLine); line > 0; --line) {
        const auto text = cursor.getString(line);
        const auto limit = line == static_cast<uint32_t>(beforeLine)
            ? beforeColumn
            : text.length() + 1;

        if (const auto position = lastIndexOf(text, term, limit, caseSensitive); position != std::u16string_view::npos) {
            return MatchLocation { line, static_cast<uint32_t>(position) };
        }
    }

    return std::nullopt;
}

SearchCommand::MatchStats SearchCommand::scanMatches(const Cursor &cursor, const std::u16string_view term, const MatchLocation &current, const bool caseSensitive) {
    if (term.empty()) {
        return { -1, 0 };
    }

    auto index = -1;
    auto total = 0;
    const auto line_count = cursor.getLineCount();
    for (auto line = 0u; line < line_count; ++line) {
        const auto text = cursor.getString(line);
        auto from = size_t { 0 };
        while (true) {
            const auto position = indexOf(text, term, from, caseSensitive);
            if (position == std::u16string_view::npos) {
                break;
            }
            if (line == current.line && static_cast<uint32_t>(position) == current.column) {
                index = total;
            }
            ++total;
            from = position + term.length();
        }
    }

    return { index, total };
}

void SearchCommand::refreshMatchStats(CursorContext &payload, const bool caseSensitive) {
    payload.wants_redraw = true;

    if (!payload.search.term || payload.search.term->empty()) {
        payload.search.match_index = -1;
        payload.search.match_count = 0;
        return;
    }

    const auto &term = payload.search.term.value();
    const auto &cursor = payload.cursor;

    // Anchor on the current selection start, or the bare cursor when nothing is selected.
    const auto selection = cursor.getSelectedRange();
    const auto anchor_line = selection ? selection->line_start : cursor.getLine();
    const auto anchor_column = selection ? selection->column_start : cursor.getColumn();

    auto total = 0;
    auto before = 0;
    auto exact = -1;
    const auto line_count = cursor.getLineCount();
    for (auto line = 0u; line < line_count; ++line) {
        const auto text = cursor.getString(line);
        auto from = size_t { 0 };
        while (true) {
            const auto position = indexOf(text, term, from, caseSensitive);
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
            from = position + term.length();
        }
    }

    if (total == 0) {
        payload.search.match_index = -1;
        payload.search.match_count = 0;
        return;
    }

    // Prefer the ordinal of a match landing on the anchor; otherwise place the counter just past
    // the matches preceding it, clamped in case the anchor sits after the final match.
    const auto index = exact >= 0 ? exact : before;
    payload.search.match_index = std::clamp(index, 0, total - 1);
    payload.search.match_count = total;
}

size_t SearchCommand::indexOf(const std::u16string_view line, const std::u16string_view term, const size_t from, const bool caseSensitive) {
    if (caseSensitive) {
        return line.find(term, from);
    }

    // Manual scan replicating find semantics without lowering whole strings; in particular,
    // an empty term matches at from whenever it fits within the line.
    for (auto position = from; position + term.size() <= line.size(); ++position) {
        const auto candidate = line.substr(position, term.size());
        if (std::equal(candidate.begin(), candidate.end(), term.begin(), [](const char16_t left, const char16_t right) {
            return toLowerAscii(left) == toLowerAscii(right);
        })) {
            return position;
        }
    }

    return std::u16string_view::npos;
}

size_t SearchCommand::lastIndexOf(const std::u16string_view line, const std::u16string_view term, const size_t limit, const bool caseSensitive) {
    if (limit == 0) {
        return std::u16string_view::npos;
    }

    // rfind returns the greatest start position <= limit - 1, i.e. strictly before the limit.
    const auto search_position = limit - 1;
    if (caseSensitive) {
        return line.rfind(term, search_position);
    }

    // Manual scan replicating rfind semantics; an empty term matches at min(search_position, line.size()).
    if (term.size() > line.size()) {
        return std::u16string_view::npos;
    }

    // Scan down to 0 inclusive, minding unsigned wrap-around on the last decrement.
    const auto start = std::min(search_position, line.size() - term.size());
    for (auto position = start;; --position) {
        const auto candidate = line.substr(position, term.size());
        if (std::equal(candidate.begin(), candidate.end(), term.begin(), [](const char16_t left, const char16_t right) {
            return toLowerAscii(left) == toLowerAscii(right);
        })) {
            return position;
        }

        if (position == 0) {
            break;
        }
    }

    return std::u16string_view::npos;
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
