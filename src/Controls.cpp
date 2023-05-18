#include "Controls.h"

Controls::Controls() {
}

Controls::~Controls() {
}

void Controls::bindKey(const SDL_Keycode keyCode, const std::function<void(SDL_Keycode, uint16_t)> action) {
    mKeyBind.emplace(keyCode, action);
}

void Controls::unbindKey(const SDL_Keycode keyCode) {
    if (mKeyBind.find(keyCode) != mKeyBind.end()) {
        mKeyBind.erase(keyCode);
    }
}

bool Controls::keyDown(const SDL_Keycode keyCode, uint16_t modifier) {
    if (mKeyBind.find(keyCode) != mKeyBind.end()) {
        mKeyBind[keyCode](keyCode, modifier);
        return true;
    }
    return false;
}
