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

// TODO: Try to abstract this a bit. Ideally, we want to end up with something  like a list of draw command to execute
// Anyway most precalc data can be shared between implementation and all we need is texture and buffers so the abstraction
// can be elsewhere in the end

// TODO: Try to create something like RendererState so internal state can be saved/restored when switching cursor
// for scroll position, and precalc data (to speed up things)
class CursorRenderer {
public:

    static const char16_t FULL_BLOCK_CHARACTER  = u'█';
    static const char16_t MIDDLE_DOT_CHARACTER  = u'•';
    static const char16_t ARROW_CHARACTER       = u'▸';

    /// @brief Bitfield used to define what should be drawed by the renderer
    enum DrawBit : uint8_t {
        /// @brief Enable / Disable scissor
        SCISSOR                 = 1,
        /// @brief Enable / Disable the draw of left margin
        LEFT_MARGIN             = 2,
        /// @brief Enable / Disable the draw of line number (LEFT_MARGIN is mandatory for this one)
        LINE_NUMBER             = 4,
        /// @brief Enable / Disable the draw of line number indicator (LEFT_MARGIN is mandatory for this one)
        LINE_NUMBER_INDICATOR   = 8,
        /// @brief Enable / Disable the draw of scrollbars and scroll indicators
        SCROLL_BAR              = 16,
        /// @brief Show the status bar
        STATUS_BAR              = 32,
        /// @brief Color the current line background
        HIGHTLIGHT_CURRENT_LINE = 64
    };

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
        /// @brief The line number color
        glm::u8vec4     currentLineNumberColor      = { 0, 0, 0, 255 }; 
        /// @brief The line number color
        glm::u8vec4     currentLinebackgroundColor  = { 220, 220, 220, 100 }; 
        /// @brief The scrollbars color
        glm::u8vec4     scrollbarColor              = { 230, 230, 230, 255 }; 
        /// @brief The scrollbars indicator color
        glm::u8vec4     scrollbarIndicatorColor     = { 180, 180, 180, 255 }; 
        /// @brief The line indicator to use
        LineIndicator   lineIndicator               = DOT;
        /// @brief The border size in pixels of the margin
        uint32_t        marginBorderWidth           = 1;
        /// @brief Width of scrallbar
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
    
    /// @brief Bind the renderer to a Cursor object (this will make the renderer dirty)
    /// @param cursor The cursor to bind
    void bindTo(const std::shared_ptr<Cursor> cursor, const std::shared_ptr<FontTexture> fontTexture);
    
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
    /// @param time Something that represent the elapsed time, in seconds. Current time (now) not needed.
    void update(float time);
    
    /// @brief Update the renderer style
    /// @param style The new style
    void style(Style style);
    
    /// @brief Tells the renderer to enable the drawing of features
    /// @param bit The DrawBit to enable
    void enableDrawBit(DrawBit bit);

    /// @brief Tells the renderer to disable the drawing of features
    /// @param bit The DrawBit to disable
    void disableDrawBit(DrawBit bit);

    /// @brief Tell if the requested DrawBit is enabled
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
    
    /// @brief Get the current style used
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

    enum CheckLine : uint8_t {
        EDITED, CREATED, DELETED
    };

    enum CaretBorder : uint8_t {
        HORIZONTAL  = 1,
        VERTICAL    = 2
    };

    /// @brief A drawing box containing the display information
    struct DrawingBox {
        /// @brief The position in screen coordinates of the drawing box (origin centered)
        glm::vec2       position;
        /// @brief The size of the drawing box
        glm::vec2       size;
        /// @brief The parent size of the drawing box (the window size)
        glm::vec2       parentSize;
        /// @brief The viewport, left, right, bottom and top corrdinates
        glm::vec4       viewport;
    };

    /// @brief Hold the renderer precalculated data related to the vertices count and buffers
    struct RenderPrecalc {
        /// @brief The number of vertices in the ShapeBuffer, for the background
        uint32_t        backgroundGlyphCount;
        /// @brief The number of glyph for the line numbers and indicator 
        uint32_t        lineNumberGlyphCount;
        /// @brief The number of glyph for the text
        uint32_t        textGlyphCount;
        /// @brief The number of glyph for the selection
        uint32_t        selectionGlyphCount;
        /// @brief The number of glyph for the status bar text
        uint32_t        statusBarTextGlyphCount;
        /// @brief The block glyph index inside the glyph buffer
        uint32_t        caretGlyphIndex;
        /// @brief The precalculated view matrix of the renderer
        glm::mat4       viewMatrix;
        /// @brief The longuest line number in string representation
        std::u16string                      longuestLineNumberString;
        /// @brief The caret position in string representation
        std::u16string                      caretPositionString;
        /// @brief The status bar string
        std::u16string                      statusBarString;
        /// @brief A string converter used for line numbers
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    };

    struct DimenPrecalc {
        /// @brief The nuber of visible line in the drawing box
        uint32_t    visibleLineCount;
        /// @brief The margin width (0 if not visible, not the same value from mStyle)
        float       marginWidth;
        /// @brief The status bar height (0 if not visible, not the same value from mStyle)
        float       statusBarHeight;
        /// @brief The line indicator width 
        float       lineIndicatorWidth;
        /// @brief The border width (0 if margin not visible, not the same value from mStyle)
        float       borderWidth;
        /// @brief The scrollbar width, not the same value from mStyle (depending on the scrollbar axis it can be the height)
        float       scrollbarWidth;
        /// @brief The longuest pixels width of lines number
        float       longuestLineNumberWidth;
        /// @brief The maximum scrolling value possible with the associated cursor
        glm::vec2   maxScroll;
        /// @brief A list containing the pair of line associted with their width in pixels, always sorted so the longuest is in the back
        std::list<std::pair<size_t, float>> lineWidth;
    };

    /// @brief Hold the caret datas
    struct CaretPrecalc {
        /// @brief Is currently visible or not
        bool            visible;
        /// @brief The caret blink rate, in milliseconds
        float           blinkRate;
        /// @brief The caret next blink time
        float           nextBlink;
        /// @brief the last caret move direction, from the cursor event
        uint8_t         lastCaretDirection;
        /// @brief The block glyph Y bearing value
        float           blockGlyphBearingY;
        /// @brief The caret block size in pixels
        glm::vec2       size;
        /// @brief The position of the caret
        glm::vec2       position;
        /// @brief The texture coordinates to use with the caret
        glm::vec4       texture;
    };

    /// @brief Bitfield used to define what changed in the renderer betwwen two frames
    enum DirtyBit : uint16_t {
        /// @brief Font changed, need to regenerate the caret precalc
        CALCULATE_CARET_PRECALC                     = 1,
        /// @brief Need to update the max scrolling possible position
        CALCULATE_MAX_SCROLL                    = 2,
        /// @brief Need to recalculate the longuest line number size
        CALCULATE_LONGUEST_LINE_NUMBER_STR      = 4,
        /// @brief Need to recalculate the whole list of line size
        CALCULATE_ALL_LINE_WIDTH                = 8,
        /// @brief Need to update the caret position / attributes in the caret glyph
        CALCULATE_CARET_POSITION                = 16,
        /// @brief The caret moved and the view may need to scroll to follow it
        TRY_SCROLL_TO_BORDERS                   = 32,
        /// @brief Need to recalculate the margin width
        CALCULATE_MARGIN_WIDTH                  = 64,
        /// @brief Need to recalculate the scrollbar width
        CALCULATE_SCROLLBAR_WIDTH               = 128,
        /// @brief Need to recalculate the scrollbar width
        CALCULATE_STATUS_BAR_HEIGHT             = 256,
        /// @brief Need to recalculate the number of line visible in the view
        CALCULATE_LINE_IN_VIEW                  = 512,
        /// @brief The style (color and some size) have changed
        STYLE_CHANGED                           = 1024,
        /// @brief The scroll position changed
        SCROLL_POSITION_CHANGED                 = 2048,
        /// @brief The scroll position changed
        TEXT_CHANGED                            = 4096,
        /// @brief The line width list need to be sorted
        REORDER_LINE_WIDTH                      = 8192,
        /// @brief Generate caret position string
        GENERATE_CARET_POSITION_STR             = 16384,
        /// @brief Generate status bar string
        GENERATE_STATUS_BAR_STR                 = 32768
    };
    
    /// @brief The GlyphShader used to render the GlyphBuffer
    std::unique_ptr<SpriteShader> mSpriteShader;

    /// @brief The GlyphBuffer containing all characters to render
    std::unique_ptr<SpriteBuffer> mSpriteBuffer;

    /// @brief The current cursor associated with the renderer
    std::shared_ptr<Cursor> mCursor;

    /// @brief The GlyphTexture used by the renderer to render characters
    std::shared_ptr<FontTexture> mFontTexture;

    /// @brief The current scroll position of the renderer, this is not the cursor position
    glm::vec2 mScroll;
  
    /// @brief The DrawingBox
    DrawingBox mDrawingBox;

    /// @brief The rendering precalc 
    RenderPrecalc mRenderPrecalc;

    /// @brief The dimension precalc 
    DimenPrecalc mDimenPrecalc;

    /// @brief The caret precalc 
    CaretPrecalc mCaretPrecalc;

    /// @brief The style of the renderer
    Style mStyle;

    /// @brief Define what is enabled to draw
    uint8_t mDrawBit;

    /// @brief Define what changed in the internal state, determine what to update later
    uint32_t mDirtyBit;

    /// @brief The time used to update the renderer
    float mUpdateTime;

    /// @brief The time used to render the bound cursor
    float mRenderTime;

    /// @brief Calculate the caret precalc
    void calculateCaretPrecalc();

    /// @brief Recalculate max scrolling values
    void calculateMaxScroll();

    /// @brief Recalculate the line number width 
    void calculateLonguestLineNumberPixel();

    /// @brief Recalculate (clear and and populate) the list of line width
    void calculateAllLineWidthPixels();

    /// @brief Update the carret position
    void calculateCaretPosition();

    /// @brief Recalculate the margin width
    void calculateMarginWidth();

    /// @brief Recalculate the scrollbar width
    void calculateScrollbarWidth();

    /// @brief Recalculate the status bar height
    void calculateStatusBarHeight();

    /// @brief Calculate the number of visible line in the view
    void calculateLineInView();

    /// @brief Generate the caret position string for the status bar
    void generateCaretPositionString();

    /// @brief Generate the status bar string
    void generateStatusBarString();

    /// @brief Scroll to a border if possible, the caret at the very side of it
    /// @brief border The border to reach (can be both)
    void scrollCaretToBorder(uint8_t border);

    /// @brief Check a line inside the cursor and update Precalc and mDirtyBit if needed
    /// @param index The line number to check
    /// @param newLine True if the line is just created
    void checkLine(size_t index, CheckLine check);

    /// @brief Sort pair by their second value
    /// @param left The left pair
    /// @param right The right pair
    /// @return The result of the comparison (see std::sort)
    static bool sortBySize(const std::pair<const size_t, float>& left, const std::pair<const size_t, float>& right) {
        return left.second < right.second;
    }

};
