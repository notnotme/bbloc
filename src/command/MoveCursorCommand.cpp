/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MoveCursorCommand.h"

#include "../core/theme/DimensionId.h"


const U16StringMap<MoveCursorCommand::Movement> MoveCursorCommand::MOVEMENT_MAP = {
    { u"up", Movement::Up },
    { u"down", Movement::Down },
    { u"left", Movement::Left },
    { u"right", Movement::Right },
    { u"bol", Movement::BeginLine },
    { u"eol", Movement::EndLine },
    { u"page_up", Movement::PageUp },
    { u"page_down", Movement::PageDown },
    { u"bof", Movement::BeginFile },
    { u"eof",  Movement::EndFile }
};

const U16StringMap<MoveCursorCommand::Boolean> MoveCursorCommand::BOOLEAN_MAP = {
    { u"true", Boolean::True },
    { u"false", Boolean::False }
};

MoveCursorCommand::MoveCursorCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void MoveCursorCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
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

std::optional<std::u16string> MoveCursorCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.empty() || args.size() > 2) {
        return u"Usage: move <direction> [selected]";
    }

    // Tries to map the movement argument
    const auto movement = mapMovement(args[0]);
    if (movement == Movement::Unknown) {
        return std::u16string(u"Unknown direction argument: ").append(args[0]);
    }

    // Tries to map the selected argument
    const auto has_select_argument = args.size() == 2;
    const auto select_text = has_select_argument
        // User decide.
        ? mapBoolean(args[1])
        // Don't enable or expand the selection by default.
        : Boolean::False;

    if (has_select_argument && select_text == Boolean::Unknown) {
        // If the user precise the select argument but it cannot be parsed.
        return std::u16string(u"Selected argument expect a boolean value: ").append(args[1]);
    }

    // Move cursor according to the effective focus target: an OSK arrow key tapped over
    // an active prompt must drive the prompt, exactly like a physical arrow would.
    // If any movement happens, then the view must be redrawn.
    switch (payload.effectiveFocus()) {
        case FocusTarget::Prompt:
            switch (movement) {
                // History is not navigable while a feedback expects an answer.
                case Movement::Up:
                    if (!payload.command_feedback && m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();

                        const auto command = m_prompt_state.previousHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                break;
                case Movement::Down:
                    if (!payload.command_feedback && m_prompt_state.getHistoryCount() > 0) {
                        m_prompt_state.clearCompletions();

                        const auto command = m_prompt_state.nextHistory();
                        payload.prompt_cursor.clear();
                        payload.prompt_cursor.insert(command);
                        payload.wants_redraw = true;
                    }
                break;
                case Movement::Left:
                    payload.prompt_cursor.moveLeft();
                    payload.wants_redraw = true;
                break;
                case Movement::Right:
                    payload.prompt_cursor.moveRight();
                    payload.wants_redraw = true;
                break;
                case Movement::BeginLine:
                    payload.prompt_cursor.moveToStart();
                    payload.wants_redraw = true;
                break;
                case Movement::EndLine:
                    payload.prompt_cursor.moveToEnd();
                    payload.wants_redraw = true;
                break;
                default:
                    return std::nullopt;
            }
        break;
        // effectiveFocus never yields Osk; the case only keeps the switch exhaustive.
        case FocusTarget::Osk:
        case FocusTarget::Editor:
            payload.search.resetMatches();
            payload.cursor.activateSelection(select_text == Boolean::True);
            switch (movement) {
                case Movement::Up:
                    payload.cursor.moveUp();
                    stickToColumn(payload);
                break;
                case Movement::Down:
                    payload.cursor.moveDown();
                    stickToColumn(payload);
                break;
                case Movement::Left:
                    payload.cursor.moveLeft();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                case Movement::Right:
                    payload.cursor.moveRight();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                case Movement::BeginLine:
                    payload.cursor.moveToStartOfLine();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                case Movement::EndLine:
                    payload.cursor.moveToEndOfLine();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                case Movement::PageUp: {
                    // The dimension CVar is a plain int; the page size enters the buffer domain here
                    const auto line_count = static_cast<uint32_t>(payload.theme.getDimension(DimensionId::PageUpDown));
                    payload.cursor.pageUp(line_count);
                    stickToColumn(payload);
                }
                break;
                case Movement::PageDown: {
                    // The dimension CVar is a plain int; the page size enters the buffer domain here
                    const auto line_count = static_cast<uint32_t>(payload.theme.getDimension(DimensionId::PageUpDown));
                    payload.cursor.pageDown(line_count);
                    stickToColumn(payload);
                }
                break;
                case Movement::BeginFile:
                    payload.cursor.moveToStartOfFile();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                case Movement::EndFile:
                    payload.cursor.moveToEndOfFile();
                    payload.stick.index = payload.cursor.getColumn();
                break;
                default:
                    return std::nullopt;
            }

            payload.scroll.follow_indicator = true;
            payload.wants_redraw = true;
        break;
        default:
        return std::nullopt;
    }

    return std::nullopt;
}

MoveCursorCommand::Movement MoveCursorCommand::mapMovement(const std::u16string_view movement) {
    if (const auto &mapped_movement = MOVEMENT_MAP.find(movement); mapped_movement != MOVEMENT_MAP.end()) {
        return mapped_movement->second;
    }

    return Movement::Unknown;
}

MoveCursorCommand::Boolean MoveCursorCommand::mapBoolean(const std::u16string_view value) {
    if (const auto &mapped_boolean = BOOLEAN_MAP.find(value); mapped_boolean != BOOLEAN_MAP.end()) {
        return mapped_boolean->second;
    }

    return Boolean::Unknown;
}

void MoveCursorCommand::stickToColumn(CursorContext &payload) {
    if (payload.stick.active) {
        const auto cursor_line = payload.cursor.getLine();
        const auto string_length = static_cast<uint32_t>(payload.cursor.getString().length());
        const auto new_column = payload.stick.index > string_length
            ? string_length
            : payload.stick.index;

        payload.cursor.setPosition(cursor_line, new_column);
    }

    const auto new_column = payload.cursor.getColumn();
    payload.stick.active = payload.stick.index >= new_column;
}


