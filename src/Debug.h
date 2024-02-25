#pragma once

#include <memory>
#include <SDL_events.h>
#include <SDL_video.h>
#include "CursorManager.h"
#include "CursorRenderer.h"

class Debug {
public:
    Debug(std::shared_ptr<CursorManager> manager, std::shared_ptr<CursorRenderer> renderer);
    virtual ~Debug();
    void initialize(SDL_Window* window, SDL_GLContext context);
    void finalize();
    void processEvent(SDL_Event* event);
    void render(SDL_Window* window);
    bool visible();
private:
    std::shared_ptr<CursorManager> mCursorManager;
    std::shared_ptr<CursorRenderer> mCursorRenderer;
    bool mShowDebugWindow;
    bool mVsync;
};
