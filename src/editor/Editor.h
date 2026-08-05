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
#ifndef EDITOR_H
#define EDITOR_H

#include <SDL.h>

#include "../core/base/GlobalRegistry.h"
#include "../core/cvar/CVarBool.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "../core/ViewState.h"
#include "../core/CursorContext.h"


/**
 * @brief Main text editor view responsible for rendering text and handling input.
 *
 * The Editor view manages the rendering the cursor buffer, processing user input,
 * tracking scroll state, everything via CursorContext.
 */
class Editor final : public View<> {
private:
    /** Minimum scrollbar thumb size in pixels, so it stays visible and grabbable on huge contents. */
    static constexpr int64_t MIN_THUMB_SIZE = 16;

    /**
     * @brief Mouse drag interactions the editor can be in between a press and its release.
     */
    enum class MouseDrag : uint8_t {
        None,            ///< No drag in progress.
        Text,            ///< Extending the selection from the pressed cell.
        VerticalThumb,   ///< Dragging the vertical scrollbar thumb.
        HorizontalThumb  ///< Dragging the horizontal scrollbar thumb.
    };

    /**
     * @brief Per-frame invariant geometry shared by the render and mouse paths.
     */
    struct FrameMetrics final {
        int32_t margin_width = 0;         ///< Width of the left margin (paddings + line numbers), border excluded.
        int32_t line_count_width = 0;     ///< Width in pixels of the greatest line number.
        uint32_t longest_line_length = 0; ///< Weighted length of the longest line, in characters.
        int32_t v_bar_width = 0;          ///< Width of the vertical scrollbar, 0 when hidden.
        int32_t h_bar_height = 0;         ///< Height of the horizontal scrollbar, 0 when hidden.
    };

    /**
     * @brief Geometry of one scrollbar axis, shared by the draw and mouse hit-test paths.
     */
    struct ScrollbarMetrics final {
        int64_t view_size = 0;    ///< Visible extent on the axis, in pixels.
        int64_t content_size = 0; ///< Content extent on the axis, in pixels (at least 1).
        int64_t thumb_size = 0;   ///< Thumb extent, in pixels.
        int64_t thumb_origin = 0; ///< Window coordinate where the thumb starts, clamped to the track.
    };

    /** CVar for toggling tab-to-space replacement in input. */
    std::shared_ptr<CVarBool> m_is_tab_to_space;

    /** CVar for toggling the editor scrollbars visibility. */
    std::shared_ptr<CVarBool> m_show_scrollbar;

    /** Mouse drag interaction currently in progress, None outside a left-button press. */
    MouseDrag m_mouse_drag;

    /** Pointer coordinate on the dragged axis when a scrollbar thumb was grabbed. */
    int32_t m_drag_grab;

    /** Scroll offset of the dragged axis when a scrollbar thumb was grabbed. */
    int64_t m_drag_scroll;

    /** Line of the last cell placed by a mouse press or drag, to skip redundant updates. */
    uint32_t m_drag_line;

    /** Column of the last cell placed by a mouse press or drag, to skip redundant updates. */
    uint32_t m_drag_column;

private:
    /** @brief Registers the tab_to_space cvar into the command manager. */
    void registerTabToSpaceCVar() const;

    /** @brief Registers the show_scrollbar cvar into the command manager. */
    void registerShowScrollbarCVar() const;

    /**
     * @brief Measures the whole buffer content height, in content-space pixels.
     *
     * The one place the line count enters the 64-bit content space: a 4G-line buffer
     * overflows a 32-bit pixel count.
     *
     * @param context A reference to the cursor context.
     * @return The content height in pixels.
     */
    [[nodiscard]] int64_t contentHeight(const CursorContext &context) const;

    /**
     * @brief Measures the width of the longest line, in content-space pixels.
     *
     * The one place the longest line length enters the 64-bit content space: a 4G-character
     * line overflows a 32-bit pixel count.
     *
     * @param longestLineLength The weighted length of the longest line, in characters.
     * @return The content width in pixels.
     */
    [[nodiscard]] int64_t contentWidth(uint32_t longestLineLength) const;

    /**
     * @brief Measures a prefix of a buffer line, in pixels.
     *
     * Matches the drawText walk exactly — the callers position the cursor indicator and the
     * selection quads with it, so the two must never disagree by a pixel: tabs snap to tab
     * stops, so the measure is only valid for a prefix starting at visual column 0. Uses the
     * per-line tab count the buffer already tracks: a tab-free line (the common case) resolves
     * in O(1) instead of walking what can be a multi-million character prefix per frame.
     *
     * @param context A reference to the cursor context.
     * @param line The line the text belongs to, to query its tab count.
     * @param text The prefix of the line to measure, starting at column 0.
     * @return The measured width in content-space pixels.
     */
    [[nodiscard]] int64_t measureLineText(const CursorContext &context, uint32_t line, std::u16string_view text) const;

    /**
     * @brief Computes the effective scrollbar sizes for the current frame.
     *
     * A bar size is 0 when the scrollbars are disabled or when the content fits on that axis.
     * Called once per frame; every consumer of the frame reuses the resolved sizes.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     * @param longestLineLength The weighted length of the longest line, in characters.
     * @param vBarWidth Receives the width of the vertical scrollbar, 0 when hidden.
     * @param hBarHeight Receives the height of the horizontal scrollbar, 0 when hidden.
     */
    void computeScrollbarSizes(const CursorContext &context, const ViewState &viewState, int32_t marginWidth, uint32_t longestLineLength, int32_t &vBarWidth, int32_t &hBarHeight) const;

    /**
     * @brief Computes the frame-invariant geometry: margin widths, longest line and scrollbar sizes.
     *
     * Both render() and the mouse handlers resolve the same metrics through this helper, so the
     * pixel positions a click maps back to are exactly the ones the render path produced.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @return The resolved frame metrics.
     */
    [[nodiscard]] FrameMetrics computeFrameMetrics(const CursorContext &context, const ViewState &viewState) const;

    /**
     * @brief Computes the thumb geometry of one scrollbar axis.
     *
     * The same math positions the drawn thumb and hit-tests a mouse press against it.
     *
     * @param trackOrigin Window coordinate where the track starts on the axis.
     * @param viewSize Visible extent on the axis, in pixels (must be positive).
     * @param contentSize Content extent on the axis, in pixels.
     * @param scroll Current scroll offset of the axis.
     * @return The resolved scrollbar metrics.
     */
    [[nodiscard]] static ScrollbarMetrics computeScrollbarMetrics(int64_t trackOrigin, int64_t viewSize, int64_t contentSize, int64_t scroll);

    /**
     * @brief Maps the pointer travel of a thumb drag back to a scroll offset.
     *
     * Requests a redraw only when the resulting offset differs from the current one.
     *
     * @param scroll Reference to the scroll offset of the dragged axis.
     * @param pointer Window coordinate of the pointer on the dragged axis.
     * @param bar The scrollbar metrics of the dragged axis.
     * @param context A reference to the cursor context, to request the redraw.
     */
    void applyThumbDrag(int64_t &scroll, int32_t pointer, const ScrollbarMetrics &bar, CursorContext &context) const;

    /**
     * @brief Converts a pixel offset inside the text area to a column of the given line.
     *
     * Walks the line with the same advances the render path uses (tabs included), starting with
     * the same fast-skip as drawText over the tab-free prefix. A pixel on the right half of a
     * character resolves to the column after it; a pixel past the end of the line clamps to eol.
     *
     * @param context A reference to the cursor context.
     * @param line The line index to resolve the column on.
     * @param targetX Content-space pixel offset from the text origin (scroll already applied).
     * @return The resolved column index.
     */
    [[nodiscard]] uint32_t columnAtPixel(const CursorContext &context, uint32_t line, int64_t targetX) const;

    /**
     * @brief Places the cursor at the cell under a window pixel, optionally extending the selection.
     *
     * Follows the same bookkeeping as the `move` command: resets the search statistics, arms or
     * disarms the selection before moving (the anchor stays at the pressed cell), sticks to the
     * new column and scrolls the indicator back into view. Does nothing when a drag stays in the
     * cell it already placed.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param metrics The frame metrics of the current geometry.
     * @param x Window-relative x coordinate, in pixels.
     * @param y Window-relative y coordinate, in pixels.
     * @param extendSelection True to extend the selection to the new cell, false to disarm it.
     */
    void placeCursorAtPixel(CursorContext &context, const ViewState &viewState, const FrameMetrics &metrics, int32_t x, int32_t y, bool extendSelection);

    /**
     * @brief Draw the visual-only scrollbars (tracks and thumbs) of the editor.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     * @param vBarWidth The width of the vertical scrollbar, 0 when hidden.
     * @param hBarHeight The height of the horizontal scrollbar, 0 when hidden.
     * @param longestLineLength The weighted length of the longest line, in characters.
     */
    void drawScrollbars(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, int32_t marginWidth, int32_t vBarWidth, int32_t hBarHeight, uint32_t longestLineLength) const;

    /**
     * @brief: Compute scroll position and max scroll for the horizontal and vertical axis.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     * @param vBarWidth The width of the vertical scrollbar, 0 when hidden.
     * @param hBarHeight The height of the horizontal scrollbar, 0 when hidden.
     * @param longestLineLength The weighted length of the longest line, in characters.
     */
    void updateScroll(CursorContext &context, const ViewState &viewState, int32_t marginWidth, int32_t vBarWidth, int32_t hBarHeight, uint32_t longestLineLength) const;

    /**
     * @brief: Draw the background layer of the editor.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawBackground(QuadBuffer &quadBuffer, const ViewState &viewState, int32_t marginWidth) const;

    /**
     * @brief: Draw the text layer in the left margin of the editor.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param lineCountWidth The width in pixel of the greatest line number.
     * @param scrollY The editor y scroll offset.
     */
    void drawMarginText(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, int32_t lineCountWidth, int64_t scrollY) const;

    /**
     * @brief Draw the text layer (glyphs, selection, and cursor indicator) of the editor.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param scrollX The editor x scroll offset.
     * @param scrollY The editor y scroll offset.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawText(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, int64_t scrollX, int64_t scrollY, int32_t marginWidth) const;

public:
    /**
     * @brief Constructs the Editor view.
     *
     * @param commandController Reference to the CommandController.
     * @param theme Reference to the Theme for rendering.
     * @param quadProgram Reference to the QuadProgram shader.
     */
    explicit Editor(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram);

    /**
     * @brief Renders the text editor to the screen.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param quadBuffer Reference to the quad buffer used to build this frame's geometry.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, ViewState &viewState, QuadBuffer &quadBuffer, float dt) override;

    /**
     * @brief Handles key down events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param keyCode SDL key code.
     * @param keyModifier Modifier bitmask (Shift, Ctrl, etc).
     * @return True if the event was handled.
     */
    bool onKeyDown(CursorContext &context, ViewState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Handles text input events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param text UTF-8 encoded character input from SDL_TEXTINPUT.
     */
    void onTextInput(CursorContext &context, ViewState &viewState, const char* text) const override;

    /**
     * @brief Handles a left mouse button press in the editor.
     *
     * A press on a scrollbar thumb starts a thumb drag; a press on a scrollbar track jumps the
     * scroll by one page toward the click. Any other press places the caret at the clicked
     * character, disarms the selection and starts a text-selection drag. The keyboard focus is
     * left untouched, so a pending prompt interaction survives mouse use.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param x Window-relative x coordinate of the press, in pixels.
     * @param y Window-relative y coordinate of the press, in pixels.
     */
    void onMouseDown(CursorContext &context, ViewState &viewState, int32_t x, int32_t y) override;

    /**
     * @brief Handles mouse motion during a left-button drag started in the editor.
     *
     * Extends the selection from the pressed cell, or moves the grabbed scrollbar thumb.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param x Window-relative x coordinate of the pointer, in pixels.
     * @param y Window-relative y coordinate of the pointer, in pixels.
     */
    void onMouseMotion(CursorContext &context, ViewState &viewState, int32_t x, int32_t y) override;

    /**
     * @brief Ends the mouse drag in progress, if any.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param x Window-relative x coordinate of the release, in pixels.
     * @param y Window-relative y coordinate of the release, in pixels.
     */
    void onMouseUp(CursorContext &context, ViewState &viewState, int32_t x, int32_t y) override;
};


#endif //EDITOR_H
