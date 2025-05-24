#ifndef CVAR_REGISTRY_H
#define CVAR_REGISTRY_H

#include <memory>
#include <string_view>

#include "CVar.h"
#include "CVarCallback.h"


class CVarRegistry {
public:
    virtual ~CVarRegistry() = default;

    /**
     * @brief Registers a new configuration variable (CVar).
     * @param name Variable name.
     * @param cvar Shared pointer to the CVar instance.
     * @param callback Optional callback invoked when the variable is modified.
     */
    virtual void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) = 0;
};


#endif //CVAR_REGISTRY_H
