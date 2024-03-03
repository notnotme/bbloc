#pragma once

#include <unordered_map>
#include <functional>
#include <SDL_keyboard.h>

class Controls {
public:

    Controls();

    virtual ~Controls();

    /// @brief Bind a key to an action
    /// @param keyCode The SDL_Keycode to bind
    /// @param action The std::function to invoke when the key is pressed
    void bindKey(const SDL_Keycode keyCode, const std::function<void(SDL_Keycode, uint16_t)> action);


    /// @brief Bind a text input to an action
    /// @param input The std::function to invoke when the text input is triggered from SDL
    void bindTextInput(const std::function<void(const char*)> input);

    /// @brief Remove a previously bind key
    /// @param keyCode The SDL_Keycode to unbind
    void unbindKey(const SDL_Keycode keyCode);

    /// @brief Remove the text input action
    void unbindTextInput();

    /// @brief To be called when a keydown event is detected, this will trigger the action bind to this key, if any.
    bool keyDown(const SDL_Keycode keyCode, uint16_t modifier);

    /// @brief To be called when a text input event is detected, this will trigger the action bind to text input
    void textInput(const char* text);

private:

    /// @brief An unordered map to store the key binding
    std::unordered_map<SDL_Keycode, std::function<void(SDL_Keycode, uint16_t)>> mKeyBind;

    /// @brief The binding to text input event
    std::function<void(const char*)> mTextInput;

    /// @brief Disallow copy
    Controls(const Controls& copy);

    /// @brief Disallow copy
    Controls& operator=(const Controls&);

};
