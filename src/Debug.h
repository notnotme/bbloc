#pragma once

#include <memory>
#include <SDL_events.h>
#include <SDL_video.h>
#include "CursorRenderer.h"
#include "Cursor.h"

class Debug {
public:
    Debug(std::shared_ptr<Cursor> cursor, std::shared_ptr<CursorRenderer> renderer);
    virtual ~Debug();
    void initialize(SDL_Window* window, SDL_GLContext context);
    void finalize();
    void processEvent(SDL_Event* event);
    void render(SDL_Window* window);
    bool visible();
private:
    std::shared_ptr<Cursor> mCursor;
    std::shared_ptr<CursorRenderer> mCursorRenderer;
    bool mShowDebugWindow;
    bool mVsync;
};
