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
#include "LongestLineTracker.h"

#include <algorithm>

#include "TextBuffer.h"


LongestLineTracker::LongestLineTracker()
    : m_max_line(0),
      m_max_length(0),
      m_tab_weight(1),
      m_is_max_dirty(false) {
    m_metrics.emplace_back();
}

void LongestLineTracker::reset() {
    m_metrics.clear();
    m_metrics.emplace_back();
    m_max_line = 0;
    m_max_length = 0;
    m_is_max_dirty = false;
}

LongestLineTracker::LineMetric LongestLineTracker::measureLine(const std::u16string_view line) {
    const auto tab_count = static_cast<uint32_t>(std::count(line.begin(), line.end(), u'\t'));
    return {static_cast<uint32_t>(line.length()), tab_count};
}

uint32_t LongestLineTracker::weightedLength(const LineMetric &metric) const {
    return metric.count + metric.tab_count * (m_tab_weight - 1);
}

void LongestLineTracker::onEdit(const TextBuffer &buffer, const BufferEdit &edit) {
    const auto first_line = edit.start.line;
    const auto old_last_line = edit.old_end.line;
    const auto new_last_line = edit.new_end.line;

    // Realign the metric slots with the buffer's new line structure
    if (new_last_line > old_last_line) {
        m_metrics.insert(m_metrics.begin() + old_last_line + 1, new_last_line - old_last_line, LineMetric{});
    } else if (old_last_line > new_last_line) {
        m_metrics.erase(m_metrics.begin() + new_last_line + 1, m_metrics.begin() + old_last_line + 1);
    }

    const auto touched_max = !m_is_max_dirty && m_max_line >= first_line && m_max_line <= old_last_line;
    if (!m_is_max_dirty && m_max_line > old_last_line) {
        m_max_line = m_max_line - old_last_line + new_last_line;
    }

    // Re-measure the touched lines and find the longest among them
    auto best_line = first_line;
    auto best_length = 0u;
    for (auto line = first_line; line <= new_last_line; ++line) {
        m_metrics[line] = measureLine(buffer.getString(line));
        const auto length = weightedLength(m_metrics[line]);
        if (length > best_length) {
            best_length = length;
            best_line = line;
        }
    }

    if (m_is_max_dirty) {
        return;
    }

    if (touched_max) {
        if (best_length >= m_max_length) {
            m_max_line = best_line;
            m_max_length = best_length;
        } else {
            // The longest line shrank; an untouched line may now be the longest
            m_is_max_dirty = true;
        }
    } else if (best_length > m_max_length) {
        m_max_line = best_line;
        m_max_length = best_length;
    }
}

uint32_t LongestLineTracker::getLongestLineLength(const uint32_t tabWeight) const {
    const auto tab_weight = std::max(tabWeight, 1u);
    if (tab_weight != m_tab_weight) {
        // A new tab weight can change which line is the longest
        m_tab_weight = tab_weight;
        m_is_max_dirty = true;
    }

    if (m_is_max_dirty) {
        rescan();
    }

    return m_max_length;
}

uint32_t LongestLineTracker::getLineTabCount(const uint32_t line) const {
    return m_metrics[line].tab_count;
}

void LongestLineTracker::rescan() const {
    m_max_line = 0;
    m_max_length = 0;
    for (auto line = 0u; line < m_metrics.size(); ++line) {
        const auto length = weightedLength(m_metrics[line]);
        if (length > m_max_length) {
            m_max_length = length;
            m_max_line = line;
        }
    }

    m_is_max_dirty = false;
}
