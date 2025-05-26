#include "MoveCursorCommand.h"

#include "../core/theme/DimensionId.h"

MoveCursorCommand::MoveCursorCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void MoveCursorCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    if (argumentIndex == 0) {
        const auto movement = { "up", "down", "left", "right", "bol", "eol", "page_up", "page_down", "bof", "eof" };
        for (const auto &item : movement) {
            itemCallback(item);
        }
    } else if (argumentIndex == 1) {
        const auto selected = { "true", "false" };
        for (const auto &item : selected) {
            itemCallback(item);
        }
    }
}

std::optional<std::u16string> MoveCursorCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.empty() || args.size() > 2) {
        return u"Usage: move <direction> [selected]";
    }

    int direction;
    if (args[0] == u"up") {
        direction = 0;
    } else if (args[0] == u"down") {
        direction = 1;
    } else if (args[0] == u"left") {
        direction = 2;
    } else if (args[0] == u"right") {
        direction = 3;
    } else if (args[0] == u"bol") {
        direction = 4;
    } else if (args[0] == u"eol") {
        direction = 5;
    } else if (args[0] == u"page_up") {
        direction = 6;
    } else if (args[0] == u"page_down") {
        direction = 7;
    }  else if (args[0] == u"bof") {
        direction = 8;
    } else if (args[0] == u"eof") {
        direction = 9;
    } else {
        return std::u16string(u"Unknown direction argument: ").append(args[0]);
    }

    bool selected;
    if (args.size() == 2) {
        if (args[1] == u"true") {
            selected = true;
        } else if (args[1] == u"false") {
            selected = false;
        } else {
            return std::u16string(u"Selected argument expect a boolean value: ").append(args[1]);
        }
    } else {
        selected = false;
    }

    switch (payload.focus_target) {
        case FocusTarget::Prompt: {
            switch (direction) {
                case 0: {
                    if (m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();
                        const auto command = m_prompt_state.previousHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                    break;
                }
                case 1: {
                    if (m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();
                        const auto command = m_prompt_state.nextHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                    break;
                }
                case 2:
                    payload.prompt_cursor.moveLeft();
                    payload.wants_redraw = true;
                    break;
                case 3:
                    payload.prompt_cursor.moveRight();
                    payload.wants_redraw = true;
                    break;
                case 4:
                    payload.prompt_cursor.moveToStart();
                    payload.wants_redraw = true;
                    break;
                case 5:
                    payload.prompt_cursor.moveToEnd();
                    payload.wants_redraw = true;
                    break;
                default:
                    // No-op
                    break;
            }

            return std::nullopt;
        }
        case FocusTarget::Editor: {
            payload.cursor.activateSelection(selected);
            switch (direction) {
                default:
                case 0: payload.cursor.moveUp(); break;
                case 1: payload.cursor.moveDown(); break;
                case 2: payload.cursor.moveLeft(); break;
                case 3: payload.cursor.moveRight(); break;
                case 4: payload.cursor.moveToStartOfLine(); break;
                case 5: payload.cursor.moveToEndOfLine(); break;
                case 6: payload.cursor.pageUp(payload.theme.getDimension(DimensionId::PageUpDown)); break;
                case 7: payload.cursor.pageDown(payload.theme.getDimension(DimensionId::PageUpDown)); break;
                case 8: payload.cursor.moveToStartOfFile(); break;
                case 9: payload.cursor.moveToEndOfFile(); break;
            }

            payload.wants_redraw = true;
            payload.follow_indicator = true;
            return std::nullopt;
        }
        default:
        return std::nullopt;
    }
}
