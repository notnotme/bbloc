#ifndef COMMAND_CONTROLLER_H
#define COMMAND_CONTROLLER_H

#include "CommandRegistry.h"
#include "CVarRegistry.h"


/**
 * @brief Combines command and configuration variable registries into a single interface.
 *
 * This class acts as a central controller, allowing systems to register both commands and CVars
 * through a unified interface. It inherits from both CommandRegistry (templated on TPayload)
 * and CVarRegistry, bridging command execution and configuration management layers.
 *
 * @tparam TPayload The payload type passed to registered commands.
 */
template<typename TPayload>
class CommandController : public CommandRegistry<TPayload>, public CVarRegistry {
public:
    ~CommandController() override = default;
};


#endif //COMMAND_CONTROLLER_H
