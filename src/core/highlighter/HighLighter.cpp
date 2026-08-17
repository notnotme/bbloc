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
#include "HighLighter.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <ranges>
#include <utf8.h>

#include "ParserCatalog.h"


HighLighter::HighLighter(Cursor &cursor)
    : m_cursor(cursor),
      m_parsers(ParserCatalog::getParsers()),
      p_current_parser(nullptr),
      p_ts_parser(ts_parser_new()),
      p_ts_tree(nullptr),
      p_ts_query_cursor(ts_query_cursor_new()),
      m_cache_start_line(0),
      // TSInput is third-party and carries no in-class initializers: its trailing `decode` member
      // is spelled out. It only applies to TSInputEncodingCustom, so a UTF-16LE input has no
      // custom decoder and passes nullptr.
      m_input(this, inputCallback, TSInputEncodingUTF16LE, nullptr),
      m_high_light(HighLightId::None),
      m_is_dirty(false),
      m_edit_lines_shifted(false),
      m_dirty_line_min(std::numeric_limits<uint32_t>::max()),
      m_dirty_line_max(0) {}

HighLighter::~HighLighter() {
    // Cleanup tree
    if (p_ts_tree != nullptr) {
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    // Cleanup parser
    ts_parser_delete(p_ts_parser);
    p_ts_parser = nullptr;
    p_current_parser = nullptr;

    // Cleanup query cursor
    ts_query_cursor_delete(p_ts_query_cursor);
    p_ts_query_cursor = nullptr;
}

void HighLighter::setMode(const HighLightId highLight) {
    m_high_light = highLight;
    m_is_dirty = true;

    if (highLight == HighLightId::None) {
        // There is no need to keep the old mode
        p_current_parser = nullptr;
    } else {
        // Set new mode
        const auto &parser = m_parsers.at(highLight);
        if (! ts_parser_set_language(p_ts_parser, parser.getLanguage())) {
            throw std::runtime_error("Could not set parser for language highlight");
        }

        p_current_parser = &parser;
    }

    if (p_ts_tree != nullptr) {
        // Delete the old tree, as tree-sitter may need to reparse it from scratch
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    m_line_cache.clear();
    m_cache_start_line = 0;
}

void HighLighter::setMode(const std::string_view extension) {
    setMode(ParserCatalog::findModeByExtension(extension));
}

std::string_view HighLighter::getModeString() const {
    if (m_high_light == HighLightId::None) {
        // None is TEXT
        return "TEXT";
    }

    return p_current_parser->getName();
}

void HighLighter::parse() {
    if (p_current_parser != nullptr && m_is_dirty) {
        // Reuse the old tree to parse and create a new one; if p_ts_tree is nullptr, parse from scratch
        auto *new_tree = ts_parser_parse(p_ts_parser, p_ts_tree, m_input);

        if (p_ts_tree == nullptr || m_line_cache.empty() || m_edit_lines_shifted) {
            // Line-count changes shift the cache rows; drop the cache, it is rebuilt lazily around the lines actually queried
            m_line_cache.clear();
            m_cache_start_line = 0;
        } else {
            // Pure in-line edits: repaint only the affected cached lines
            repaintChangedLines(new_tree);
        }

        // Delete the old tree and set the new as the current one
        if (p_ts_tree != nullptr) {
            ts_tree_delete(p_ts_tree);
        }
        p_ts_tree = new_tree;

        m_edit_lines_shifted = false;
        m_dirty_line_min = std::numeric_limits<uint32_t>::max();
        m_dirty_line_max = 0;
        m_is_dirty = false;
    }
}

void HighLighter::updateCache(const uint32_t line) const {
    const auto line_count = m_cursor.getLineCount();
    const auto half_window = CACHE_LINE_COUNT / 2;
    const auto start_line = line > half_window ? line - half_window : 0;
    const auto end_line = std::min(start_line + CACHE_LINE_COUNT, line_count);

    m_cache_start_line = start_line;
    m_line_cache.clear();
    m_line_cache.resize(end_line - start_line);

    paintCacheLines(ts_tree_root_node(p_ts_tree), start_line, end_line - 1);
}

void HighLighter::paintCacheLines(const TSNode rootNode, const uint32_t firstLine, const uint32_t lastLine) const {
    ts_query_cursor_set_point_range(p_ts_query_cursor, TSPoint{.row = firstLine, .column = 0}, TSPoint{.row = lastLine + 1, .column = 0});
    ts_query_cursor_exec(p_ts_query_cursor, p_current_parser->getQuery(), rootNode);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(p_ts_query_cursor, &match)) {
        for (uint16_t i = 0; i < match.capture_count; ++i) {
            const auto capture = match.captures[i];
            const auto node = capture.node;
            const auto start_point = ts_node_start_point(node);
            const auto end_point = ts_node_end_point(node);

            const auto token_id = p_current_parser->getTokenId(capture.index);
            if (token_id == TokenId::None) {
                continue;
            }

            // A node may start before or end after the painted range; clamp its lines to it
            const auto first_line = std::max(start_point.row, firstLine);
            const auto last_line = std::min(end_point.row, lastLine);
            for (auto current = first_line; current <= last_line; ++current) {
                const auto current_line = m_cursor.getString(current);
                const auto start_col = current == start_point.row ? start_point.column / sizeof(char16_t) : 0;
                const auto end_col = current == end_point.row ? end_point.column / sizeof(char16_t) : current_line.length();

                auto &cells = m_line_cache[current - m_cache_start_line];
                if (cells.empty()) {
                    cells.resize(current_line.length(), TokenId::None);
                }

                // Paint first-wins: earlier captures keep priority over later overlapping ones
                const auto paint_end = std::min(end_col, cells.size());
                const auto paint_start = std::min(start_col, paint_end);
                std::replace(cells.begin() + static_cast<std::ptrdiff_t>(paint_start), cells.begin() + static_cast<std::ptrdiff_t>(paint_end), TokenId::None, token_id);
            }
        }
    }
}

void HighLighter::repaintChangedLines(TSTree *newTree) {
    const auto cache_first = m_cache_start_line;
    const auto cache_last = m_cache_start_line + static_cast<uint32_t>(m_line_cache.size()) - 1;

    // Cover the accumulated edit span, clipped to the cached window
    auto repaint_first = std::numeric_limits<uint32_t>::max();
    auto repaint_last = std::numeric_limits<uint32_t>::min();
    if (m_dirty_line_min <= cache_last && m_dirty_line_max >= cache_first) {
        repaint_first = std::max(m_dirty_line_min, cache_first);
        repaint_last = std::min(m_dirty_line_max, cache_last);
    }

    // Union with the structural changes reported by tree-sitter (e.g. an edit commenting out lines below)
    uint32_t range_count = 0;
    auto *ranges = ts_tree_get_changed_ranges(p_ts_tree, newTree, &range_count);
    for (uint32_t i = 0; i < range_count; ++i) {
        const auto range_first = ranges[i].start_point.row;
        const auto range_last = ranges[i].end_point.row;
        if (range_first > cache_last || range_last < cache_first) {
            continue;
        }

        repaint_first = std::min(repaint_first, std::max(range_first, cache_first));
        repaint_last = std::max(repaint_last, std::min(range_last, cache_last));
    }
    free(ranges);

    if (repaint_first > repaint_last) {
        // No changed line intersects the cached window
        return;
    }

    // Cleared rows are lazily resized to their line's current length while painting
    for (auto current = repaint_first; current <= repaint_last; ++current) {
        m_line_cache[current - m_cache_start_line].clear();
    }

    paintCacheLines(ts_tree_root_node(newTree), repaint_first, repaint_last);
}

bool HighLighter::shiftLineCache(const BufferEdit &edit) {
    const auto delta = static_cast<int64_t>(edit.new_end.line) - static_cast<int64_t>(edit.old_end.line);
    if (m_line_cache.empty() || delta >= CACHE_LINE_COUNT || delta <= -static_cast<int64_t>(CACHE_LINE_COUNT)) {
        // Nothing cached, or the edit displaces more rows than the window holds: rebuilding is cheaper.
        return false;
    }

    const auto cache_first = m_cache_start_line;
    const auto cache_last = cache_first + static_cast<uint32_t>(m_line_cache.size()) - 1;

    if (edit.start.line > cache_last) {
        // Entirely below the window: every cached row keeps both its content and its line number.
        return true;
    }

    if (edit.old_end.line < cache_first) {
        // Entirely above the window: the rows are untouched, only the line they describe moves.
        const auto shifted = static_cast<int64_t>(cache_first) + delta;
        m_cache_start_line = shifted > 0 ? static_cast<uint32_t>(shifted) : 0;
        return true;
    }

    if (delta > 0) {
        // Lines old_end.line + 1 .. new_end.line are new: open a blank row for each of them. They all
        // fall inside the dirty span, so paintCacheLines resizes and fills them on the next parse.
        const auto position = std::min(static_cast<size_t>(edit.old_end.line + 1 - cache_first), m_line_cache.size());
        m_line_cache.insert(m_line_cache.begin() + static_cast<ptrdiff_t>(position), static_cast<size_t>(delta), std::vector<TokenId>{});

        if (m_line_cache.size() > CACHE_LINE_COUNT) {
            // Keep the window at its nominal size; the rows past it are rebuilt on demand.
            m_line_cache.resize(CACHE_LINE_COUNT);
        }

        return true;
    }

    // Lines new_end.line + 1 .. old_end.line are gone: drop their rows, clipped to the cached window.
    const auto removed_first = std::max(edit.new_end.line + 1, cache_first);
    const auto removed_last = std::min(edit.old_end.line, cache_last);
    const auto erase_first = std::min(static_cast<size_t>(removed_first - cache_first), m_line_cache.size());
    const auto erase_last = std::min(static_cast<size_t>(removed_last - cache_first) + 1, m_line_cache.size());
    if (erase_first >= erase_last) {
        // Every removed line sits past the window: the cached rows are untouched.
        return true;
    }

    m_line_cache.erase(m_line_cache.begin() + static_cast<ptrdiff_t>(erase_first), m_line_cache.begin() + static_cast<ptrdiff_t>(erase_last));

    if (m_line_cache.empty()) {
        m_cache_start_line = 0;
    } else if (erase_first == 0) {
        // The window lost its first rows: the line following the erased span now opens it.
        m_cache_start_line = edit.new_end.line + 1;
    }

    return true;
}

void HighLighter::edit(const BufferEdit &edit) {
    if (p_ts_tree != nullptr) {
        // This just converts and relays the object coming from the cursor. The column products
        // widen to size_t through sizeof, so they re-enter tree-sitter's 32-bit space explicitly.
        const auto ts_edit = TSInputEdit {
            .start_byte = edit.start_byte,
            .old_end_byte = edit.old_end_byte,
            .new_end_byte = edit.new_end_byte,
            .start_point = TSPoint{.row = edit.start.line, .column = static_cast<uint32_t>(edit.start.column * sizeof(char16_t))},
            .old_end_point = TSPoint{.row = edit.old_end.line, .column = static_cast<uint32_t>(edit.old_end.column * sizeof(char16_t))},
            .new_end_point = TSPoint{.row = edit.new_end.line, .column = static_cast<uint32_t>(edit.new_end.column * sizeof(char16_t))}
        };

        ts_tree_edit(p_ts_tree, &ts_edit);

        // Accumulate the dirty line span until the next parse consumes it
        const auto delta = static_cast<int64_t>(edit.new_end.line) - static_cast<int64_t>(edit.old_end.line);
        if (delta != 0) {
            // The cached rows survive a line-count change as long as they can be realigned with it.
            if (! shiftLineCache(edit)) {
                m_edit_lines_shifted = true;
            }

            // The span the previous edits accumulated is expressed in the line numbering this edit
            // just changed: move the ends sitting below the edit so they still point at their lines.
            if (m_dirty_line_min != std::numeric_limits<uint32_t>::max() && m_dirty_line_min > edit.old_end.line) {
                m_dirty_line_min = static_cast<uint32_t>(static_cast<int64_t>(m_dirty_line_min) + delta);
            }
            if (m_dirty_line_max > edit.old_end.line) {
                m_dirty_line_max = static_cast<uint32_t>(static_cast<int64_t>(m_dirty_line_max) + delta);
            }
        }

        m_dirty_line_min = std::min(m_dirty_line_min, edit.start.line);
        m_dirty_line_max = std::max({m_dirty_line_max, edit.old_end.line, edit.new_end.line});
        m_is_dirty = true;
    }
}

bool HighLighter::isSupported(const std::string_view extension) {
    if (ParserCatalog::findModeByExtension(extension) != HighLightId::None) {
        return true;
    }

    // So the set_hl_command can work with "txt"
    return extension == ".txt" || extension == ".TXT";
}

void HighLighter::getParserCompletions(const AutoCompleteCallback &callback) {
    // Add a "txt" item, for HighLightId::None
    callback(u"txt");
    for (const auto &descriptor: std::views::values(ParserCatalog::getDescriptors())) {
        const auto utf16_argument_value = utf8::utf8to16(descriptor.argument_value);
        callback(utf16_argument_value);
    }
}

std::span<const TokenId> HighLighter::getHighLightLine(const uint32_t line) const {
    if (m_high_light == HighLightId::None || p_ts_tree == nullptr || line >= m_cursor.getLineCount()) {
        return {};
    }

    if (line < m_cache_start_line || line >= m_cache_start_line + m_line_cache.size()) {
        updateCache(line);
    }

    return m_line_cache[line - m_cache_start_line];
}

std::optional<std::u16string_view> HighLighter::readCallback(const uint32_t line, const uint32_t column) const {
    const auto line_count = m_cursor.getLineCount();
    if (line >= line_count) {
        return std::nullopt;
    }

    const auto string = m_cursor.getString(line);
    if (column < string.length()) {
        return string.substr(column);
    }

    // Tells the parser there is more
    return u"\n";
}

const char *HighLighter::inputCallback(void *payload, const uint32_t byteIndex, const TSPoint position, uint32_t *bytesRead) {
    // This input callback is working with logical positions, so byteIndex is not needed
    (void) byteIndex;

    // Get the HighLighter instance
    const auto *self = static_cast<HighLighter *>(payload);

    // Multiply and divide column according to char size, we are working with char16_t (2 bytes)
    const auto line = position.row;
    const auto column = static_cast<uint32_t>(position.column / sizeof(char16_t));
    if (const auto &optional_line = self->readCallback(line, column)) {
        // We got some data
        *bytesRead = static_cast<uint32_t>(optional_line->length() * sizeof(char16_t));
        return reinterpret_cast<const char*>(optional_line->data());
    }

    // Tells tree-sitter there is no more data to process at this location
    *bytesRead = 0;
    return nullptr;
}
