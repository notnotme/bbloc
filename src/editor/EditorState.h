#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <cstdint>

#include "../core/ViewState.h"


/**
 * @brief Manages layout and scroll state for the editor view.
 *
 * Tracks the scroll offsets and whether the editor should
 * automatically follow the cursor indicator during the next render pass.
 */
class EditorState final : public ViewState {
private:
    /** Horizontal scroll offset in pixels. */
    int32_t m_scroll_x;

    /** Vertical scroll offset in pixels. */
    int32_t m_scroll_y;

    /** Indicates if the view should auto-scroll to the cursor indicator. */
    bool m_follow_indicator;

public:
    /** @brief Constructs an EditorState with default values. */
    explicit EditorState();

    /** @return Current horizontal scroll offset. */
    [[nodiscard]] int32_t getScrollX() const;

    /** @return Current vertical scroll offset. */
    [[nodiscard]] int32_t getScrollY() const;

    /** @return True if the view is set to follow the cursor indicator. */
    [[nodiscard]] bool isFollowingIndicator() const;

    /**
     * @brief Enables or disables follow-to-cursor behavior.
     * @param follow True to enable, false to disable.
     */
    void setFollowIndicator(bool follow);

    /**
     * @brief Sets the horizontal scroll offset.
     * @param x New scroll position on the X axis.
     */
    void setScrollX(int32_t x);

    /**
     * @brief Sets the vertical scroll offset.
     * @param y New scroll position on the Y axis.
     */
    void setScrollY(int32_t y);
};


#endif //EDITOR_STATE_H
