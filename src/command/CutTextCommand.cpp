#include "CutTextCommand.h"

#include "CopyTextCommand.h"


void CutTextCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> CutTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Copy first: on failure the selection is left untouched.
    if (const auto &error = CopyTextCommand::copySelectionToClipboard(payload)) {
        return error;
    }

    if (const auto &edit = payload.cursor.eraseSelection()) {
        // If we had some text selected, then erase it.
        payload.highlighter.edit(edit.value());
        payload.cursor.activateSelection(false);

        // If we cut some text, a redrawing is needed.
        payload.wants_redraw = true;
    }

    return std::nullopt;
}
