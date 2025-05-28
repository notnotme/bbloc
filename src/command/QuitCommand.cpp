#include "QuitCommand.h"

#include <SDL_events.h>


void QuitCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> QuitCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Push an SDL_QUIT event, and the event loop will catch it.
    SDL_Event event { .type = SDL_QUIT };
    SDL_PushEvent(&event);
    return std::nullopt;
}
