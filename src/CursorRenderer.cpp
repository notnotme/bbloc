#include "CursorRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#define SHAPE_BUFFER_SIZE 8192 // Number of Vertex
#define GLYPH_BUFFER_SIZE 8192 // Number of Vertex
#define TEXT_MARGIN 1 // Pixels

CursorRenderer::CursorRenderer() :
mSpriteShader(std::make_unique<SpriteShader>()),
mSpriteBuffer(std::make_unique<SpriteBuffer>(GLYPH_BUFFER_SIZE)),
mScroll(0),
mDrawingBox({ { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0, 0, 0} }),
mRenderPrecalc({ 0, 0, 0, 0, 0, 0, glm::mat4(1) }),
mDimenPrecalc({ 0, 0, 0, 0, 0, 0, 0, { 0, 0 } }),
mCaretPrecalc({ true, 0.5f, 0, 0, 0, { 2, 0 }, { 0, 0 }, { 0, 0, 0, 0 } }),
mDrawBit(SCISSOR | LEFT_MARGIN | LINE_NUMBER | SCROLL_BAR | LINE_NUMBER_INDICATOR | HIGHTLIGHT_CURRENT_LINE | STATUS_BAR),
mDirtyBit(CALCULATE_CARET_PRECALC | CALCULATE_MAX_SCROLL | CALCULATE_LONGUEST_LINE_NUMBER_STR | CALCULATE_ALL_LINE_WIDTH
    | CALCULATE_CARET_POSITION | CALCULATE_LINE_IN_VIEW | CALCULATE_MARGIN_WIDTH | CALCULATE_SCROLLBAR_WIDTH
    | CALCULATE_STATUS_BAR_HEIGHT | GENERATE_CARET_POSITION_STR | GENERATE_STATUS_BAR_STR),
mUpdateTime(0),
mRenderTime(0) {
}

CursorRenderer::~CursorRenderer() {
}

void CursorRenderer::initialize() {
    mSpriteShader->initialize();
    mSpriteBuffer->initialize(mSpriteShader->vertexAttribute(), SpriteBuffer::Usage::DYNAMIC);
}

void CursorRenderer::finalize() const {
    mSpriteShader->finalize();
    mSpriteBuffer->finalize();
}

void CursorRenderer::updateWindowSize(float widthPixel, float heightPixel) {
    mDrawingBox.parentSize.x = widthPixel;
    mDrawingBox.parentSize.y = heightPixel;
    mRenderPrecalc.viewMatrix = glm::ortho(0.0f, (float) widthPixel, (float) heightPixel, 0.0f, 0.0f, 1.0f);
}

void CursorRenderer::updateDrawingBox(float xPixel, float yPixel, float widthPixel, float heightPixel) {
    mDrawingBox.position.x = xPixel;
    mDrawingBox.position.y = yPixel;
    mDrawingBox.size.x = widthPixel;
    mDrawingBox.size.y = heightPixel;
    mDrawingBox.viewport.x = mDrawingBox.position.x - mDrawingBox.size.x / 2.0f;
    mDrawingBox.viewport.y = mDrawingBox.position.x + mDrawingBox.size.x / 2.0f;
    mDrawingBox.viewport.z = mDrawingBox.position.y + mDrawingBox.size.y / 2.0f;
    mDrawingBox.viewport.w = mDrawingBox.position.y - mDrawingBox.size.y / 2.0f;

    mDirtyBit |= CALCULATE_MAX_SCROLL;
    mDirtyBit |= CALCULATE_LINE_IN_VIEW;
    mDirtyBit |= CALCULATE_CARET_POSITION;
}

void CursorRenderer::bindTo(const std::shared_ptr<Cursor> cursor, const std::shared_ptr<FontTexture> fontTexture) {
    mCursor = cursor;
    mFontTexture = fontTexture;
    mDirtyBit = CALCULATE_CARET_PRECALC | CALCULATE_MAX_SCROLL | CALCULATE_LONGUEST_LINE_NUMBER_STR | CALCULATE_ALL_LINE_WIDTH
    | CALCULATE_CARET_POSITION | CALCULATE_LINE_IN_VIEW | CALCULATE_MARGIN_WIDTH | CALCULATE_SCROLLBAR_WIDTH | CALCULATE_STATUS_BAR_HEIGHT
    | GENERATE_CARET_POSITION_STR | GENERATE_STATUS_BAR_STR;
}

void CursorRenderer::update(float time) {
    auto functionStartTime = std::chrono::steady_clock::now();
    if (!mCursor.get() || !mFontTexture.get() || mDrawingBox.size.x <= 0 || mDrawingBox.size.y <= 0) {
        mUpdateTime = 0.0f;
        return;
    }

    // Pump and clear the cursor events
    mCaretPrecalc.lastCaretDirection = 0;
    std::u16string characterNumberText;
    std::u16string lineNumberText;
    const auto caretPosition = mCursor->position();
    auto cursorEvent = mCursor->event();
    while (cursorEvent != nullptr) {
        switch (cursorEvent->type) {
        case Cursor::EventType::CARET_MOVED:
            mDirtyBit |= CALCULATE_CARET_POSITION;
            mDirtyBit |= TRY_SCROLL_TO_BORDERS;
            mDirtyBit |= GENERATE_CARET_POSITION_STR;
            mDirtyBit |= GENERATE_STATUS_BAR_STR;
            // Trigger both caret direction to make it visible on screen if it is out of bounds
            mCaretPrecalc.lastCaretDirection |= VERTICAL;
            mCaretPrecalc.lastCaretDirection |= HORIZONTAL;
        break;
        case Cursor::EventType::LINE_CHANGED:
            checkLine(cursorEvent->data, EDITED);
            mDirtyBit |= TEXT_CHANGED;
        break;
        case Cursor::EventType::LINE_CREATED:
            checkLine(cursorEvent->data, CREATED);
            mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
            mDirtyBit |= GENERATE_STATUS_BAR_STR;
            mDirtyBit |= CALCULATE_MAX_SCROLL;
            mDirtyBit |= TEXT_CHANGED;
        break;
        case Cursor::EventType::LINE_DELETED:
            checkLine(cursorEvent->data, DELETED);
            mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
            mDirtyBit |= GENERATE_STATUS_BAR_STR;
            mDirtyBit |= CALCULATE_MAX_SCROLL;
            mDirtyBit |= TEXT_CHANGED;
        break;
        }

        mCursor->popEvent();
        cursorEvent = mCursor->event();
    }

    // Inspect what changed in the internal state
    auto updateBuffer = false;
    auto updateCaret = false;

    if (mDirtyBit & CALCULATE_CARET_PRECALC) {
        calculateCaretPrecalc();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_ALL_LINE_WIDTH) {
        calculateAllLineWidthPixels();
    }
    if (mDirtyBit & REORDER_LINE_WIDTH) {
        mDirtyBit &= ~REORDER_LINE_WIDTH;
        mDimenPrecalc.lineWidth.sort(sortBySize);
    }
    if (mDirtyBit & CALCULATE_LINE_IN_VIEW) {
        calculateLineInView();
    }
    if (mDirtyBit & CALCULATE_LONGUEST_LINE_NUMBER_STR) {
        calculateLonguestLineNumberPixel();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_MARGIN_WIDTH) {
        calculateMarginWidth();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_SCROLLBAR_WIDTH) {
        calculateScrollbarWidth();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_STATUS_BAR_HEIGHT) {
        calculateStatusBarHeight();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_MAX_SCROLL) {
        calculateMaxScroll();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_CARET_POSITION) {
        calculateCaretPosition();
        updateCaret = true;
    }
    if (mDirtyBit & GENERATE_CARET_POSITION_STR) {
        generateCaretPositionString();
        updateBuffer = true;
    }
    if (mDirtyBit & GENERATE_STATUS_BAR_STR) {
        generateStatusBarString();
        updateBuffer = true;
    }
    if (mDirtyBit & TRY_SCROLL_TO_BORDERS) {
        scrollCaretToBorder(mCaretPrecalc.lastCaretDirection);
        // Passing here indicate that the caret moved, so make it instantly visible
        mCaretPrecalc.visible = true;
        mCaretPrecalc.nextBlink = time + mCaretPrecalc.blinkRate;
        if (mCaretPrecalc.lastCaretDirection & VERTICAL) {
            // Needed to update the indicator position in the margin area
            updateBuffer = true;
        } else {
            updateCaret = true;
        }
    }
    if (mDirtyBit & SCROLL_POSITION_CHANGED) {
        mDirtyBit &= ~SCROLL_POSITION_CHANGED;
        updateBuffer = true;
    }
    if (mDirtyBit & STYLE_CHANGED) {
        mDirtyBit &= ~STYLE_CHANGED;
        updateBuffer = true;
    }
    if (mDirtyBit & TEXT_CHANGED) {
        mDirtyBit &= ~TEXT_CHANGED;
        updateBuffer = true;
    }

    if (time > mCaretPrecalc.nextBlink) {
        mCaretPrecalc.nextBlink = time + mCaretPrecalc.blinkRate;
        mCaretPrecalc.visible = !mCaretPrecalc.visible;
        updateCaret = true;
    }

    SpriteVertex sprite;
    if (updateBuffer) {
        // Inverse scroll now
        auto scroll = -mScroll;
        // Create needed values
        auto lineHeightPixel = mCaretPrecalc.size.y;
        auto left = mDrawingBox.viewport.x;
        auto right = mDrawingBox.viewport.y;
        auto bottom = mDrawingBox.viewport.z;
        auto top = mDrawingBox.viewport.w;
        
        // Reset mRenderPrecalc data
        mRenderPrecalc.backgroundGlyphCount = 0;
        mRenderPrecalc.textGlyphCount = 0;
        mRenderPrecalc.lineNumberGlyphCount = 0;
        mRenderPrecalc.selectionGlyphCount = 0;
        mRenderPrecalc.caretGlyphIndex = 0;
        mRenderPrecalc.statusBarTextGlyphCount = 0;
        
        // Map the whole buffers
        mSpriteBuffer->use();
        mSpriteBuffer->map();

        // Update the background elements
        auto backgroundPosition = mDrawingBox.position;
        auto backgroundSize = mDrawingBox.size;
        sprite.texture = mCaretPrecalc.texture;

        // Maybe draw the status bar
        if (drawBit(STATUS_BAR)) {
            // Upate the background values
            backgroundPosition.y -= mDimenPrecalc.statusBarHeight / 2.0f;
            backgroundSize.y -= mDimenPrecalc.statusBarHeight;

            sprite.position.x = backgroundPosition.x;
            sprite.position.y = bottom - mDimenPrecalc.statusBarHeight / 2.0f;
            sprite.size.x = backgroundSize.x;
            sprite.size.y = mDimenPrecalc.statusBarHeight;
            sprite.tint = mStyle.marginColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            // Decrease bottom
            bottom -= mDimenPrecalc.statusBarHeight;
        }

        // Maybe draw the left margin
        if (drawBit(LEFT_MARGIN)) {
            // Upate the background values
            backgroundPosition.x += (mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth) / 2.0f;
            backgroundSize.x -= mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;

            sprite.position.x = left + mDimenPrecalc.marginWidth / 2.0f;
            sprite.position.y = backgroundPosition.y;
            sprite.size.x = mDimenPrecalc.marginWidth;
            sprite.size.y = backgroundSize.y;
            sprite.tint = mStyle.marginColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            // Maybe draw the border
            if (mStyle.marginBorderWidth > 0) {
                sprite.position.x = left + mDimenPrecalc.marginWidth + mStyle.marginBorderWidth / 2.0f;
                sprite.size.x = mStyle.marginBorderWidth;
                sprite.tint = mStyle.marginBorderColor;
                mSpriteBuffer->add(sprite);
                ++mRenderPrecalc.backgroundGlyphCount;
            }

            // Increase left start
            left += mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;
        }

        // Maybe show the scrollbar
        if (drawBit(SCROLL_BAR) && mStyle.scrollbarWidth > 0) {
            // Upate the background values
            backgroundPosition -= mDimenPrecalc.scrollbarWidth / 2.0f;
            backgroundSize -= mDimenPrecalc.scrollbarWidth;

            auto indicatorWidth = glm::max({ 32.0f, 32.0f }, backgroundSize - mDimenPrecalc.maxScroll);
            auto indicatorPosition = glm::abs(scroll / mDimenPrecalc.maxScroll) * (backgroundSize - indicatorWidth) + indicatorWidth / 2.0f;
            auto halfScrollbarWidth = mDimenPrecalc.scrollbarWidth / 2.0f;

            // Vertical
            sprite.position.x = right - halfScrollbarWidth;
            sprite.position.y = backgroundPosition.y;
            sprite.size.x = mDimenPrecalc.scrollbarWidth;
            sprite.size.y = backgroundSize.y;
            sprite.tint = mStyle.scrollbarColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            sprite.position.y = top + indicatorPosition.y;
            sprite.size.x = mDimenPrecalc.scrollbarWidth;
            sprite.size.y = indicatorWidth.y;
            sprite.tint = mStyle.scrollbarIndicatorColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            // Horizontal
            sprite.position.x = backgroundPosition.x;
            sprite.position.y = bottom - halfScrollbarWidth;
            sprite.size.x = backgroundSize.x;
            sprite.size.y = mDimenPrecalc.scrollbarWidth;
            sprite.tint = mStyle.scrollbarColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            sprite.position.x = left + indicatorPosition.x;
            sprite.size.x = indicatorWidth.x;
            sprite.size.y = mDimenPrecalc.scrollbarWidth;
            sprite.tint = mStyle.scrollbarIndicatorColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;
            
            // Bottom right corner
            sprite.position.x = right - halfScrollbarWidth;
            sprite.position.y = bottom - halfScrollbarWidth;
            sprite.size.x = mDimenPrecalc.scrollbarWidth;
            sprite.size.y = mDimenPrecalc.scrollbarWidth;
            sprite.tint = mStyle.scrollbarColor;
            mSpriteBuffer->add(sprite);
            ++mRenderPrecalc.backgroundGlyphCount;
            
            // Decrease bottom and right
            bottom -= mDimenPrecalc.scrollbarWidth;
            right -= mDimenPrecalc.scrollbarWidth;
        }

        // Draw the editor background
        sprite.position.x = backgroundPosition.x;
        sprite.position.y = backgroundPosition.y;
        sprite.size.x = backgroundSize.x;
        sprite.size.y = backgroundSize.y;
        sprite.tint = mStyle.backgroundColor;
        mSpriteBuffer->add(sprite);
        ++mRenderPrecalc.backgroundGlyphCount;

        // Calculate scroll amount for the text
        auto scrollAmount = glm::abs(scroll.y / lineHeightPixel);
        auto scrollFraction = glm::fract(scrollAmount) * lineHeightPixel;
        auto firstVisibleLine = glm::floor(scrollAmount);
        auto topText = top + mCaretPrecalc.blockGlyphBearingY - scrollFraction;
        auto lastVisibleLine = firstVisibleLine + mDimenPrecalc.visibleLineCount + 2; // one line above and below the visible area must be drawn
        auto visibleText = true;

        if (firstVisibleLine < 0) {
            // Gap between the first line andtop of the screen
            auto skipCount = glm::abs(firstVisibleLine);
            topText += (skipCount * lineHeightPixel);
            if (topText > bottom + lineHeightPixel) {
                // Below visible area
                visibleText = false;
            } else {
                // Always start at line 0 in this case
                firstVisibleLine = 0;
            }
        } else if (firstVisibleLine > mCursor->size()) {
            // Above visible area
            visibleText = false;
        } 

        if (visibleText) {
            // Maybe draw the text in the margin
            if (drawBit(LEFT_MARGIN)) {
                auto printAtX = left - mDimenPrecalc.marginWidth - mDimenPrecalc.borderWidth;
                auto printAtY = topText;

                // Print line number or indicator maybe
                if (drawBit(LINE_NUMBER_INDICATOR) || drawBit(LINE_NUMBER)) {
                    for (size_t lineIndex = firstVisibleLine; lineIndex < lastVisibleLine; ++lineIndex) {
                        if (lineIndex >= mCursor->size() || printAtY > bottom + lineHeightPixel + mDimenPrecalc.scrollbarWidth) {
                            // Stop early if possible
                            break;
                        }

                        bool isCurrentLine = lineIndex == caretPosition.y;
                        if (isCurrentLine && drawBit(LINE_NUMBER_INDICATOR)) {
                            mFontTexture->get(mStyle.lineIndicator, [&](const FontTexture::Tile& tile) {
                                sprite.position.x = glm::floor(printAtX + (tile.bearing.x + tile.size.x / 2.0f));
                                sprite.position.y = glm::round(printAtY - (tile.bearing.y - tile.size.y / 2.0f));
                                sprite.size = tile.size;
                                sprite.texture = tile.texture;
                                sprite.tint = mStyle.currentLineNumberColor;
                                
                                mSpriteBuffer->add(sprite);
                                printAtX += tile.advance;
                                ++mRenderPrecalc.lineNumberGlyphCount;
                            });
                        }

                        if (drawBit(LINE_NUMBER)) {
                            // Don't print line 0 but start with 1 instead
                            auto number = std::to_string(lineIndex + 1);
                            for (auto character : number) {
                                mFontTexture->get(character, [&](const FontTexture::Tile& tile) {
                                    sprite.position.x = glm::floor(printAtX + (tile.bearing.x + tile.size.x / 2.0f));
                                    sprite.position.y =  glm::round(printAtY - (tile.bearing.y - tile.size.y / 2.0f));
                                    sprite.size = tile.size;
                                    sprite.texture = tile.texture;
                                    sprite.tint = isCurrentLine ? mStyle.currentLineNumberColor : mStyle.lineNumberColor;

                                    mSpriteBuffer->add(sprite);
                                    printAtX += tile.advance;
                                    ++mRenderPrecalc.lineNumberGlyphCount;
                                });
                            }
                        }

                        printAtY += lineHeightPixel;
                        printAtX = left - mDimenPrecalc.marginWidth - mDimenPrecalc.borderWidth;
                    }
                }
            }

            // Draw the cursor selection
            auto selection = mCursor->selection();
            if (!selection.empty()) {
                bool selectionStartInViewY = selection.start.y >= firstVisibleLine || selection.start.y < firstVisibleLine + mDimenPrecalc.visibleLineCount;
                bool selectionEndInViewY = selection.end.y >= firstVisibleLine || selection.end.y < firstVisibleLine + mDimenPrecalc.visibleLineCount;
                // check if we are in the view area on the y axis
                if (selectionStartInViewY || selectionEndInViewY) {
                    sprite.texture = mCaretPrecalc.texture;
                    sprite.tint = mStyle.selectedTextColor;
                    sprite.size.y = lineHeightPixel;
                    if (selection.start.y == selection.end.y) {
                        // Start / end on the same line
                        if (selection.end.x < selection.start.x) {
                            // Check if we need to invert X axis
                            std::swap(selection.end.x, selection.start.x);
                        }
                        auto line = mCursor->stringView(selection.start.y);
                        auto startY = (selection.start.y - firstVisibleLine) * lineHeightPixel;
                        startY -= mCaretPrecalc.blockGlyphBearingY - lineHeightPixel / 2.0f;

                        auto selectionStartX = mFontTexture->measure(line.substr(0, selection.start.x)).x;
                        auto selectionWidthPixel = mFontTexture->measure(line.substr(selection.start.x, selection.end.x - selection.start.x)).x;
                        sprite.position.x = (backgroundPosition.x - backgroundSize.x / 2.0f) + (selectionStartX + (selectionWidthPixel / 2.0f)) + scroll.x;
                        sprite.position.y = glm::round(topText + startY);
                        sprite.size.x = selectionWidthPixel;
                        mSpriteBuffer->add(sprite);
                        ++mRenderPrecalc.selectionGlyphCount;
                    } else {
                        // Start / end on multiple lines
                        if (selection.end.y < selection.start.y) {
                            // Check if we need to invert Y axis
                            std::swap(selection.end, selection.start);
                        }
                        for(auto y = selection.start.y; y<=selection.end.y; ++y) {
                            auto line = mCursor->stringView(y);
                            auto startY = (y - firstVisibleLine) * lineHeightPixel;
                            startY -= mCaretPrecalc.blockGlyphBearingY - lineHeightPixel / 2.0f;
                            if (y == selection.start.y) {
                                auto selectionStartX = mFontTexture->measure(line.substr(0, selection.start.x)).x;
                                auto selectionWidthPixel = backgroundSize.x - selectionStartX;
                                sprite.position.x = (backgroundPosition.x - backgroundSize.x / 2.0f) + (selectionStartX + (selectionWidthPixel / 2.0f)) + scroll.x / 2.0f;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = selectionWidthPixel - scroll.x;
                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.selectionGlyphCount;
                            } else if (y == selection.end.y) {
                                auto selectionEndX = mFontTexture->measure(line.substr(0, selection.end.x)).x;
                                sprite.position.x = (backgroundPosition.x - backgroundSize.x / 2.0f) + (selectionEndX / 2.0f) + scroll.x;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = selectionEndX;
                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.selectionGlyphCount;
                            } else {
                                // TODO: merge all lines inbetween start and end
                                sprite.position.x = backgroundPosition.x;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = backgroundSize.x;
                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.selectionGlyphCount;
                            }
                        }
                    }
                }
            }

            // Text
            if (drawBit(HIGHTLIGHT_CURRENT_LINE)) {
                // Check if we are in the view area on the y axis
                if (caretPosition.y >= firstVisibleLine && caretPosition.y < firstVisibleLine + mDimenPrecalc.visibleLineCount) {
                    auto positionInView = (caretPosition.y - firstVisibleLine) * lineHeightPixel;
                    positionInView -= mCaretPrecalc.blockGlyphBearingY - lineHeightPixel / 2.0f;
                    
                    sprite.texture = mCaretPrecalc.texture;
                    sprite.position.x = backgroundPosition.x;
                    sprite.position.y = glm::round(topText + positionInView);
                    sprite.tint = mStyle.currentLinebackgroundColor;
                    sprite.size.x = backgroundSize.x;
                    sprite.size.y = lineHeightPixel;
                    mSpriteBuffer->add(sprite);
                    ++mRenderPrecalc.textGlyphCount;
                }
            }

            // Put the caret in the buffer, keep trace of it's position
            sprite.position = mCaretPrecalc.position;
            sprite.texture = mCaretPrecalc.texture;
            sprite.size.x =  mStyle.caretWidth;
            sprite.size.y = lineHeightPixel;
            if (mCaretPrecalc.visible) {
                sprite.tint = mStyle.caretColor;
            } else {
                sprite.tint = glm::u8vec4(0);
            }
            mRenderPrecalc.caretGlyphIndex = mSpriteBuffer->index();
            mSpriteBuffer->add(sprite);

            auto printAtX = left;
            auto printAtY = topText;
            for (size_t lineIndex = firstVisibleLine; lineIndex < lastVisibleLine; ++lineIndex) {
                if (lineIndex >= mCursor->size() || printAtY > bottom + lineHeightPixel) {
                    // Stop early if possible
                    break;
                }

                auto line = mCursor->stringView(lineIndex);
                for (auto character : line) {
                    mFontTexture->get(character, [&](const FontTexture::Tile& tile) {
                        sprite.position.x = glm::floor(printAtX + (tile.bearing.x + tile.size.x / 2.0f) + scroll.x);
                        sprite.position.y = glm::round(printAtY - (tile.bearing.y - tile.size.y / 2.0f));
                        
                        switch (character) {
                        case u'	':
                            // non printable but advance the caret
                            // Take in account the TAB (0x0009) character (= SPACE width * 4)
                            printAtX += tile.advance * 4;
                            break;
                        default:
                            if (sprite.position.x - tile.size.x / 2.0f >= right) {
                                // Out of visible area on the X axis, next line
                                break;
                            } else if (sprite.position.x + tile.size.x / 2.0f >= left) {
                                // In visible area
                                sprite.size = tile.size;
                                sprite.texture = tile.texture;
                                sprite.tint = mStyle.textColor;

                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.textGlyphCount;
                            }
                        case u' ':
                            // SPACE is not printable but need to advance the caret as a normal character
                            printAtX += tile.advance;
                        }
                    });
                }

                printAtY += lineHeightPixel;
                printAtX = left;
            }

            // Draw the status bar text 
            if (drawBit(STATUS_BAR)) {
                auto printAtX = mDrawingBox.viewport.x + TEXT_MARGIN * 2; // Add a mandatory left margin (TODO: move that elsewhere)
                auto printAtY = mDrawingBox.viewport.z - mCaretPrecalc.blockGlyphBearingY + mCaretPrecalc.size.y / 2.0f;
                for (auto character : mRenderPrecalc.statusBarString) {
                    mFontTexture->get(character, [&](const FontTexture::Tile& tile) {
                        sprite.position.x = glm::floor(printAtX + (tile.bearing.x + tile.size.x / 2.0f));
                        sprite.position.y = glm::round(printAtY - (tile.bearing.y - tile.size.y / 2.0f));
                        
                        switch (character) {
                        case u'	':
                            // non printable but advance the caret
                            // Take in account the TAB (0x0009) character (= SPACE width * 4)
                            printAtX += tile.advance * 4;
                            break;
                        default:
                            sprite.size = tile.size;
                            sprite.texture = tile.texture;
                            sprite.tint = mStyle.textColor;

                            mSpriteBuffer->add(sprite);
                            ++mRenderPrecalc.statusBarTextGlyphCount;
                        case u' ':
                            // SPACE is not printable but need to advance the caret as a normal character
                            printAtX += tile.advance;
                        }
                    });
                }
            }
        }
        mSpriteBuffer->unmap();
    }

    if (!updateBuffer && updateCaret) {
        // If the buffer was not updated, we need to update the caret
        mSpriteBuffer->use();
        sprite.position = mCaretPrecalc.position;
        sprite.texture = mCaretPrecalc.texture;
        sprite.size.x = mStyle.caretWidth;
        sprite.size.y = mCaretPrecalc.size.y;
        if (mCaretPrecalc.visible) {
            sprite.tint = mStyle.caretColor;
        } else {
            sprite.tint = glm::u8vec4(0);
        }
        mSpriteBuffer->update(mRenderPrecalc.caretGlyphIndex, sprite);
    }

    auto now = std::chrono::steady_clock::now();
    mUpdateTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - functionStartTime).count() * 1000;
}

void CursorRenderer::render() {
    auto functionStartTime = std::chrono::steady_clock::now();
    if (!mCursor.get() || !mFontTexture.get() || mDrawingBox.size.x <= 0 || mDrawingBox.size.y <= 0) {
        mRenderTime = 0.0f;
        return;
    }

    glm::ivec2 scissorPosition = { mDrawingBox.viewport.x, (mDrawingBox.parentSize.y - mDrawingBox.viewport.w) - mDrawingBox.size.y };
    glm::ivec2 scissorSize = { mDrawingBox.size.x, mDrawingBox.size.y };
    auto drawIndex = 0;

    glEnable(GL_BLEND);
    glBlendEquation (GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    if (drawBit(SCISSOR)) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorPosition.x, scissorPosition.y, scissorSize.x, scissorSize.y);
    }

    mFontTexture->use();
    mSpriteShader->use();
    mSpriteShader->setMatrix(mRenderPrecalc.viewMatrix);
    mSpriteBuffer->use();

    // Render All background shapes
    mSpriteBuffer->draw(0, mRenderPrecalc.backgroundGlyphCount);
    drawIndex += mRenderPrecalc.backgroundGlyphCount;
    
    // Render margin text
    if (drawBit(LEFT_MARGIN)) {
        if (drawBit(SCISSOR)) {
            scissorPosition.y += mDimenPrecalc.statusBarHeight;
            scissorSize.y -= mDimenPrecalc.statusBarHeight;
            scissorSize.x = mDimenPrecalc.marginWidth;
            glScissor(scissorPosition.x, scissorPosition.y, scissorSize.x, scissorSize.y);
        }

        if (drawBit(LINE_NUMBER)) {
            mSpriteBuffer->draw(drawIndex, mRenderPrecalc.lineNumberGlyphCount);
            drawIndex += mRenderPrecalc.lineNumberGlyphCount;
        } else if (drawBit(LINE_NUMBER_INDICATOR)) {
            mSpriteBuffer->draw(drawIndex, 1);
            drawIndex += 1;
        }
    }

    // Render selection + caret + text
    if (drawBit(SCISSOR)) {
        scissorPosition.x += mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;
        scissorSize.x = mDrawingBox.size.x - mDimenPrecalc.marginWidth - mDimenPrecalc.borderWidth - mDimenPrecalc.scrollbarWidth;
        scissorPosition.y += mDimenPrecalc.scrollbarWidth;
        scissorSize.y -= mDimenPrecalc.scrollbarWidth;
        glScissor(scissorPosition.x, scissorPosition.y, scissorSize.x, scissorSize.y);
    }

    if (!mCursor->selectionVisible()) {
        // Maybe skip the selection
        drawIndex += mRenderPrecalc.selectionGlyphCount;

        mSpriteBuffer->draw(drawIndex, mRenderPrecalc.textGlyphCount + 1);
        drawIndex += mRenderPrecalc.textGlyphCount + 1;
    } else {
        mSpriteBuffer->draw(drawIndex, mRenderPrecalc.selectionGlyphCount + mRenderPrecalc.textGlyphCount + 1);
        drawIndex += mRenderPrecalc.selectionGlyphCount + mRenderPrecalc.textGlyphCount + 1;
    }

    // Render status bar text
    if (drawBit(STATUS_BAR)) {
        if (drawBit(SCISSOR)) {
            scissorPosition.x -= mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;
            scissorSize.x = mDrawingBox.size.x;
            scissorPosition.y -= mDimenPrecalc.scrollbarWidth + mDimenPrecalc.statusBarHeight;
            scissorSize.y += mDimenPrecalc.scrollbarWidth;
            glScissor(scissorPosition.x, scissorPosition.y, scissorSize.x, scissorSize.y);
        }
        mSpriteBuffer->draw(drawIndex, mRenderPrecalc.statusBarTextGlyphCount); 
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    if (drawBit(SCISSOR)) {
        glScissor(0, 0, mDrawingBox.parentSize.x, mDrawingBox.parentSize.y);
        glDisable(GL_SCISSOR_TEST);
    }

    auto now = std::chrono::steady_clock::now();
    mRenderTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - functionStartTime).count() * 1000;
}

void CursorRenderer::style(const CursorRenderer::Style style) {
    mDirtyBit |= STYLE_CHANGED;

    // Also eventually change other values // trigger dirty bits
    if (mStyle.marginBorderWidth != style.marginBorderWidth) {
        mDirtyBit |= CALCULATE_MARGIN_WIDTH;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.scrollbarWidth != style.scrollbarWidth) {
        mDirtyBit |= CALCULATE_SCROLLBAR_WIDTH;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.lineIndicator != style.lineIndicator) {
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
        mDirtyBit |= CALCULATE_MARGIN_WIDTH;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.caretWidth != style.caretWidth) {
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }

    mStyle = style;
}

CursorRenderer::Style CursorRenderer::style() const {
    return mStyle;
}

void CursorRenderer::enableDrawBit(DrawBit bit) {
    mDrawBit |= bit;
    switch (bit) {
    case HIGHTLIGHT_CURRENT_LINE:
        mDirtyBit |= TEXT_CHANGED;
    break;
    case STATUS_BAR:
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_STATUS_BAR_HEIGHT;
    break;
    case LINE_NUMBER_INDICATOR:
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
    case LEFT_MARGIN:
    case LINE_NUMBER:
        mDirtyBit |= CALCULATE_MARGIN_WIDTH;
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
    break;
    case SCROLL_BAR:
        mDirtyBit |= CALCULATE_SCROLLBAR_WIDTH;
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
    break;
    default:
    break;
    }
}

void CursorRenderer::disableDrawBit(DrawBit bit) {
    mDrawBit &= ~bit;
    switch (bit) {
    case HIGHTLIGHT_CURRENT_LINE:
        mDirtyBit |= TEXT_CHANGED;
    break;
    case STATUS_BAR:
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_STATUS_BAR_HEIGHT;
    break;
    case LINE_NUMBER_INDICATOR:
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
    case LEFT_MARGIN:
    case LINE_NUMBER:
        mDirtyBit |= CALCULATE_MARGIN_WIDTH;
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
    break;
    case SCROLL_BAR:
        mDirtyBit |= CALCULATE_SCROLLBAR_WIDTH;
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
    break;
    default:
    break;
    }
}

bool CursorRenderer::drawBit(DrawBit bit) const {
    return mDrawBit & bit;
}

void CursorRenderer::invalidate() {
    mDirtyBit |= CALCULATE_CARET_PRECALC;
    mDirtyBit |= CALCULATE_MAX_SCROLL;
    mDirtyBit |= CALCULATE_CARET_POSITION;
    mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER_STR;
    mDirtyBit |= CALCULATE_ALL_LINE_WIDTH;
    mDirtyBit |= CALCULATE_LINE_IN_VIEW;
    mDirtyBit |= CALCULATE_MARGIN_WIDTH;
    mDirtyBit |= CALCULATE_STATUS_BAR_HEIGHT;
    mDirtyBit |= GENERATE_STATUS_BAR_STR;
}

void CursorRenderer::scrollBy(float x, float y) {
    // Cap horizontal scroll
    if (mScroll.x + x < 0) {
        mScroll.x = 0;
    } else if (mScroll.x > mDimenPrecalc.maxScroll.x) {
        mScroll.x = mDimenPrecalc.maxScroll.x;
    } else {
        mScroll.x += x;
    }

    // Cap vertical scroll
    if (mScroll.y + y < 0) {
        mScroll.y = 0;
    } else if (mScroll.y > mDimenPrecalc.maxScroll.y) {
        mScroll.y = mDimenPrecalc.maxScroll.y;
    } else {
        mScroll.y += y;
    }
    mDirtyBit |= SCROLL_POSITION_CHANGED;
    mDirtyBit |= CALCULATE_CARET_POSITION;
}

void CursorRenderer::scrollTo(float x, float y) {
    // Cap horizontal scroll
    if (x < 0) {
        mScroll.x = 0;
    } else if (x > mDimenPrecalc.maxScroll.x) {
        mScroll.x = mDimenPrecalc.maxScroll.x;
    } else {
        mScroll.x = x;
    }

    // Cap vertical scroll
    if (y < 0) {
        mScroll.y = 0;
    } else if (y > mDimenPrecalc.maxScroll.y) {
        mScroll.y = mDimenPrecalc.maxScroll.y;
    } else {
        mScroll.y = y;
    }

    mDirtyBit |= SCROLL_POSITION_CHANGED;
    mDirtyBit |= CALCULATE_CARET_POSITION;
}

glm::u32vec2 CursorRenderer::scroll() const {
    return mScroll;
}

void CursorRenderer::calculateCaretPrecalc() {
    // Fill some caret precalc data, using the block character
    mFontTexture->get(FULL_BLOCK_CHARACTER, [&](const FontTexture::Tile& tile) {
        mCaretPrecalc.size = tile.size;
        mCaretPrecalc.blockGlyphBearingY = tile.bearing.y;
    });
    mDirtyBit &= ~CALCULATE_CARET_PRECALC;
}

void CursorRenderer::calculateLonguestLineNumberPixel() {
    auto lineCount = mCursor->size();
    mRenderPrecalc.longuestLineNumberString = mRenderPrecalc.converter.from_bytes(std::to_string(lineCount));
    mDimenPrecalc.longuestLineNumberWidth = mFontTexture->measure(mRenderPrecalc.longuestLineNumberString).x;
    mFontTexture->get(mStyle.lineIndicator, [&](const FontTexture::Tile& tile) {
        mDimenPrecalc.lineIndicatorWidth = tile.advance;
    });
    mDirtyBit &= ~CALCULATE_LONGUEST_LINE_NUMBER_STR;
}

void CursorRenderer::calculateAllLineWidthPixels() {
    auto lineCount = mCursor->size();
    mDimenPrecalc.lineWidth.resize(0);
    for (size_t index = 0; index < lineCount; ++index) {
        auto line = mCursor->stringView(index);
        auto size = mFontTexture->measure(line);
        mDimenPrecalc.lineWidth.emplace_back(std::make_pair(index, size.x));
    }
    mDimenPrecalc.lineWidth.sort(sortBySize);
    mDirtyBit &= ~CALCULATE_ALL_LINE_WIDTH;
}

void CursorRenderer::calculateMarginWidth() {
    if (drawBit(LEFT_MARGIN)) {
        mDimenPrecalc.marginWidth = TEXT_MARGIN * 2;
        mDimenPrecalc.marginWidth += drawBit(LINE_NUMBER) ? mDimenPrecalc.longuestLineNumberWidth : 0;
        mDimenPrecalc.marginWidth += drawBit(LINE_NUMBER_INDICATOR) ? mDimenPrecalc.lineIndicatorWidth : 0;
        mDimenPrecalc.borderWidth = mStyle.marginBorderWidth;
    } else {
        mDimenPrecalc.marginWidth = 0;
        mDimenPrecalc.borderWidth = 0;
    }

    mDirtyBit &= ~CALCULATE_MARGIN_WIDTH;
}

void CursorRenderer::calculateScrollbarWidth() {
    mDimenPrecalc.scrollbarWidth = drawBit(SCROLL_BAR) ? mStyle.scrollbarWidth : 0;
    mDirtyBit &= ~CALCULATE_SCROLLBAR_WIDTH;
}

void CursorRenderer::calculateStatusBarHeight() {
    mDimenPrecalc.statusBarHeight = drawBit(STATUS_BAR) ? mCaretPrecalc.size.y : 0;
    mDirtyBit &= ~CALCULATE_STATUS_BAR_HEIGHT;
}

void CursorRenderer::calculateMaxScroll() {
    // Horizontal
    auto horizontalVisibleArea = mDrawingBox.size.x - mDimenPrecalc.marginWidth - mDimenPrecalc.borderWidth - mDimenPrecalc.scrollbarWidth;
    auto& longuestLineWidthPixel = mDimenPrecalc.lineWidth.back();

    if (longuestLineWidthPixel.second > horizontalVisibleArea) {
        mDimenPrecalc.maxScroll.x = longuestLineWidthPixel.second - horizontalVisibleArea;
    } else {
        mDimenPrecalc.maxScroll.x = 0;
    }
    
    // Vertical
    auto fontHeight = mCaretPrecalc.size.y;
    auto cursorSize = mCursor.get() ? mCursor->size() : 0;
    auto verticalVisibleArea = mDrawingBox.size.y - mDimenPrecalc.scrollbarWidth - mDimenPrecalc.statusBarHeight;
    
    mDimenPrecalc.maxScroll.y = cursorSize * fontHeight;
    if (mDimenPrecalc.maxScroll.y > verticalVisibleArea) {
        mDimenPrecalc.maxScroll.y -= verticalVisibleArea;
    } else {
        mDimenPrecalc.maxScroll.y = 0;
    }

    // Don't go past the max scroll if we are calculating new values
    if (mScroll.x > mDimenPrecalc.maxScroll.x) {
        mScroll.x = mDimenPrecalc.maxScroll.x;
    }
    if (mScroll.y > mDimenPrecalc.maxScroll.y) {
        mScroll.y = mDimenPrecalc.maxScroll.y;
    }

    mDirtyBit &= ~CALCULATE_MAX_SCROLL;
}

void CursorRenderer::calculateCaretPosition() {
    auto caretPosition = mCursor->position(); 
    auto lineHeight = mCaretPrecalc.size.y;
    auto scroll = -mScroll;

    // Calculate Y caret position
    auto y = mDrawingBox.viewport.w + mCaretPrecalc.blockGlyphBearingY + scroll.y;
    y -= mCaretPrecalc.blockGlyphBearingY - lineHeight / 2.0f;
    y += caretPosition.y * lineHeight;

    // Calculate X carret position
    auto line = mCursor->stringView(caretPosition.y);
    auto x = mDrawingBox.viewport.x + scroll.x + mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;
    x += mFontTexture->measure(line.substr(0, caretPosition.x)).x;
    x += mStyle.caretWidth / 2.0f;
    
    mCaretPrecalc.position.y = glm::round(y);
    mCaretPrecalc.position.x = glm::ceil(x);
    mDirtyBit &= ~CALCULATE_CARET_POSITION;
}

void CursorRenderer::calculateLineInView() {
    // Calculate the number of visible line
    auto visibleLine = 0;
    auto fontHeight = mCaretPrecalc.size.y;
    if (fontHeight != 0) {
        visibleLine = glm::ceil(mDrawingBox.size.y / fontHeight);
    }
    mDimenPrecalc.visibleLineCount = visibleLine;
    mDirtyBit &= ~CALCULATE_LINE_IN_VIEW;
}

void CursorRenderer::scrollCaretToBorder(uint8_t border) {
    // Calculate drawing area and top text location
    auto lineHeight = mCaretPrecalc.size.y;
    auto scroll = -mScroll;

    // Keep a copy, because we scroll one axis at a time to avoid zigzag
    auto doScroll = false;
    auto scrollToPosition = scroll;
    if (border & HORIZONTAL) {
        auto leftBorder = mDrawingBox.viewport.x + mDimenPrecalc.marginWidth + mDimenPrecalc.borderWidth;
        auto rightBorder = mDrawingBox.viewport.y - mDimenPrecalc.scrollbarWidth;

        if (mCaretPrecalc.position.x - mStyle.caretWidth < leftBorder) {
            auto leftScroll = (mCaretPrecalc.position.x - mStyle.caretWidth) - scroll.x - leftBorder;
            scrollToPosition.x = leftScroll;
            doScroll = true;
        } 
        else
        if (mCaretPrecalc.position.x + mStyle.caretWidth > rightBorder) {
            auto rightScroll = (mCaretPrecalc.position.x + mStyle.caretWidth) - scroll.x - rightBorder;
            scrollToPosition.x = rightScroll;
            doScroll = true;
        }
        else {
            // Revert back original scroll value
            scrollToPosition.x = -scrollToPosition.x;
        }
    }
    if (border & VERTICAL) {
        auto topBorder = mDrawingBox.viewport.w;
        auto bottomBorder = mDrawingBox.viewport.z - mDimenPrecalc.scrollbarWidth - mDimenPrecalc.statusBarHeight;

        if (mCaretPrecalc.position.y - lineHeight / 2.0f < topBorder) {
            auto topScroll = mCaretPrecalc.position.y - mDrawingBox.viewport.w - scroll.y - lineHeight / 2.0f;
            scrollToPosition.y = topScroll;
            doScroll = true;
        }
        else
        if (mCaretPrecalc.position.y + lineHeight / 2.0f > bottomBorder) {
            auto bottomScroll = mCaretPrecalc.position.y - scroll.y  - mDrawingBox.viewport.z + mDimenPrecalc.scrollbarWidth + mDimenPrecalc.statusBarHeight + lineHeight / 2.0f;
            scrollToPosition.y = bottomScroll;
            doScroll = true;
        }
        else {
            // Revert back original scroll value
            scrollToPosition.y = -scrollToPosition.y;
        }
    }
    if (doScroll) {
        scrollTo(scrollToPosition.x, scrollToPosition.y);
    }

    mDirtyBit &= ~TRY_SCROLL_TO_BORDERS;
}

void CursorRenderer::checkLine(size_t index, CheckLine check) {
    auto changedLine = mCursor->stringView(index);
    auto changedLineWidth = mFontTexture->measure(changedLine).x;
    auto& longuestLineWidth = mDimenPrecalc.lineWidth.back();
    std::list<std::pair<size_t, float>>::iterator it;

    switch (check) {
    case EDITED:
        // Find the precalculated line width
        it = std::find_if(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](const std::pair<size_t, float>& pair) {
            return pair.first == index;
        });

        if (it == mDimenPrecalc.lineWidth.end()) {
            // Should never goes here
            return;
        }
    
        it->second = changedLineWidth;
        if (index == longuestLineWidth.first) {
            // Longuest line change make scroll dirty but not the line width list as we updated the top value
            mDirtyBit |= CALCULATE_MAX_SCROLL;
        } else {
            if (changedLineWidth > longuestLineWidth.second) {
                // The longuest line changed and the max scroll 
                mDirtyBit |= CALCULATE_MAX_SCROLL;
            }
            // But we don't know were we are in the line width list, we need to sort it
            mDirtyBit |= REORDER_LINE_WIDTH;
        }
    break;
    case CREATED:
        // Increase each line number at and after index in the precalc
        std::for_each(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](std::pair<size_t, float>& pair) {
            if (pair.first >= index) {
                ++pair.first;
            }
        });

        if (changedLineWidth > longuestLineWidth.second) {
            // The inserted line is the new longuest line, replae to back element in the list
            mDimenPrecalc.lineWidth.insert(mDimenPrecalc.lineWidth.end(), std::make_pair(index, changedLineWidth));
        } else {
            // Put at the beginning, and we need to sort the lines again
            mDimenPrecalc.lineWidth.insert(mDimenPrecalc.lineWidth.begin(), std::make_pair(index, changedLineWidth));
            mDirtyBit |= REORDER_LINE_WIDTH;
        }
    break;
    case DELETED:
        // Find the precalculated line width
        it = std::find_if(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](const std::pair<size_t, float>& pair) {
            return pair.first == index;
        });

        if (it == mDimenPrecalc.lineWidth.end()) {
            // Should never goes here
           return;
        }

        // Decrease each line number at and after index in the precalc
        std::for_each(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](std::pair<size_t, float>& pair) {
            if (pair.first >= index) {
                --pair.first;
            }
        });
        mDimenPrecalc.lineWidth.erase(it);
    break;
    }
}

void CursorRenderer::generateCaretPositionString() {
    auto caretPosition = mCursor->position(); 
    auto characterNumberText = mRenderPrecalc.converter.from_bytes(std::to_string(caretPosition.x + 1));
    auto lineNumberText = mRenderPrecalc.converter.from_bytes(std::to_string(caretPosition.y + 1));
    mRenderPrecalc.caretPositionString = u"L: " + lineNumberText.append(u",").append(characterNumberText);
    mDirtyBit &= ~GENERATE_CARET_POSITION_STR;
}

void CursorRenderer::generateStatusBarString() {
    mRenderPrecalc.statusBarString = std::u16string(u"Lines: ").append(mRenderPrecalc.longuestLineNumberString)
        .append(u" | ").append(mRenderPrecalc.caretPositionString);
    mDirtyBit &= ~GENERATE_STATUS_BAR_STR;
}
