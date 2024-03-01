#include "CursorRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#define SPRITE_BUFFER_SIZE 8192 // Number of Vertex

CursorRenderer::CursorRenderer() :
mSpriteShader(std::make_unique<SpriteShader>()),
mSpriteBuffer(std::make_unique<SpriteBuffer>(SPRITE_BUFFER_SIZE)),
mDrawingBox({ { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0, 0, 0} }),
mRenderPrecalc({ 0, 0, 0, 0, 0, glm::mat4(1) }),
mDimenPrecalc({ 0, 0, 0, { 0, 0 }, { 0, 0, 0, 0 } }),
mCaretPrecalc({ true, 0.5f, 0, 0, { 2, 0 }, { 0, 0 } }),
mScroll(0),
mDrawBit(UINT8_MAX),
mDirtyBit(UINT16_MAX),
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
    mDrawingBox.viewport.y = mDrawingBox.position.y - mDrawingBox.size.y / 2.0f;
    mDrawingBox.viewport.z = mDrawingBox.position.x + mDrawingBox.size.x / 2.0f;
    mDrawingBox.viewport.w = mDrawingBox.position.y + mDrawingBox.size.y / 2.0f;

    mDirtyBit |= CALCULATE_MAX_SCROLL;
    mDirtyBit |= CALCULATE_LINE_IN_VIEW;
    mDirtyBit |= CALCULATE_CARET_POSITION;
    mDirtyBit |= INVALIDATE_SPRITES;
    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
}

void CursorRenderer::bind(const std::shared_ptr<FontTexture> fontTexture) {
    mFontTexture = fontTexture;
    mDirtyBit |= CALCULATE_CARET_PRECALC | CALCULATE_MAX_SCROLL | CALCULATE_LONGUEST_LINE_NUMBER
    | CALCULATE_ALL_LINE_WIDTH | CALCULATE_CARET_POSITION | CALCULATE_LINE_IN_VIEW | INVALIDATE_SPRITES
    | INVALIDATE_SCROLL_INDICATORS;
}

void CursorRenderer::bind(const std::shared_ptr<Cursor> cursor) {
    mCursor = cursor;
    mDirtyBit |= CALCULATE_MAX_SCROLL | CALCULATE_LONGUEST_LINE_NUMBER | CALCULATE_ALL_LINE_WIDTH
    | CALCULATE_CARET_POSITION | INVALIDATE_SCROLL_INDICATORS;
}

void CursorRenderer::update(float time) {
    auto functionStartTime = std::chrono::steady_clock::now();
    if (!mCursor.get() || !mFontTexture.get() || mDrawingBox.size.x <= 0 || mDrawingBox.size.y <= 0) {
        mUpdateTime = 0.0f;
        return;
    }

    // Pump and clear the cursor events
    auto caretPosition = mCursor->position();
    auto cursorEvent = mCursor->event();
    while (cursorEvent != nullptr) {
        switch (cursorEvent->type) {
        case Cursor::EventType::CARET_MOVED:
            mDirtyBit |= CALCULATE_CARET_POSITION;
            mDirtyBit |= TRY_SCROLL_TO_BORDERS;
        break;
        case Cursor::EventType::LINE_CREATED:
        case Cursor::EventType::LINE_CHANGED:
        case Cursor::EventType::LINE_DELETED:
            checkLine(cursorEvent->data, (CheckLine) cursorEvent->type);
        break;
        }

        mCursor->popEvent();
        cursorEvent = mCursor->event();
    }

    // Inspect what changed in the internal state
    auto updateBuffer = false;
    auto updateCaret = false;
    if (mDirtyBit & CALCULATE_CARET_PRECALC) {
        invalidateSpriteTextures();
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
    if (mDirtyBit & CALCULATE_LONGUEST_LINE_NUMBER) {
        calculateLonguestLineNumberPixel();
        updateBuffer = true;
    }
    if (mDirtyBit & INVALIDATE_SPRITES) {
        invalidateSprite();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_MAX_SCROLL) {
        calculateMaxScroll();
        updateBuffer = true;
    }
    if (mDirtyBit & INVALIDATE_SCROLL_INDICATORS) {
        invalidateScrollIndicators();
        updateBuffer = true;
    }
    if (mDirtyBit & CALCULATE_CARET_POSITION) {
        calculateCaretPosition();
        updateCaret = true;
    }
    if (mDirtyBit & TRY_SCROLL_TO_BORDERS) {
        scrollCaretToBorder();
        // Passing here indicate that the caret moved, so make it instantly visible
        mCaretPrecalc.visible = true;
        mCaretPrecalc.nextBlinkTime = time + mCaretPrecalc.blinkTime;
    }
    if (mDirtyBit & STYLE_CHANGED) {
        mDirtyBit &= ~STYLE_CHANGED;
        updateBuffer = true;
    }
    if (mDirtyBit & TEXT_CHANGED) {
        mDirtyBit &= ~TEXT_CHANGED;
        updateBuffer = true;
    }

    if (time > mCaretPrecalc.nextBlinkTime) {
        mCaretPrecalc.nextBlinkTime = time + mCaretPrecalc.blinkTime;
        mCaretPrecalc.visible = !mCaretPrecalc.visible;
        updateCaret = true;
    }

    SpriteVertex sprite;
    if (updateBuffer) {
        // Reset mRenderPrecalc data
        mRenderPrecalc.backgroundGlyphCount = 0;
        mRenderPrecalc.textGlyphCount = 0;
        mRenderPrecalc.lineNumberGlyphCount = 0;
        mRenderPrecalc.selectionGlyphCount = 0;
        mRenderPrecalc.caretGlyphIndex = 0;

        // Map the whole buffers
        mSpriteBuffer->use();
        mSpriteBuffer->map();

        // Maybe draw the left margin
        if (drawBit(LEFT_MARGIN)) {
            // Upate the background values
            mSpriteBuffer->add(mMarginSprite);
            ++mRenderPrecalc.backgroundGlyphCount;

            // Maybe draw the border
            if (mBorderSprite.size.x > 0) {
                mSpriteBuffer->add(mBorderSprite);
                ++mRenderPrecalc.backgroundGlyphCount;
            }
        }

        // Maybe show the scrollbar
        if (drawBit(SCROLL_BAR) && mStyle.scrollbarWidth > 0) {
            // Vertical
            mSpriteBuffer->add(mVScrollSprite);
            mSpriteBuffer->add(mVScrollIndicatorSprite);
            ++mRenderPrecalc.backgroundGlyphCount;
            ++mRenderPrecalc.backgroundGlyphCount;

            // Horizontal
            mSpriteBuffer->add(mHScrollSprite);
            mSpriteBuffer->add(mHScrollIndicatorSprite);
            ++mRenderPrecalc.backgroundGlyphCount;
            ++mRenderPrecalc.backgroundGlyphCount;

            // Bottom right corner
            mSpriteBuffer->add(mCornerSprite);
            ++mRenderPrecalc.backgroundGlyphCount;
        }

        // Draw the editor background
        mSpriteBuffer->add(mBackgroundSprite);
        ++mRenderPrecalc.backgroundGlyphCount;

        // Calculate scroll amount for the text
        auto scroll = -mScroll;
        auto scrollAmount = glm::abs(scroll.y / mCaretPrecalc.size.y);
        auto scrollFraction = glm::fract(scrollAmount) * mCaretPrecalc.size.y;
        auto topText = mDimenPrecalc.textBounds.y + mCaretPrecalc.bearingY - scrollFraction;
        auto firstVisibleLine = glm::floor(scrollAmount);
        auto lastVisibleLine = firstVisibleLine + mDimenPrecalc.visibleLineCount + 2; // one line above and below the visible area must be drawn
        auto visibleText = true;

        // Allows scroll out of view
        if (firstVisibleLine < 0) {
            // Gap between the first line and the top of the screen
            auto skipCount = glm::abs(firstVisibleLine);
            topText += (skipCount * mCaretPrecalc.size.y);
            if (topText > mDimenPrecalc.textBounds.w + mCaretPrecalc.size.y) {
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
                auto printAtX = mDrawingBox.viewport.x;
                auto printAtY = topText;

                // Print line number or indicator maybe
                if (drawBit(LINE_NUMBER_INDICATOR) || drawBit(LINE_NUMBER)) {
                    for (size_t lineIndex = firstVisibleLine; lineIndex < lastVisibleLine; ++lineIndex) {
                        if (lineIndex >= mCursor->size() || printAtY > mDrawingBox.viewport.w + mCaretPrecalc.size.y / 2.0f) {
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
                            for (const auto& character : number) {
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

                        printAtY += mCaretPrecalc.size.y;
                        printAtX = mDrawingBox.viewport.x;
                    }
                }
            }

            // Draw the cursor selection
            // TODO: clip out of bounds
            auto selection = mCursor->selection();
            if (!selection.empty()) {
                bool selectionStartInViewY = selection.start.y >= firstVisibleLine || selection.start.y < firstVisibleLine + mDimenPrecalc.visibleLineCount;
                bool selectionEndInViewY = selection.end.y >= firstVisibleLine || selection.end.y < firstVisibleLine + mDimenPrecalc.visibleLineCount;
                // check if we are in the view area on the y axis
                if (selectionStartInViewY || selectionEndInViewY) {
                    sprite.texture = mFontTexture->pixelCoordinates();
                    sprite.tint = mStyle.selectedTextColor;
                    sprite.size.y = mCaretPrecalc.size.y;
                    if (selection.start.y == selection.end.y) {
                        // Start / end on the same line
                        if (selection.end.x < selection.start.x) {
                            // Check if we need to invert X axis
                            std::swap(selection.end.x, selection.start.x);
                        }

                        auto line = mCursor->stringView(selection.start.y);
                        auto startY = (selection.start.y - firstVisibleLine) * mCaretPrecalc.size.y;
                        startY -= mCaretPrecalc.bearingY - mCaretPrecalc.size.y / 2.0f;

                        auto selectionStartX = mFontTexture->measure(line.substr(0, selection.start.x)).x;
                        auto selectionWidthPixel = mFontTexture->measure(line.substr(selection.start.x, selection.end.x - selection.start.x)).x;
                        sprite.position.x = (mBackgroundSprite.position.x - mBackgroundSprite.size.x / 2.0f) + (selectionStartX + (selectionWidthPixel / 2.0f)) + scroll.x;
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
                            auto startY = (y - firstVisibleLine) * mCaretPrecalc.size.y;
                            startY -= mCaretPrecalc.bearingY - mCaretPrecalc.size.y / 2.0f;
                            if (y == selection.start.y) {
                                auto selectionStartX = mFontTexture->measure(line.substr(0, selection.start.x)).x;
                                auto selectionWidthPixel = mBackgroundSprite.size.x - selectionStartX;
                                sprite.position.x = (mBackgroundSprite.position.x - mBackgroundSprite.size.x / 2.0f) + (selectionStartX + (selectionWidthPixel / 2.0f)) + scroll.x / 2.0f;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = selectionWidthPixel - scroll.x;
                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.selectionGlyphCount;
                            } else if (y == selection.end.y) {
                                auto selectionEndX = mFontTexture->measure(line.substr(0, selection.end.x)).x;
                                sprite.position.x = (mBackgroundSprite.position.x - mBackgroundSprite.size.x / 2.0f) + (selectionEndX / 2.0f) + scroll.x;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = selectionEndX;
                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.selectionGlyphCount;
                            } else {
                                sprite.position.x = mBackgroundSprite.position.x;
                                sprite.position.y = glm::round(topText + startY);
                                sprite.size.x = mBackgroundSprite.size.x;
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
                    auto positionInView = (caretPosition.y - firstVisibleLine) * mCaretPrecalc.size.y;
                    positionInView -= mCaretPrecalc.bearingY - mCaretPrecalc.size.y / 2.0f;

                    sprite.position.x = mBackgroundSprite.position.x;
                    sprite.position.y = glm::round(topText + positionInView);
                    sprite.size.x = mBackgroundSprite.size.x;
                    sprite.size.y = mCaretPrecalc.size.y;
                    sprite.texture = mFontTexture->pixelCoordinates();
                    sprite.tint = mStyle.currentLinebackgroundColor;
                    mSpriteBuffer->add(sprite);
                    ++mRenderPrecalc.textGlyphCount;
                }
            }

            // Put the caret in the buffer, keep trace of it's position
            sprite.position = mCaretPrecalc.position;
            sprite.size.x =  mStyle.caretWidth;
            sprite.size.y = mCaretPrecalc.size.y;
            sprite.texture = mFontTexture->pixelCoordinates();
            if (mCaretPrecalc.visible) {
                sprite.tint = mStyle.caretColor;
            } else {
                sprite.tint = glm::u8vec4(0);
            }
            mRenderPrecalc.caretGlyphIndex = mSpriteBuffer->index();
            mSpriteBuffer->add(sprite);

            auto printAtX = mDimenPrecalc.textBounds.x;
            auto printAtY = topText;

            sprite.tint = mStyle.textColor;
            for (size_t lineIndex = firstVisibleLine; lineIndex < lastVisibleLine; ++lineIndex) {
                if (lineIndex >= mCursor->size() || printAtY > mDimenPrecalc.textBounds.w + mCaretPrecalc.size.y / 2.0f) {
                    // Stop early if possible
                    break;
                }

                const auto line = mCursor->stringView(lineIndex);
                for (const auto& character : line) {
                    bool stop = false;
                    mFontTexture->get(character, [&](const FontTexture::Tile& tile) {
                        sprite.position.x = glm::floor(printAtX + (tile.bearing.x + tile.size.x / 2.0f) + scroll.x);
                        sprite.position.y = glm::round(printAtY - (tile.bearing.y - tile.size.y / 2.0f));

                        if (sprite.position.x - tile.size.x / 2.0f >= mDrawingBox.viewport.z - mVScrollSprite.size.x) {
                            // Out of visible area on the X axis, next line
                            stop = true;
                            return;
                        }

                        switch (character) {
                        case u'	':
                            // Tab
                            printAtX += tile.advance * 4;
                        break;
                        case u' ':
                            // Space
                            printAtX += tile.advance;
                        break;
                        default:
                            if (sprite.position.x + tile.size.x / 2.0f >= mDimenPrecalc.textBounds.x) {
                                // In visible area
                                sprite.size = tile.size;
                                sprite.texture = tile.texture;

                                mSpriteBuffer->add(sprite);
                                ++mRenderPrecalc.textGlyphCount;
                            }
                            printAtX += tile.advance;
                        break;
                        }
                    });
                    if (stop) {
                        break;
                    }
                }

                printAtY += mCaretPrecalc.size.y;
                printAtX = mDimenPrecalc.textBounds.x;
            }
        }
        mSpriteBuffer->unmap();
    }

    if (!updateBuffer && updateCaret) {
        // If the buffer was not updated, we need to update the caret
        mSpriteBuffer->use();
        sprite.texture = mFontTexture->pixelCoordinates();
        sprite.position = mCaretPrecalc.position;
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

    auto drawIndex = 0;
    glEnable(GL_BLEND);
    glBlendEquation (GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    if (!drawBit(SCISSOR)) {
        glDisable(GL_SCISSOR_TEST);
    }

    mFontTexture->use();
    mSpriteShader->use();
    mSpriteShader->setMatrix(mRenderPrecalc.viewMatrix);
    mSpriteBuffer->use();

    // Render All background shapes
    glScissor(
        mDrawingBox.viewport.x,
        (mDrawingBox.parentSize.y - mDrawingBox.viewport.y) - mDrawingBox.size.y,
        mDrawingBox.size.x,
        mDrawingBox.size.y);

    mSpriteBuffer->draw(0, mRenderPrecalc.backgroundGlyphCount);
    drawIndex += mRenderPrecalc.backgroundGlyphCount;

    // Render margin text
    if (drawBit(LEFT_MARGIN)) {
        glScissor(
            mDrawingBox.viewport.x,
            (mDrawingBox.parentSize.y - mDrawingBox.viewport.y) - mDrawingBox.size.y,
            mMarginSprite.size.x + mBorderSprite.size.x,
            mDrawingBox.size.y);

        if (drawBit(LINE_NUMBER)) {
            mSpriteBuffer->draw(drawIndex, mRenderPrecalc.lineNumberGlyphCount);
            drawIndex += mRenderPrecalc.lineNumberGlyphCount;
        } else if (drawBit(LINE_NUMBER_INDICATOR)) {
            mSpriteBuffer->draw(drawIndex, 1);
            drawIndex += 1;
        }
    }

    // Render selection + caret + text
    glScissor(
        mDrawingBox.viewport.x + mMarginSprite.size.x + mBorderSprite.size.x,
        mDrawingBox.parentSize.y - mDrawingBox.viewport.y - mDrawingBox.size.y + mHScrollSprite.size.y,
        mDrawingBox.size.x - mMarginSprite.size.x - mBorderSprite.size.x - mVScrollSprite.size.x,
        mDrawingBox.size.y - mHScrollSprite.size.y);

    if (!mCursor->selectionVisible()) {
        // Maybe skip the selection
        drawIndex += mRenderPrecalc.selectionGlyphCount;

        mSpriteBuffer->draw(drawIndex, mRenderPrecalc.textGlyphCount + 1);
        drawIndex += mRenderPrecalc.textGlyphCount + 1;
    } else {
        mSpriteBuffer->draw(drawIndex, mRenderPrecalc.selectionGlyphCount + mRenderPrecalc.textGlyphCount + 1);
        drawIndex += mRenderPrecalc.selectionGlyphCount + mRenderPrecalc.textGlyphCount + 1;
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, mDrawingBox.parentSize.x, mDrawingBox.parentSize.y);

    auto now = std::chrono::steady_clock::now();
    mRenderTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - functionStartTime).count() * 1000;
}

void CursorRenderer::style(const CursorRenderer::Style style) {
    mDirtyBit |= STYLE_CHANGED;

    // Also eventually change other values // trigger dirty bits
    if (mStyle.marginBorderWidth != style.marginBorderWidth) {
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.scrollbarWidth != style.scrollbarWidth) {
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.lineIndicator != style.lineIndicator) {
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }
    if (mStyle.caretWidth != style.caretWidth) {
        mDirtyBit |= CALCULATE_CARET_POSITION;
    }

    mDirtyBit |= INVALIDATE_SPRITES;
    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;

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
    case LINE_NUMBER_INDICATOR:
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;
    case LEFT_MARGIN:
    case LINE_NUMBER:
    case SCROLL_BAR:
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= INVALIDATE_SPRITES;
        mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
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
    case LINE_NUMBER_INDICATOR:
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;
    case LEFT_MARGIN:
    case LINE_NUMBER:
    case SCROLL_BAR:
        mDirtyBit |= CALCULATE_CARET_POSITION;
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= INVALIDATE_SPRITES;
        mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
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
    mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;
    mDirtyBit |= CALCULATE_ALL_LINE_WIDTH;
    mDirtyBit |= CALCULATE_CARET_POSITION;
    mDirtyBit |= CALCULATE_LINE_IN_VIEW;
    mDirtyBit |= INVALIDATE_SPRITES;
    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
}

void CursorRenderer::invalidateSpriteTextures() {
    const auto& pixel = mFontTexture->pixelCoordinates();
    mMarginSprite.texture = pixel;
    mBorderSprite.texture = pixel;
    mBackgroundSprite.texture = pixel;
    mHScrollSprite.texture = pixel;
    mHScrollIndicatorSprite.texture = pixel;
    mVScrollSprite.texture = pixel;
    mVScrollIndicatorSprite.texture = pixel;
    mCornerSprite.texture = pixel;
}

void CursorRenderer::invalidateSprite() {
    if (drawBit(LEFT_MARGIN)) {
        // Left margin
        mMarginSprite.tint = mStyle.marginColor;
        mMarginSprite.size.x = 0;
        mMarginSprite.size.y = mDrawingBox.size.y;
        if (drawBit(LINE_NUMBER)) {
            mMarginSprite.size.x += mDimenPrecalc.lineNumberWidth;
        }
        if (drawBit(LINE_NUMBER_INDICATOR)) {
            mMarginSprite.size.x += mDimenPrecalc.lineIndicatorWidth;
        }
        mMarginSprite.position.x = mDrawingBox.viewport.x + mMarginSprite.size.x / 2.0f;
        mMarginSprite.position.y = mDrawingBox.viewport.y + mMarginSprite.size.y / 2.0f;

        // Border
        mBorderSprite.tint = mStyle.marginBorderColor;
        mBorderSprite.size.x = mStyle.marginBorderWidth;
        mBorderSprite.size.y = mMarginSprite.size.y;
        mBorderSprite.position.x = mDrawingBox.viewport.x + mMarginSprite.size.x + mBorderSprite.size.x / 2.0f;
        mBorderSprite.position.y = mMarginSprite.position.y;
    } else {
        mMarginSprite.size.x = 0;
        mMarginSprite.size.y = 0;
        mBorderSprite.size.x = 0;
        mBorderSprite.size.y = 0;
    }

    if (drawBit(SCROLL_BAR)) {
        // Horizontal scrollbars
        mHScrollSprite.tint = mStyle.scrollbarColor;
        mHScrollSprite.size.x = mDrawingBox.size.x - mStyle.scrollbarWidth;
        mHScrollSprite.size.y = mStyle.scrollbarWidth;
        if (drawBit(LEFT_MARGIN)) {
            mHScrollSprite.size.x -= mMarginSprite.size.x + mBorderSprite.size.x;
        }
        mHScrollSprite.position.x = mDrawingBox.viewport.z - mStyle.scrollbarWidth - mHScrollSprite.size.x / 2.0f;
        mHScrollSprite.position.y = mDrawingBox.viewport.w - mStyle.scrollbarWidth / 2.0f;

        mHScrollIndicatorSprite.size.y = mStyle.scrollbarWidth;
        mHScrollIndicatorSprite.tint = mStyle.scrollbarIndicatorColor;

        // Vertical scrollbars
        mVScrollSprite.tint = mStyle.scrollbarColor;
        mVScrollSprite.size.x = mStyle.scrollbarWidth;
        mVScrollSprite.size.y = mDrawingBox.size.y - mStyle.scrollbarWidth;
        mVScrollSprite.position.x = mDrawingBox.viewport.z - mStyle.scrollbarWidth / 2.0f;
        mVScrollSprite.position.y = mDrawingBox.viewport.y + mVScrollSprite.size.y / 2.0f;

        mVScrollIndicatorSprite.size.x = mStyle.scrollbarWidth;
        mVScrollIndicatorSprite.tint = mStyle.scrollbarIndicatorColor;

        // Corner
        mCornerSprite.tint = mStyle.scrollbarColor;
        mCornerSprite.size.x = mStyle.scrollbarWidth;
        mCornerSprite.size.y = mStyle.scrollbarWidth;;
        mCornerSprite.position.x = mDrawingBox.viewport.z - mStyle.scrollbarWidth / 2.0f;
        mCornerSprite.position.y = mDrawingBox.viewport.w - mStyle.scrollbarWidth / 2.0f;
    } else {
        mHScrollSprite.size.x = 0;
        mHScrollSprite.size.y = 0;
        mVScrollSprite.size.x = 0;
        mVScrollSprite.size.y = 0;
    }

    // Background
    mBackgroundSprite.tint = mStyle.backgroundColor;
    mBackgroundSprite.size.x = mDrawingBox.size.x;
    mBackgroundSprite.size.y = mDrawingBox.size.y;
    if (drawBit(LEFT_MARGIN)) {
        mBackgroundSprite.size.x -= mMarginSprite.size.x;
        if (mBorderSprite.size.x > 0) {
            mBackgroundSprite.size.x -= mBorderSprite.size.x;
        }
    }
    if (drawBit(SCROLL_BAR)) {
        mBackgroundSprite.size.x -= mVScrollSprite.size.x;
        mBackgroundSprite.size.y -= mHScrollSprite.size.y;
    }
    mBackgroundSprite.position.x = mDrawingBox.viewport.z - mBackgroundSprite.size.x / 2.0f;
    mBackgroundSprite.position.y = mDrawingBox.viewport.y + mBackgroundSprite.size.y / 2.0f;
    if (drawBit(SCROLL_BAR)) {
        mBackgroundSprite.position.x -= mVScrollSprite.size.x;
    }

    // Calculate the text bounds
    mDimenPrecalc.textBounds.x = mDrawingBox.viewport.x + mMarginSprite.size.x + mBorderSprite.size.x;
    mDimenPrecalc.textBounds.y = mDrawingBox.viewport.y;
    mDimenPrecalc.textBounds.z = mDrawingBox.viewport.z - mVScrollSprite.size.x;
    mDimenPrecalc.textBounds.w = mDrawingBox.viewport.w - mHScrollSprite.size.y;

    mDirtyBit &= ~INVALIDATE_SPRITES;
}

void CursorRenderer::invalidateScrollIndicators() {
    // Upate the background values
    auto indicatorWidth = glm::max({ mHScrollSprite.size.y, mVScrollSprite.size.x }, mBackgroundSprite.size - mDimenPrecalc.maxScroll);
    auto indicatorPosition = glm::abs(-mScroll / mDimenPrecalc.maxScroll) * (mBackgroundSprite.size - indicatorWidth) + indicatorWidth / 2.0f;

    mVScrollIndicatorSprite.position.x = mVScrollSprite.position.x;
    mVScrollIndicatorSprite.position.y = (mVScrollSprite.position.y - mVScrollSprite.size.y / 2.0f) + indicatorPosition.y;
    mVScrollIndicatorSprite.size.x = mVScrollSprite.size.x;
    mVScrollIndicatorSprite.size.y = indicatorWidth.y;
    mVScrollIndicatorSprite.tint = mStyle.scrollbarIndicatorColor;

    mHScrollIndicatorSprite.position.y = mHScrollSprite.position.y;
    mHScrollIndicatorSprite.position.x = (mHScrollSprite.position.x - mHScrollSprite.size.x / 2.0f) + indicatorPosition.x;
    mHScrollIndicatorSprite.size.x = indicatorWidth.x;
    mHScrollIndicatorSprite.size.y = mHScrollSprite.size.y;
    mHScrollIndicatorSprite.tint = mStyle.scrollbarIndicatorColor;

    mDirtyBit &= ~INVALIDATE_SCROLL_INDICATORS;
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

    mDirtyBit |= TEXT_CHANGED;
    mDirtyBit |= CALCULATE_CARET_POSITION;
    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
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

    mDirtyBit |= TEXT_CHANGED;
    mDirtyBit |= CALCULATE_CARET_POSITION;
    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
}

glm::u32vec2 CursorRenderer::scroll() const {
    return mScroll;
}

void CursorRenderer::calculateCaretPrecalc() {
    // Fill some caret precalc data, using the block character
    mFontTexture->get(FULL_BLOCK_CHARACTER, [&](const FontTexture::Tile& tile) {
        mCaretPrecalc.size = tile.size;
        mCaretPrecalc.bearingY = tile.bearing.y;
    });
    mDirtyBit &= ~CALCULATE_CARET_PRECALC;
}

void CursorRenderer::calculateLonguestLineNumberPixel() {
    auto lineCount = mCursor->size();
    auto string = mRenderPrecalc.converter.from_bytes(std::to_string(lineCount));
    mDimenPrecalc.lineNumberWidth = mFontTexture->measure(string).x;
    mFontTexture->get(mStyle.lineIndicator, [&](const FontTexture::Tile& tile) {
        mDimenPrecalc.lineIndicatorWidth = tile.advance;
    });
    mDirtyBit &= ~CALCULATE_LONGUEST_LINE_NUMBER;
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

void CursorRenderer::calculateMaxScroll() {
    // Horizontal
    auto horizontalVisibleArea = mDimenPrecalc.textBounds.z - mDimenPrecalc.textBounds.x;
    auto& longuestLineWidthPixel = mDimenPrecalc.lineWidth.back();

    if (longuestLineWidthPixel.second > horizontalVisibleArea) {
        mDimenPrecalc.maxScroll.x = longuestLineWidthPixel.second + mStyle.caretWidth - horizontalVisibleArea;
    } else {
        mDimenPrecalc.maxScroll.x = 0;
    }

    // Vertical
    auto fontHeight = mCaretPrecalc.size.y;
    auto cursorSize = mCursor.get() ? mCursor->size() : 0;
    auto verticalVisibleArea = mDimenPrecalc.textBounds.w - mDimenPrecalc.textBounds.y;

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

    // Calculate Y caret position
    auto y = mDrawingBox.viewport.y + mCaretPrecalc.bearingY - mScroll.y;
    y -= mCaretPrecalc.bearingY - lineHeight / 2.0f;
    y += caretPosition.y * lineHeight;

    // Calculate X carret position
    auto line = mCursor->stringView(caretPosition.y);
    auto x = mDimenPrecalc.textBounds.x - mScroll.x;
    x += mFontTexture->measure(line.substr(0, caretPosition.x)).x;
    x += glm::ceil(mStyle.caretWidth / 2.0f);

    mCaretPrecalc.position.y = glm::round(y);
    mCaretPrecalc.position.x = glm::floor(x);
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

void CursorRenderer::scrollCaretToBorder() {
    // Calculate drawing area and top text location
    auto lineHeight = mCaretPrecalc.size.y;

    auto scrollToPosition = -mScroll;
    auto doScroll = false;

    // Horizontal
    if (mCaretPrecalc.position.x - mStyle.caretWidth < mDimenPrecalc.textBounds.x) {
        auto leftScroll = (mCaretPrecalc.position.x - mStyle.caretWidth) + mScroll.x - mDimenPrecalc.textBounds.x;
        scrollToPosition.x = leftScroll;
        doScroll = true;
    }
    else
    if (mCaretPrecalc.position.x + mStyle.caretWidth > mDimenPrecalc.textBounds.z) {
        auto rightScroll = (mCaretPrecalc.position.x + mStyle.caretWidth) + mScroll.x - mDimenPrecalc.textBounds.z;
        scrollToPosition.x = rightScroll;
        doScroll = true;
    }
    else {
        // Revert back original scroll value
        scrollToPosition.x = -scrollToPosition.x;
    }

    // Vertical
    if (mCaretPrecalc.position.y - lineHeight / 2.0f < mDimenPrecalc.textBounds.y) {
        auto topScroll = mCaretPrecalc.position.y - mDimenPrecalc.textBounds.y + mScroll.y - lineHeight / 2.0f;
        scrollToPosition.y = topScroll;
        doScroll = true;
    }
    else
    if (mCaretPrecalc.position.y + lineHeight / 2.0f > mDimenPrecalc.textBounds.w) {
        auto bottomScroll = mCaretPrecalc.position.y - mDimenPrecalc.textBounds.w + mScroll.y + lineHeight / 2.0f;
        scrollToPosition.y = bottomScroll;
        doScroll = true;
    }
    else {
        // Revert back original scroll value
        scrollToPosition.y = -scrollToPosition.y;
    }

    if (doScroll) {
        scrollTo(scrollToPosition.x, scrollToPosition.y);
    }

    mDirtyBit &= ~TRY_SCROLL_TO_BORDERS;
}

void CursorRenderer::checkLine(size_t index, CheckLine check) {
    std::list<std::pair<size_t, float>>::iterator it;

    if (check == DELETED) {
        // Find the precalculated line width item
        it = std::find_if(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](const std::pair<size_t, float>& pair) {
            return pair.first == index;
        });

        if (it == mDimenPrecalc.lineWidth.end()) {
            // Should never goes here
           return;
        }

        // Decrease each line number after and after index in the precalc
        std::for_each(mDimenPrecalc.lineWidth.begin(), mDimenPrecalc.lineWidth.end(), [&](std::pair<size_t, float>& pair) {
            if (pair.first >= index) {
                --pair.first;
            }
        });

        // Trigger dirty bits
        mDirtyBit |= CALCULATE_MAX_SCROLL;
        mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
        mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;

        // Erase the line from the cached list
        mDimenPrecalc.lineWidth.erase(it);
    } else {
        auto changedLine = mCursor->stringView(index);
        auto changedLineWidth = mFontTexture->measure(changedLine).x;
        auto& longuestLineWidth = mDimenPrecalc.lineWidth.back();

        switch (check) {
        case CHANGED:
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
                mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
                mDirtyBit |= CALCULATE_MAX_SCROLL;
            } else {
                if (changedLineWidth > longuestLineWidth.second) {
                    // The longuest line changed and the max scrolls
                    mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
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

            // Trigger dirty bits
            mDirtyBit |= CALCULATE_LONGUEST_LINE_NUMBER;
            mDirtyBit |= INVALIDATE_SCROLL_INDICATORS;
            mDirtyBit |= CALCULATE_MAX_SCROLL;
        break;
        default: break;
        }
    }

    // TODO check if in visible area before trigger that bit
    mDirtyBit |= TEXT_CHANGED;
}
