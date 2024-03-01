#pragma once

#include <memory>
#include <codecvt>
#include <locale>
#include <list>
#include <chrono>
#include <glm/glm.hpp>
#include "renderer/sprite/SpriteBuffer.h"
#include "renderer/sprite/SpriteShader.h"
#include "renderer/FontTexture.h"
#include "Cursor.h"

// TODO: Try to create something like RendererState to wrap all Precalc so internal state can be saved/restored when switching cursor
// for scroll position etc

class CursorRenderer {
public:

    static const char16_t FULL_BLOCK_CHARACTER  = u'█';
    static const char16_t MIDDLE_DOT_CHARACTER  = u'•';
    static const char16_t ARROW_CHARACTER       = u'▸';

    /// @brief Bitfield used to define what should be draw by the renderer
    enum DrawBit : uint8_t {
        /// @brief Enable / Disable the scissor
        SCISSOR                 = 1,
        /// @brief Draw the left margin (including indicator and line number)
        LEFT_MARGIN             = 2,
        /// @brief Draw line number (LEFT_MARGIN is mandatory for this one)
        LINE_NUMBER             = 4,
        /// @brief Draw line number indicator (LEFT_MARGIN is mandatory for this one)
        LINE_NUMBER_INDICATOR   = 8,
        /// @brief Fill the current line background
        HIGHTLIGHT_CURRENT_LINE = 16,
        /// @brief Draw scrollbars and scroll indicators (to be removed once UI is ready)
        SCROLL_BAR              = 32
    };

    // @brief Define what line indicator to use
    enum LineIndicator : char16_t {
        DOT         = MIDDLE_DOT_CHARACTER,
        ARROW       = ARROW_CHARACTER,
        LINE        = u'-'
    };

    /// @brief A structure containing the style used by the renderer
    struct Style {
        /// @brief The background color
        glm::u8vec4     backgroundColor             = { 255, 255, 255, 255 };
        /// @brief The caret color
        glm::u8vec4     caretColor                  = { 0, 0, 0, 255 };
        /// @brief The normal text color
        glm::u8vec4     textColor                   = { 0, 0, 0, 200 };
        /// @brief The selected text color
        glm::u8vec4     selectedTextColor           = { 0, 0, 255, 200 };
        /// @brief The margin color
        glm::u8vec4     marginColor                 = { 240, 240, 240, 255 };
        /// @brief The margin border color
        glm::u8vec4     marginBorderColor           = { 160, 160, 160, 255 };
        /// @brief The line number color
        glm::u8vec4     lineNumberColor             = { 0, 0, 0, 127 };
        /// @brief The current line number color
        glm::u8vec4     currentLineNumberColor      = { 0, 0, 0, 255 };
        /// @brief The current line background color
        glm::u8vec4     currentLinebackgroundColor  = { 220, 220, 220, 100 };
        /// @brief The scrollbars color
        glm::u8vec4     scrollbarColor              = { 230, 230, 230, 255 };
        /// @brief The scrollbars indicator color
        glm::u8vec4     scrollbarIndicatorColor     = { 180, 180, 180, 255 };
        /// @brief The line indicator to display
        LineIndicator   lineIndicator               = DOT;
        /// @brief The border size of the left margin
        uint32_t        marginBorderWidth           = 1;
        /// @brief The width of scrallbar
        uint32_t        scrollbarWidth              = 16;
        /// @brief The caret width
        uint8_t         caretWidth                  = 2;
    };

    CursorRenderer();
    virtual ~CursorRenderer();

    /// @brief Initialize the renderer, must be called prior to use
    void initialize();

    /// @brief Finalize the renderer, must be called before forgeting it
    void finalize() const;

    /// @brief Bind the renderer to a FontTexture object
    /// @param fontTexture The FontTexture that will the font data to the renderer
    void bind(const std::shared_ptr<FontTexture> fontTexture);

    /// @brief Bind the renderer to a Cursor object
    /// @param cursor The Cursor that should be rendered
    void bind(const std::shared_ptr<Cursor> cursor);

    /// @brief Update the DrawingBox dimension/position (origin centered)
    /// @param xPixel The position on the X axis in pixels
    /// @param yPixel The position on the Y axis in pixels
    /// @param widthPixel The width of the drawing box
    /// @param heightPixel The height of the DrawingBox
    void updateDrawingBox(float xPixel, float yPixel, float widthPixel, float heightPixel);

    /// @brief Update the parent window size inside the renderer
    /// @param widthPixel The new width of the window (in pixels)
    /// @param heightPixel The new height of the window (in pixels)
    void updateWindowSize(float widthPixel, float heightPixel);

    /// @brief Update the internal state of the renderer, this will clear all dirty bits
    /// @param time Something that represent the elapsed time, in seconds.
    void update(float time);

    /// @brief Update the renderer style
    /// @param style The new style
    void style(const Style style);

    /// @brief Enable a DrawBit
    /// @param bit The DrawBit to enable
    void enableDrawBit(DrawBit bit);

    /// @brief Disable a DrawBit
    /// @param bit The DrawBit to disable
    void disableDrawBit(DrawBit bit);

    /// @brief Test if the requested DrawBit is enabled
    /// @return true if enabled, false otherwise
    bool drawBit(DrawBit bit) const;

    /// @brief Render the cursor into the DrawingBox
    void render();

    /// @brief Activate all dirty bit, to recompute internal states
    void invalidate();

    /// @brief Scroll by a specified amount of pixels
    /// @param x The x value to add to scroll X
    /// @param y  The y value to add to scroll Y
    void scrollBy(float x, float y);

    /// @brief Scroll to a specified amount of pixels
    /// @param x The x value of scroll X
    /// @param y  The y value of scroll Y
    void scrollTo(float x, float y);

    /// @brief Get a copy of the current style used
    /// @return The current style
    Style style() const;

    /// @brief Return the current scroll value
    /// @return The current scroll value, cannot be negtive (even if the renderer itself support negative scroll)
    glm::u32vec2 scroll() const;

private:

    /// @brief Allow debug to inspect
    friend class Debug;

    /// @brief Disallow copy
    CursorRenderer(const CursorRenderer& copy);
    /// @brief Disallow copy
    CursorRenderer& operator=(const CursorRenderer&);

    /// @brief When the cursor change the text this is used to trigger different check in the renderer
    enum CheckLine : uint8_t {
        CHANGED = Cursor::EventType::LINE_CHANGED,
        CREATED = Cursor::EventType::LINE_CREATED,
        DELETED = Cursor::EventType::LINE_DELETED
    };

    /// @brief A drawing box containing the display information
    struct DrawingBox {
        /// @brief The position in screen coordinates of the drawing box (origin centered)
        glm::vec2   position;
        /// @brief The size of the drawing box
        glm::vec2   size;
        /// @brief The parent size of the drawing box (the window size)
        glm::vec2   parentSize;
        /// @brief The viewport left, top, right, and bottom position
        glm::vec4   viewport;
    };

    /// @brief Hold the renderer precalculated data related to the vertices count and buffers
    struct RenderPrecalc {
        /// @brief The number of vertices for the background
        uint32_t    backgroundGlyphCount;
        /// @brief The number of vertices for the line numbers and indicator
        uint32_t    lineNumberGlyphCount;
        /// @brief The number of vertices for the text
        uint32_t    textGlyphCount;
        /// @brief The number of vertices for the selection
        uint32_t    selectionGlyphCount;
        /// @brief The vertice index of the caret inside the sprite buffer
        uint32_t    caretGlyphIndex;
        /// @brief The precalculated view matrix of the renderer
        glm::mat4   viewMatrix;
        /// @brief A string converter used for line numbers
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    };

    /// @brief Hold some precalculated dimension
    struct DimenPrecalc {
        /// @brief The number of visible line in the drawing box
        uint32_t    visibleLineCount;
        /// @brief The line indicator width
        float       lineIndicatorWidth;
        /// @brief The longuest lines number pixels width
        float       lineNumberWidth;
        /// @brief The maximum scrolling value possible with the binded cursor
        glm::vec2   maxScroll;
        /// @brief The main text bounds, left top, right and bottom.
        glm::vec4   textBounds;
        /// @brief A list containing the pair of line associated with their width in pixels, must always be sorted so the longuest is in the back
        std::list<std::pair<size_t, float>> lineWidth;
    };

    /// @brief Hold the caret datas
    struct CaretPrecalc {
        /// @brief Is currently visible or not
        bool            visible;
        /// @brief The caret blink rate, in milliseconds
        float           blinkTime;
        /// @brief The caret next blink time
        float           nextBlinkTime;
        /// @brief The block glyph Y bearing value
        float           bearingY;
        /// @brief The caret block size in pixels
        glm::vec2       size;
        /// @brief The position of the caret
        glm::vec2       position;
    };

    /// @brief Bitfield used to define what changed in the renderer between two frames
    enum DirtyBit : uint16_t {
        /// @brief The font changed, need to regenerate the caret precalc
        CALCULATE_CARET_PRECALC                 = 1,
        /// @brief Need to update the max scrolling possible position
        CALCULATE_MAX_SCROLL                    = 2,
        /// @brief Need to recalculate the longuest line number width
        CALCULATE_LONGUEST_LINE_NUMBER          = 4,
        /// @brief Need to recalculate the whole list of line widths
        CALCULATE_ALL_LINE_WIDTH                = 8,
        /// @brief Need to update the caret position
        CALCULATE_CARET_POSITION                = 16,
        /// @brief Need to recalculate the number of line visible in the drawing box
        CALCULATE_LINE_IN_VIEW                  = 32,
        /// @brief The caret moved and the view may need to scroll to follow it
        TRY_SCROLL_TO_BORDERS                   = 64,
        /// @brief The style changed
        STYLE_CHANGED                           = 128,
        /// @brief The text changed
        TEXT_CHANGED                            = 256,
        /// @brief The line width list need to be sorted
        REORDER_LINE_WIDTH                      = 512,
        /// @brief Invalidate all sprite shape, forcing them to reposition, resize and set color
        INVALIDATE_SPRITES                      = 1024,
        /// @brief Invalidate the scroll indicators
        INVALIDATE_SCROLL_INDICATORS            = 2048
    };

    /// @brief The SpriteShader used to render the SpriteBuffer
    std::unique_ptr<SpriteShader> mSpriteShader;

    /// @brief The SpriteBuffer containing all block and characters to render
    std::unique_ptr<SpriteBuffer> mSpriteBuffer;

    /// @brief The current Cursor associated with the renderer
    std::shared_ptr<Cursor> mCursor;

    /// @brief The GlyphTexture associated with the renderer, to provide font data
    std::shared_ptr<FontTexture> mFontTexture;

    /// @brief The left margin sprite
    SpriteVertex mMarginSprite;

    /// @brief The border sprite, next to the left margin
    SpriteVertex mBorderSprite;

    /// @brief The main text background sprite
    SpriteVertex mBackgroundSprite;

    /// @brief The horizontal scrollbar background sprite (to be removed)
    SpriteVertex mHScrollSprite;

    /// @brief The horizontal scrollbar sprite indicator (to be removed)
    SpriteVertex mHScrollIndicatorSprite;

    /// @brief The vertical scrollbar background sprite (to be removed)
    SpriteVertex mVScrollSprite;

    /// @brief The vertical scrollbar sprite indicator (to be removed)
    SpriteVertex mVScrollIndicatorSprite;

    /// @brief The sprite to fill the bottom right corner of the window  (to be removed)
    SpriteVertex mCornerSprite;

    /// @brief The DrawingBox (everything render in it)
    DrawingBox mDrawingBox;

    /// @brief The rendering precalc data
    RenderPrecalc mRenderPrecalc;

    /// @brief The dimension precalc data
    DimenPrecalc mDimenPrecalc;

    /// @brief The caret precalc data
    CaretPrecalc mCaretPrecalc;

    /// @brief The style of the renderer
    Style mStyle;

    /// @brief The current scroll position of the renderer, this is not the cursor position
    glm::vec2 mScroll;

    /// @brief Define what is enabled to draw
    uint8_t mDrawBit;

    /// @brief Define what changed in the internal state between two frames
    uint16_t mDirtyBit;

    /// @brief The time used to update the renderer
    float mUpdateTime;

    /// @brief The time used to render the cursor
    float mRenderTime;

    /// @brief Reset the sprites texture coordinates
    void invalidateSpriteTextures();

    /// @brief Recalculate all background sprite position, size and color
    void invalidateSprite();

    /// @brief Recalculate the scrollbar indicator position, size and color
    void invalidateScrollIndicators();

    /// @brief Recalculate the caret precalc data
    void calculateCaretPrecalc();

    /// @brief Recalculate the max scrolling values
    void calculateMaxScroll();

    /// @brief Recalculate the line number width
    void calculateLonguestLineNumberPixel();

    /// @brief Recalculate (clear and and populate) the list of line width
    void calculateAllLineWidthPixels();

    /// @brief Update the carret position
    void calculateCaretPosition();

    /// @brief Calculate the number of visible line in the view
    void calculateLineInView();

    /// @brief Scroll to a border if possible, to follow the caret
    void scrollCaretToBorder();

    /// @brief Check a line inside the cursor and trigger dirty bits if needed
    /// @param index The line number to check
    /// @param check The kind of check to use
    void checkLine(size_t index, CheckLine check);

    /// @brief Sortline by their widths
    /// @param left The left pair
    /// @param right The right pair
    /// @return The result of the comparison (see std::sort)
    static bool sortBySize(const std::pair<const size_t, float>& left, const std::pair<const size_t, float>& right) {
        return left.second < right.second;
    }

};
