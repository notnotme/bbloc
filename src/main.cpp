#include <memory>
#include <thread>
#include <chrono>
#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "renderer/FontTexture.h"
#include "CursorManager.h"
#include "CursorRenderer.h"
#include "Controls.h"
#include "Debug.h"

std::shared_ptr<FontTexture> fontTexture = std::make_shared<FontTexture>();
std::shared_ptr<CursorRenderer> renderer = std::make_shared<CursorRenderer>();
std::shared_ptr<CursorManager> cursorManager = std::make_shared<CursorManager>();
std::shared_ptr<Controls> controls = std::make_shared<Controls>();
std::shared_ptr<Debug> debug = std::make_shared<Debug>(cursorManager, renderer);

void bindKeyboardControls() {
    controls->bindKey(SDLK_ESCAPE, [](SDL_Keycode keyCode, uint16_t modifier) {
        // TOOD: Save modified cursor(s)
        SDL_Event quitEvent;
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
    });
    controls->bindKey(SDLK_UP, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (modifier & KMOD_SHIFT) {
            cursor->enterSelection();
        } else {
            cursor->exitSelection();
        }
        cursor->move(Cursor::Direction::UP);
    });
    controls->bindKey(SDLK_DOWN, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (modifier & KMOD_SHIFT) {
            cursor->enterSelection();
        } else {
            cursor->exitSelection();
        }
        cursor->move(Cursor::Direction::DOWN);
    });
    controls->bindKey(SDLK_LEFT, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (modifier & KMOD_SHIFT) {
            cursor->enterSelection();
            cursor->move(Cursor::Direction::LEFT);
        } else if (modifier & KMOD_ALT) {
            cursorManager->previous(true);
            renderer->bindTo(cursorManager->get(), fontTexture);
            renderer->invalidate();
        } else {
            cursor->exitSelection();
            cursor->move(Cursor::Direction::LEFT);
        }
    });
    controls->bindKey(SDLK_RIGHT, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (modifier & KMOD_SHIFT) {
            cursor->enterSelection();
            cursor->move(Cursor::Direction::RIGHT);
        } else if (modifier & KMOD_ALT) {
            cursorManager->next(true);
            renderer->bindTo(cursorManager->get(), fontTexture);
            renderer->invalidate();
        } else {
            cursor->exitSelection();
            cursor->move(Cursor::Direction::RIGHT);
        }
    });
    controls->bindKey(SDLK_BACKSPACE, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (cursor->selectionVisible()) {
            cursor->eraseSelection();
        } else {
            cursor->remove(false);
        }
    });
    controls->bindKey(SDLK_DELETE, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (cursor->selectionVisible()) {
            cursor->eraseSelection();
        } else {
            cursor->remove(true);
        }
    });
    controls->bindKey(SDLK_RETURN, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (cursor->selectionVisible()) {
            cursor->eraseSelection();
        }
        cursor->newLine();
    });
    controls->bindKey(SDLK_TAB, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (cursor->selectionVisible()) {
            cursor->eraseSelection();
        }
        cursor->insert(u'\t');
    });
    controls->bindKey(SDLK_s, [](SDL_Keycode keyCode, uint16_t modifier) {
        auto cursor = cursorManager->get();
        if (modifier & KMOD_CTRL) {
            cursor->save("test.txt");
        }
    });
};

void bindGamepadControls() {
};

int main(int argc, char *argv[]) {
    glm::ivec2 windowSize = { 1280, 720 };
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    window = SDL_CreateWindow("bbloc", 0, 0, windowSize.x, windowSize.y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    
    context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);
    SDL_GameControllerOpen(0);
    gladLoadGL();

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_WRITEMASK);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    debug->initialize(window, context);
    fontTexture->initialize(256);
    fontTexture->setFont("romfs/consola.ttf");

    renderer->initialize();
    renderer->updateDrawingBox(windowSize.x / 2.0f, windowSize.y / 2.0f, windowSize.x, windowSize.y);
    renderer->updateWindowSize(windowSize.x, windowSize.y);

    cursorManager->open("romfs/main.cpp");
    cursorManager->open("romfs/UiSystem.cpp");
    cursorManager->open("romfs/main.cpp");
    renderer->bindTo(cursorManager->get(), fontTexture);

    bindKeyboardControls();
    bindGamepadControls();

    SDL_Event event;
    auto frequency = SDL_GetPerformanceFrequency();
    auto loop = true;
    while (loop) {
        while (SDL_PollEvent(&event)) {
            debug->processEvent(&event);
            switch (event.type) {
            case SDL_QUIT:
                loop = false;
            break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    windowSize.x = event.window.data1;
                    windowSize.y = event.window.data2;
                    renderer->updateWindowSize(windowSize.x, windowSize.y);
                    renderer->updateDrawingBox(windowSize.x / 2.0f, windowSize.y / 2.0f, windowSize.x, windowSize.y);
                break;
                }
            break;
            case SDL_KEYDOWN:
                if (debug->visible()) {
                    break;
                }
                controls->keyDown(event.key.keysym.sym, event.key.keysym.mod);
            break;
            case SDL_TEXTINPUT:
                if (debug->visible()) {
                    break;
                }

                auto cursor = cursorManager->get();
                if (cursor->selectionVisible()) {
                    cursor->eraseSelection();
                }
                cursor->insert(event.text.text);
            break;
            }
        }

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glViewport(0, 0, windowSize.x, windowSize.y);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer->update((float) SDL_GetPerformanceCounter() / frequency);
        renderer->render();

        debug->render(window);
        SDL_GL_SwapWindow(window);

        // Can be used to limit cpu cycles
        // std::this_thread::sleep_for (std::chrono::milliseconds(1000 / 120));
    }

    debug->finalize();
    fontTexture->finalize();
    renderer->finalize();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
