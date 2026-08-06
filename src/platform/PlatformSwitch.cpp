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

#include <switch.h>


std::string Platform::assetPath(const std::string_view relative) {
    // Assets are packaged into the NRO and mounted on the romfs:/ device.
    constexpr auto prefix = std::string_view("romfs/");
    if (relative.starts_with(prefix)) {
        return std::string("romfs:/").append(relative.substr(prefix.size()));
    }
    return std::string(relative);
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
