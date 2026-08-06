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
#include "Platform.h"

#include <SDL.h>
#include <switch.h>


std::string Platform::assetPath(const std::string_view relative) {
    // Assets are packaged into the NRO and mounted on the romfs:/ device.
    constexpr auto prefix = std::string_view("romfs/");
    if (relative.starts_with(prefix)) {
        return std::string("romfs:/").append(relative.substr(prefix.size()));
    }
    return std::string(relative);
}

std::optional<std::string> Platform::userConfigDir(const std::string_view executablePath) {
    // romfs:/ is read-only, so the editable autoexec goes beside the NRO, whose path hbmenu
    // passes as argv[0] ("sdmc:/switch/bbloc.nro"). Without it there is nowhere to write.
    const auto separator = executablePath.rfind('/');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }
    return std::string(executablePath.substr(0, separator + 1));
}

std::string_view Platform::keyboardLayout() {
    // Same source the patched SDL keyboard driver reads; the name mapping mirrors its
    // GetLayoutTable(): layouts without a table (CJK, FrenchCa, UsInternational) use qwerty.
    if (R_FAILED(setsysInitialize())) {
        return "qwerty";
    }

    auto layout = SetKeyboardLayout_EnglishUs;
    const auto result = setsysGetKeyboardLayout(&layout);
    setsysExit();

    if (R_FAILED(result)) {
        return "qwerty";
    }

    switch (layout) {
        case SetKeyboardLayout_French:
            return "azerty";
        case SetKeyboardLayout_EnglishUk:
            return "uk";
        case SetKeyboardLayout_Spanish:
            return "spanish";
        case SetKeyboardLayout_SpanishLatin:
            return "spanish_latin";
        case SetKeyboardLayout_German:
            return "qwertz";
        case SetKeyboardLayout_Italian:
            return "italian";
        case SetKeyboardLayout_Portuguese:
            return "portuguese";
        case SetKeyboardLayout_Russian:
            return "russian";
        default:
            return "qwerty";
    }
}

std::optional<Platform::ColorScheme> Platform::preferredColorScheme() {
    // setsysInitialize is reference-counted: harmless if the patched SDL already holds it.
    if (R_FAILED(setsysInitialize())) {
        return std::nullopt;
    }

    auto color_set = ColorSetId_Light;
    const auto result = setsysGetColorSetId(&color_set);
    setsysExit();

    if (R_FAILED(result)) {
        return std::nullopt;
    }
    return color_set == ColorSetId_Dark ? ColorScheme::Dark : ColorScheme::Light;
}

void Platform::addControllerMappings() {
    // The port's database entry for the console pad ("Switch Controller" GUID) is
    // positional: a:b1,b:b0,x:b3,y:b2 — SDL A lands on the button labeled B. This
    // label-true override keeps every other assignment identical, so "pad:a" is the
    // labeled A everywhere. Single sideways Joy-Cons keep the driver's positional
    // remap tables and are not corrected here.
    SDL_GameControllerAddMapping(
        "000038f853776974636820436f6e7400,Switch Controller,"
        "a:b0,b:b1,x:b2,y:b3,back:b11,start:b10,"
        "dpdown:b15,dpleft:b12,dpright:b14,dpup:b13,"
        "leftshoulder:b6,rightshoulder:b7,lefttrigger:b8,righttrigger:b9,"
        "leftstick:b4,rightstick:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,");
}
