#ifndef CVAR_REGISTRY_H
#define CVAR_REGISTRY_H

#include <memory>
#include <string_view>

#include "CVar.h"
#include "CVarCallback.h"


/**
 * @brief Interface for registering and handling configuration variables.
 *
 * Implementations of this class manage configuration variables (CVars)
 * and allow for dynamic updates through callback hooks.
 */
class CVarRegistry {
public:
    virtual ~CVarRegistry() = default;

    /**
     * @brief Registers a new configuration variable (CVar).
     *
     * Associates a named CVar with the registry and optionally sets a callback to be
     * triggered when the variable's value changes.
     *
     * @param name Variable name.                              ///< Unique identifier for the CVar.
     * @param cvar Shared pointer to the CVar instance.         ///< The CVar object being registered.
     * @param callback Optional callback invoked on changes.    ///< Called when the variable is modified.
     */
    virtual void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) = 0;
};


#endif //CVAR_REGISTRY_H
