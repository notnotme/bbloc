#include "EditorState.h"

#include <algorithm>


EditorState::EditorState()
    : m_scroll_x(0),
      m_scroll_y(0),
      m_follow_indicator(false) {}

int32_t EditorState::getScrollX() const {
    return m_scroll_x;
}

int32_t EditorState::getScrollY() const {
    return m_scroll_y;
}

bool EditorState::isFollowingIndicator() const {
    return m_follow_indicator;
}

void EditorState::setFollowIndicator(const bool follow) {
    m_follow_indicator = follow;
}

void EditorState::setScrollX(const int32_t x) {
    m_scroll_x = x;
}

void EditorState::setScrollY(const int32_t y) {
    m_scroll_y = y;
}
