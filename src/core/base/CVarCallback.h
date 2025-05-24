#ifndef CVAR_CALLBACK_H
#define CVAR_CALLBACK_H

#include <functional>


/**
 * @brief Callback function type invoked when a configuration variable (CVar) is modified.
 *
 * This callback is called after a CVar's value is successfully changed, allowing
 * the application to respond to the change (e.g., updating UI, triggering side effects).
 */
using CVarCallback = std::function<
    void()
>;


#endif //CVAR_CALLBACK_H
