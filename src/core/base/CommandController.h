#ifndef COMMAND_CONTROLLER_H
#define COMMAND_CONTROLLER_H

#include "CommandRegistry.h"
#include "CVarRegistry.h"


template<typename TPayload>
class CommandController : public CommandRegistry<TPayload>, public CVarRegistry {
public:
    ~CommandController() override = default;
};


#endif //COMMAND_CONTROLLER_H
