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
#include "OskCommand.h"

#include <utf8.h>

#include "../osk/OskLayout.h"


OskCommand::OskCommand(OskState &oskState)
    : m_osk_state(oskState) {}

void OskCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    if (argumentIndex == 0) {
        itemCallback(u"show");
        itemCallback(u"hide");
        itemCallback(u"toggle");
        itemCallback(u"layout");
    } else if (argumentIndex == 1 && !previousArgs.empty() && previousArgs[0] == u"layout") {
        OskLayout::forEachName(itemCallback);
    }
}

std::optional<std::u16string> OskCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.empty()) {
        return u"Usage: osk <show|hide|toggle|layout>";
    }

    if (args[0] == u"layout") {
        if (args.size() != 2) {
            return u"Usage: osk layout <name>";
        }

        // Layout names are plain ASCII; the conversion feeds the UTF-8 layout registry.
        const auto *layout = OskLayout::findLayout(utf8::utf16to8(args[1]));
        if (layout == nullptr) {
            return std::u16string(u"Unknown layout: ").append(args[1]);
        }

        m_osk_state.setLayout(*layout);
        payload.wants_redraw = true;
        return std::nullopt;
    }

    if (args.size() != 1 || (args[0] != u"show" && args[0] != u"hide" && args[0] != u"toggle")) {
        return u"Usage: osk <show|hide|toggle|layout>";
    }

    const auto show = args[0] == u"show" || (args[0] == u"toggle" && !m_osk_state.isVisible());
    m_osk_state.setVisible(show);
    if (!show) {
        // Drop the transient interaction state so the next show starts clean, and release
        // the pad focus if the OSK held it.
        m_osk_state.resetInteraction();
        if (payload.focus_target == FocusTarget::Osk) {
            payload.focus_target = FocusTarget::Editor;
        }
    }
    // Showing never touches the focus: the OSK acquires the pad focus lazily, on the first
    // d-pad/A press ControllerInput routes to it, so mouse users never see the key cursor.

    // The relayout runs through the resize path; following the indicator keeps the caret
    // in view when the editor shrinks or grows back.
    payload.scroll.follow_indicator = true;
    payload.wants_redraw = true;
    return std::nullopt;
}
