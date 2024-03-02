#pragma once

#include <unordered_map>
#include <functional>
#include <SDL_keyboard.h>

class Controls {
public:

    Controls();
    virtual ~Controls();

    void bindKey(const SDL_Keycode keyCode, const std::function<void(SDL_Keycode, uint16_t)> action);
    void bindTextInput(const std::function<void(const char*)> input);
    void unbindKey(const SDL_Keycode keyCode);
    void unbindTextInput();
    bool keyDown(const SDL_Keycode keyCode, uint16_t modifier);
    void textInput(const char* text);

private:

    std::unordered_map<SDL_Keycode, std::function<void(SDL_Keycode, uint16_t)>> mKeyBind;
    std::function<void(const char*)> mTextInput;

    /// @brief Disallow copy
    Controls(const Controls& copy);
    /// @brief Disallow copy
    Controls& operator=(const Controls&);

};
