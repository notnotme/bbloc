#include "MoveCursorCommand.h"

#include "../core/theme/DimensionId.h"


const std::unordered_map<std::u16string, MoveCursorCommand::Movement> MoveCursorCommand::MOVEMENT_MAP = {
    { u"up", Movement::UP },
    { u"down", Movement::DOWN },
    { u"left", Movement::LEFT },
    { u"right", Movement::RIGHT },
    { u"bol", Movement::BEGIN_FILE },
    { u"eol", Movement::END_LINE },
    { u"page_up", Movement::PAGE_UP },
    { u"page_down", Movement::PAGE_DOWN },
    { u"bof", Movement::BEGIN_FILE },
    { u"eof",  Movement::END_FILE }
};

const std::unordered_map<std::u16string, MoveCursorCommand::Boolean> MoveCursorCommand::BOOLEAN_MAP = {
    { u"true", Boolean::TRUE },
    { u"false", Boolean::FALSE }
};

MoveCursorCommand::MoveCursorCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void MoveCursorCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex == 0) {
        for (const auto &item : std::views::keys(MOVEMENT_MAP)) {
            itemCallback(item);
        }
    } else if (argumentIndex == 1) {
        for (const auto &item : std::views::keys(BOOLEAN_MAP)) {
            itemCallback(item);
        }
    }
}

std::optional<std::u16string> MoveCursorCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.empty() || args.size() > 2) {
        return u"Usage: move <direction> [selected]";
    }

    // Tries to map the movement argument
    const auto movement = mapMovement(args[0]);
    if (movement == Movement::UNKNOWN) {
        return std::u16string(u"Unknown direction argument: ").append(args[0]);
    }

    // Tries to map the selected argument
    const auto has_select_argument = args.size() == 2;
    const auto select_text = has_select_argument
        // User decide.
        ? mapBoolean(args[1])
        // Don't enable or expand the selection by default.
        : Boolean::FALSE;

    if (has_select_argument && select_text == Boolean::UNKNOWN) {
        // If the user precise the select argument but it cannot be parsed.
        return std::u16string(u"Selected argument expect a boolean value: ").append(args[1]);
    }

    // Move cursor according to focus target.
    // If any movement happens, then the view must be redrawn.
    switch (payload.focus_target) {
        case FocusTarget::Prompt:
            switch (movement) {
                case Movement::UP:
                    if (m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();

                        const auto command = m_prompt_state.previousHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                break;
                case Movement::DOWN:
                    if (m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();

                        const auto command = m_prompt_state.nextHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                break;
                case Movement::LEFT:
                    payload.prompt_cursor.moveLeft();
                    payload.wants_redraw = true;
                break;
                case Movement::RIGHT:
                    payload.prompt_cursor.moveRight();
                    payload.wants_redraw = true;
                break;
                case Movement::BEGIN_LINE:
                    payload.prompt_cursor.moveToStart();
                    payload.wants_redraw = true;
                break;
                case Movement::END_LINE:
                    payload.prompt_cursor.moveToEnd();
                    payload.wants_redraw = true;
                break;
                default:
                    return std::nullopt;
            }
        break;
        case FocusTarget::Editor:
            payload.cursor.activateSelection(select_text == Boolean::TRUE);
            switch (movement) {
                case Movement::UP:
                    payload.cursor.moveUp();
                    stickToColumn(payload);
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::DOWN:
                    payload.cursor.moveDown();
                    stickToColumn(payload);
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::LEFT:
                    payload.cursor.moveLeft();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::RIGHT:
                    payload.cursor.moveRight();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::BEGIN_LINE:
                    payload.cursor.moveToStartOfLine();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::END_LINE:
                    payload.cursor.moveToEndOfLine();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::PAGE_UP: {
                    const auto line_count = payload.theme.getDimension(DimensionId::PageUpDown);
                    payload.cursor.pageUp(line_count);
                    stickToColumn(payload);
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                }
                break;
                case Movement::PAGE_DOWN: {
                    const auto line_count = payload.theme.getDimension(DimensionId::PageUpDown);
                    payload.cursor.pageDown(line_count);
                    stickToColumn(payload);
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                }
                break;
                case Movement::BEGIN_FILE:
                    payload.cursor.moveToStartOfFile();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                case Movement::END_FILE:
                    payload.cursor.moveToEndOfFile();
                    payload.stick_column_index = payload.cursor.getColumn();
                    payload.follow_indicator = true;
                    payload.wants_redraw = true;
                break;
                default:
                    return std::nullopt;
            }
        break;
        default:
        return std::nullopt;
    }

    return std::nullopt;
}

MoveCursorCommand::Movement MoveCursorCommand::mapMovement(const std::u16string_view movement) {
    const auto movement_str = std::u16string(movement.begin(), movement.end());
    if (const auto &mapped_movement = MOVEMENT_MAP.find(movement_str); mapped_movement != MOVEMENT_MAP.end()) {
        return mapped_movement->second;
    }

    return Movement::UNKNOWN;
}

MoveCursorCommand::Boolean MoveCursorCommand::mapBoolean(const std::u16string_view value) {
    const auto boolean_str = std::u16string(value.begin(), value.end());
    if (const auto &mapped_boolean = BOOLEAN_MAP.find(boolean_str); mapped_boolean != BOOLEAN_MAP.end()) {
        return mapped_boolean->second;
    }

    return Boolean::UNKNOWN;
}

void MoveCursorCommand::stickToColumn(CursorContext &payload) {
    if (payload.stick_to_column) {
        const auto cursor_line = payload.cursor.getLine();
        const auto string_length = payload.cursor.getString().length();
        const auto new_column = payload.stick_column_index > string_length
            ? string_length
            : payload.stick_column_index;

        payload.cursor.setPosition(cursor_line, new_column);
    }

    const auto new_column = payload.cursor.getColumn();
    payload.stick_to_column = payload.stick_column_index >= new_column;
}


