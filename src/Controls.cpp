#include "Controls.h"

Controls::Controls() :
mTextInput(nullptr) {
}

Controls::~Controls() {
}

void Controls::bindKey(const SDL_Keycode keyCode, const std::function<void(SDL_Keycode, uint16_t)> action) {
    mKeyBind.emplace(keyCode, action);
}

void Controls::bindTextInput(const std::function<void(const char*)> input) {
    mTextInput = input;
}

void Controls::unbindKey(const SDL_Keycode keyCode) {
    if (mKeyBind.find(keyCode) != mKeyBind.end()) {
        mKeyBind.erase(keyCode);
    }
}

void Controls::unbindTextInput() {
    mTextInput = nullptr;
}


bool Controls::keyDown(const SDL_Keycode keyCode, uint16_t modifier) {
    if (mKeyBind.find(keyCode) != mKeyBind.end()) {
        mKeyBind[keyCode](keyCode, modifier);
        return true;
    }
    return false;
}

void Controls::textInput(const char* text) {
    if (mTextInput != nullptr) {
        mTextInput(text);
    }
}
