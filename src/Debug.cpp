#include "Debug.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

Debug::Debug(std::shared_ptr<Cursor> cursor, std::shared_ptr<CursorRenderer> renderer) :
mCursor(cursor),
mCursorRenderer(renderer),
mShowDebugWindow(false),
mVsync(true) {
}

Debug::~Debug() {
}

void Debug::initialize(SDL_Window* window, SDL_GLContext context) {
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    auto& imgui = ImGui::GetIO();
    imgui.LogFilename = nullptr;
    imgui.IniFilename = nullptr;
    imgui.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    mShowDebugWindow = true;
}

void Debug::finalize() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(nullptr);
}

void Debug::processEvent(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
#ifdef DEBUG
    switch (event->type) {
    case SDL_KEYDOWN:
        // Temporary quick escape for development
       if (event->key.keysym.sym == SDLK_ESCAPE) {
            SDL_Event quitEvent;
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        // Temporary quick debug show/hide for development
        if (event->key.keysym.sym == SDLK_F12) {
            mShowDebugWindow = !mShowDebugWindow;
        }
    break;
    }
#endif
}

void Debug::render(SDL_Window* window) {
    glm::ivec2 windowSize;
    SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
    auto left = glm::floor(mCursorRenderer->mDrawingBox.position.x - (mCursorRenderer->mDrawingBox.size.x / 2.0f));
    auto right = glm::floor(mCursorRenderer->mDrawingBox.position.x + (mCursorRenderer->mDrawingBox.size.x / 2.0f));
    auto bottom = glm::floor(mCursorRenderer->mDrawingBox.position.y + (mCursorRenderer->mDrawingBox.size.y / 2.0f));
    auto top = glm::floor(mCursorRenderer->mDrawingBox.position.y - (mCursorRenderer->mDrawingBox.size.y / 2.0f));
    auto& io = ImGui::GetIO();
    static auto bUpdate = 0.0f;
    static auto bRender = 0.0f;
    if (bUpdate < mCursorRenderer->mUpdateTime) bUpdate = mCursorRenderer->mUpdateTime;
    if (bRender < mCursorRenderer->mRenderTime) bRender = mCursorRenderer->mRenderTime;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    //ImGui::ShowDemoWindow();
    if (mShowDebugWindow) {
        ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2, windowSize.y / 2), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("Debug", &mShowDebugWindow, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Menu")) {
                    if (ImGui::MenuItem("Quit", nullptr)) {
                        SDL_Event quitEvent;
                        quitEvent.type = SDL_QUIT;
                        SDL_PushEvent(&quitEvent);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::BeginTabBar("DebugTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Renderer")) {
                    ImGui::LabelText("Update", "%.3f, %.3f", mCursorRenderer->mUpdateTime, bUpdate);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset##bUpdate")) bUpdate = 0.0f;
                    ImGui::LabelText("Render", "%.3f, %.3f", mCursorRenderer->mRenderTime, bRender);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset##bRender")) bRender = 0.0f;
                    ImGui::SeparatorText("DrawingBox");
                    ImGui::LabelText("Screen coordinates", "%.1f, %.1f, %.1f, %.1f", left, top, right, bottom);
                    auto drawingBox = mCursorRenderer->mDrawingBox;
                    if (ImGui::InputFloat("Position X", &drawingBox.position.x, 1, 5)) {
                        mCursorRenderer->updateDrawingBox(drawingBox.position.x, drawingBox.position.y, drawingBox.size.x, drawingBox.size.y);
                    }
                    if (ImGui::InputFloat("Position Y", &drawingBox.position.y, 1, 5)) {
                        mCursorRenderer->updateDrawingBox(drawingBox.position.x, drawingBox.position.y, drawingBox.size.x, drawingBox.size.y);
                    }
                    if (ImGui::InputFloat("Size X", &drawingBox.size.x, 1, 5)) {
                        mCursorRenderer->updateDrawingBox(drawingBox.position.x, drawingBox.position.y, drawingBox.size.x, drawingBox.size.y);
                    }
                    if (ImGui::InputFloat("Size Y", &drawingBox.size.y, 1, 5)) {
                        mCursorRenderer->updateDrawingBox(drawingBox.position.x, drawingBox.position.y, drawingBox.size.x, drawingBox.size.y);
                    }
                    ImGui::SeparatorText("Draw bits");
                    auto showScrollbar = mCursorRenderer->drawBit(CursorRenderer::DrawBit::SCROLL_BAR);
                    if (ImGui::Checkbox("Show scrollbar", &showScrollbar)) {
                        if (showScrollbar) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::SCROLL_BAR);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::SCROLL_BAR);
                    }
                    auto hightlightCurrentLine = mCursorRenderer->drawBit(CursorRenderer::DrawBit::HIGHTLIGHT_CURRENT_LINE);
                    if (ImGui::Checkbox("Hilight current line", &hightlightCurrentLine)) {
                        if (hightlightCurrentLine) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::HIGHTLIGHT_CURRENT_LINE);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::HIGHTLIGHT_CURRENT_LINE);
                    }
                    auto showLeftMargin = mCursorRenderer->drawBit(CursorRenderer::DrawBit::LEFT_MARGIN);
                    if (ImGui::Checkbox("Show left margin", &showLeftMargin)) {
                        if (showLeftMargin) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::LEFT_MARGIN);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::LEFT_MARGIN);
                    }
                    ImGui::BeginDisabled(!showLeftMargin);
                    auto showLineNumber = mCursorRenderer->drawBit(CursorRenderer::DrawBit::LINE_NUMBER);
                    if (ImGui::Checkbox("Show line number", &showLineNumber)) {
                        if (showLineNumber) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::LINE_NUMBER);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::LINE_NUMBER);
                    }
                    auto showLineNumberIndicator = mCursorRenderer->drawBit(CursorRenderer::DrawBit::LINE_NUMBER_INDICATOR);
                    if (ImGui::Checkbox("Show line number indicator", &showLineNumberIndicator)) {
                        if (showLineNumberIndicator) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::LINE_NUMBER_INDICATOR);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::LINE_NUMBER_INDICATOR);
                    }
                    ImGui::EndDisabled();
                    auto showStatusBar = mCursorRenderer->drawBit(CursorRenderer::DrawBit::STATUS_BAR);
                    if (ImGui::Checkbox("Show status bar", &showStatusBar)) {
                        if (showStatusBar) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::STATUS_BAR);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::STATUS_BAR);
                    }
                    ImGui::SeparatorText("Other");
                    bool scissor = mCursorRenderer->drawBit(CursorRenderer::DrawBit::SCISSOR);
                    if (ImGui::Checkbox("Enable scissor", &scissor)) {
                        if (scissor) mCursorRenderer->enableDrawBit(CursorRenderer::DrawBit::SCISSOR);
                        else mCursorRenderer->disableDrawBit(CursorRenderer::DrawBit::SCISSOR);
                    }
                    if (ImGui::Checkbox("Enable Vsync", &mVsync)) {
                        if (mVsync) SDL_GL_SetSwapInterval(1);
                        else SDL_GL_SetSwapInterval(0);
                    }
                    ImGui::LabelText("SpriteBuffer count", "%lu", mCursorRenderer->mSpriteBuffer->index());
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Cursor")) {
                    ImGui::LabelText("Lines", "%li", mCursor->size());
                    ImGui::SeparatorText("Caret");
                    ImGui::LabelText("Position", "%i, %i", mCursor->mPosition.x, mCursor->mPosition.y);
                    if (ImGui::ArrowButton("CaretUp", ImGuiDir_Up)) mCursor->move(Cursor::Direction::UP);
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("CaretDown", ImGuiDir_Down)) mCursor->move(Cursor::Direction::DOWN);
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("CaretLeft", ImGuiDir_Left)) mCursor->move(Cursor::Direction::LEFT);
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("CaretRight", ImGuiDir_Right)) mCursor->move(Cursor::Direction::RIGHT);
                    ImGui::SameLine();
                    if (ImGui::Button("BOF")) mCursor->bof();
                    ImGui::SameLine();
                    if (ImGui::Button("EOF")) mCursor->eof();
                    ImGui::SameLine();
                    if (ImGui::Button("BOL")) mCursor->bol();
                    ImGui::SameLine();
                    if (ImGui::Button("EOL")) mCursor->eol();
                    ImGui::SeparatorText("Scroll");
                    ImGui::LabelText("Max scroll", "%f, %f", mCursorRenderer->mDimenPrecalc.maxScroll.x, mCursorRenderer->mDimenPrecalc.maxScroll.y);
                    glm::vec2 scroll = mCursorRenderer->mScroll;
                    if (ImGui::SliderFloat("Scroll X", &scroll.x, 0, mCursorRenderer->mDimenPrecalc.maxScroll.x)) {
                        mCursorRenderer->scrollTo(scroll.x, scroll.y);
                    }
                    if (ImGui::SliderFloat("Scroll Y", &scroll.y, 0, mCursorRenderer->mDimenPrecalc.maxScroll.y)) {
                        mCursorRenderer->scrollTo(scroll.x, scroll.y);
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Font")) {
                    ImGui::LabelText("Font size", "%i px", mCursorRenderer->mFontTexture->fontHeightPixel());
                    ImGui::SameLine();
                    if (ImGui::Button("-")) {
                        mCursorRenderer->mFontTexture->decreaseFontSize();
                        mCursorRenderer->invalidate();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("+")) {
                        mCursorRenderer->mFontTexture->increaseFontSize();
                        mCursorRenderer->invalidate();
                    }
                    ImGui::LabelText("Line count", "%u", mCursorRenderer->mDimenPrecalc.visibleLineCount);
                    ImGui::SeparatorText("Texture");
                    auto imageSize = ImVec2(mCursorRenderer->mFontTexture->mTextureSize, mCursorRenderer->mFontTexture->mTextureSize);
                    ImGui::Text("Size %i", mCursorRenderer->mFontTexture->mTextureSize);
                    auto cursorPosition = ImGui::GetCursorPos();
                    auto center = ImVec2(cursorPosition.x + (ImGui::GetContentRegionAvail().x - imageSize.x) * 0.5f, cursorPosition.y);
                    ImGui::SetCursorPos(center);
                    ImGui::Image((void*)(intptr_t) mCursorRenderer->mFontTexture->mTextureId, imageSize);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Style")) {
                    auto style = mCursorRenderer->style();
                    glm::vec4 backgroundColor = glm::vec4(style.backgroundColor) / 255.0f;
                    glm::vec4 marginColor = glm::vec4(style.marginColor) / 255.0f;
                    glm::vec4 marginBorderColor = glm::vec4(style.marginBorderColor) / 255.0f;
                    glm::vec4 scrollbarColor = glm::vec4(style.scrollbarColor) / 255.0f;
                    glm::vec4 scrollbarIndicatorColor = glm::vec4(style.scrollbarIndicatorColor) / 255.0f;
                    glm::vec4 textColor = glm::vec4(style.textColor) / 255.0f;
                    glm::vec4 selectedTextColor = glm::vec4(style.selectedTextColor) / 255.0f;
                    glm::vec4 caretColor = glm::vec4(style.caretColor) / 255.0f;
                    glm::vec4 lineNumberColor = glm::vec4(style.lineNumberColor) / 255.0f;
                    glm::vec4 currentLineNumberColor = glm::vec4(style.currentLineNumberColor) / 255.0f;
                    glm::vec4 currentLinebackgroundColor = glm::vec4(style.currentLinebackgroundColor) / 255.0f;
                    int32_t caretWidth = style.caretWidth;
                    ImGui::SeparatorText("Editor");
                    if (ImGui::SliderInt("Scrollbar width", (int32_t*) &style.scrollbarWidth, 0, 24)) {
                         // TODO: Make power of 2 if pixels collapse
                        style.scrollbarWidth = style.scrollbarWidth;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::SliderInt("Margin border width", (int32_t*) &style.marginBorderWidth, 0, 24)) {
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::SliderInt("Caret width", &caretWidth, 1, mCursorRenderer->mCaretPrecalc.size.x)) {
                        style.caretWidth = caretWidth;
                        mCursorRenderer->style(style);
                    }
                    const char* lineIndicator_items[] = { "DOT", "ARROW", "LINE" };
                    static auto lineIndicator_current = 0;
                    if (ImGui::Combo("Line indicator", &lineIndicator_current, lineIndicator_items, IM_ARRAYSIZE(lineIndicator_items))) {
                        switch(lineIndicator_current) {
                            case 0 : style.lineIndicator = CursorRenderer::LineIndicator::DOT; break;
                            case 1 : style.lineIndicator = CursorRenderer::LineIndicator::ARROW; break;
                            case 2 : style.lineIndicator = CursorRenderer::LineIndicator::LINE; break;
                        }
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Background", &backgroundColor.r)) {
                        style.backgroundColor = backgroundColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Margin", &marginColor.r)) {
                        style.marginColor = marginColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Margin border", &marginBorderColor.r)) {
                        style.marginBorderColor = marginBorderColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Scrollbar", &scrollbarColor.r)) {
                        style.scrollbarColor = scrollbarColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Scrollbar indicator", &scrollbarIndicatorColor.r)) {
                        style.scrollbarIndicatorColor = scrollbarIndicatorColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Current line background", &currentLinebackgroundColor.r)) {
                        style.currentLinebackgroundColor = currentLinebackgroundColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    ImGui::SeparatorText("Text");
                    if (ImGui::ColorEdit4("Caret", &caretColor.r)) {
                        style.caretColor = caretColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Normal text", &textColor.r)) {
                        style.textColor = textColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Selected text", &selectedTextColor.r)) {
                        style.selectedTextColor = selectedTextColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Line number", &lineNumberColor.r)) {
                        style.lineNumberColor = lineNumberColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    if (ImGui::ColorEdit4("Current line number", &currentLineNumberColor.r)) {
                        style.currentLineNumberColor = currentLineNumberColor * 255.0f;
                        mCursorRenderer->style(style);
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Debug::visible() {
    return mShowDebugWindow;
}