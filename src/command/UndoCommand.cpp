#include "UndoCommand.h"


void UndoCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> UndoCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    const auto &edit = payload.cursor.undo();
    if (!edit) {
        return u"Nothing to undo.";
    }

    payload.highlighter.edit(edit.value());
    payload.cursor.activateSelection(false);
    payload.stick_column_index = payload.cursor.getColumn();

    // Redraw and follow the cursor.
    payload.wants_redraw = true;
    payload.follow_indicator = true;
    return std::nullopt;
}
