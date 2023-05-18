#pragma once

#include <unordered_map>
#include <functional>
#include <SDL_keyboard.h>

class Controls {
public:

    Controls();
    virtual ~Controls();

    void bindKey(const SDL_Keycode keyCode, const std::function<void(SDL_Keycode, uint16_t)> action);
    void unbindKey(const SDL_Keycode keyCode);
    bool keyDown(const SDL_Keycode keyCode, uint16_t modifier);

private:

    std::unordered_map<SDL_Keycode, std::function<void(SDL_Keycode, uint16_t)>> mKeyBind;

    /// @brief Disallow copy
    Controls(const Controls& copy);
    /// @brief Disallow copy
    Controls& operator=(const Controls&);

};
