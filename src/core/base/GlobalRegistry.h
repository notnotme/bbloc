#ifndef GLOBAL_REGISTRY_H
#define GLOBAL_REGISTRY_H

#include "CommandRegistry.h"
#include "CVarRegistry.h"


/**
 * @brief Combines command and configuration variable registries into a single interface.
 *
 * This class acts as a central registry, allowing systems to register both commands and CVars
 * through a unified interface. It inherits from both CommandRegistry (templated on TPayload)
 * and CVarRegistry.
 *
 * @tparam TPayload The payload type passed to registered commands.
 */
template<typename TPayload>
class GlobalRegistry : public CommandRegistry<TPayload>, public CVarRegistry {
public:
    ~GlobalRegistry() override = default;
};


#endif //GLOBAL_REGISTRY_H
