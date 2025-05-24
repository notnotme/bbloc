#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <memory>
#include <string_view>

#include "Command.h"


template<class TPayload>
class CommandRegistry {
public:
    /** @brief Virtual destructor for inheritance. */
    virtual ~CommandRegistry() = default;

    /**
     * @brief Register a command into the registry.
     * @param name The name of the command to register.
     * @param command A shared pointer to the command to register.
     */
    virtual void registerCommand(std::string_view name, std::shared_ptr<Command<TPayload>> command) = 0;
};


#endif //COMMAND_REGISTRY_H
