#include <memory>
#include <thread>
#include <chrono>
#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "renderer/FontTexture.h"
#include "cursor/VectorCursor.h" 
#include "cursor/StringCursor.h"
#include "cursor/RopeCursor.h"
#include "CursorRenderer.h"
#include "Debug.h"

int main(int argc, char *argv[]) {
    glm::ivec2 windowSize = { 1280, 720 };
    std::shared_ptr<FontTexture> fontTexture = std::make_shared<FontTexture>();
    std::shared_ptr<CursorRenderer> renderer = std::make_shared<CursorRenderer>();
    std::shared_ptr<Cursor> cursor = std::make_shared<RopeCursor>();
    std::shared_ptr<Debug> debug = std::make_shared<Debug>(cursor, renderer);

    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
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

    cursor->load("romfs/UiSystem.cpp");
    fontTexture->initialize(256);
    fontTexture->setFont("romfs/consola.ttf");
    renderer->initialize();
    renderer->updateDrawingBox(windowSize.x / 2.0f, windowSize.y / 2.0f, windowSize.x, windowSize.y);
    renderer->updateWindowSize(windowSize.x, windowSize.y);
    renderer->bindTo(cursor, fontTexture);

    SDL_Event event;
    auto frequency = SDL_GetPerformanceFrequency();
    auto loop = true;
    auto prompt = false;
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
                if (prompt) {
                    // User input in the prompt area
                    // ... code ...
                } else {
                    // In editor mode
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        // TOOD: Save modified cursor(s)
                        SDL_Event quitEvent;
                        quitEvent.type = SDL_QUIT;
                        SDL_PushEvent(&quitEvent);
                    break;
                    case SDLK_UP:
                        cursor->move(Cursor::Direction::UP);
                    break;
                    case SDLK_DOWN:
                        cursor->move(Cursor::Direction::DOWN);
                    break;
                    case SDLK_LEFT:
                        cursor->move(Cursor::Direction::LEFT);
                    break;
                    case SDLK_RIGHT:
                        cursor->move(Cursor::Direction::RIGHT);
                    break;
                    case SDLK_BACKSPACE:
                        cursor->remove();
                    break;
                    case SDLK_RETURN:
                        cursor->newLine();
                    break;
                    case SDLK_TAB:
                        cursor->insert(u'\t');
                    break;
                    }
                }
            break;
            case SDL_TEXTINPUT:
                if (debug->visible()) {
                    break;
                }
                if (prompt) {
                    // User input in the prompt area
                    // ... code ..
                } else {
                    cursor->insert(event.text.text);
                }
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
