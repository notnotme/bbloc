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


std::string Platform::assetPath(const std::string_view relative) {
    // Assets live in ./romfs relative to the working directory: the path is already correct.
    return std::string(relative);
}

std::optional<Platform::ColorScheme> Platform::preferredColorScheme() {
    // No system-theme query wired up on desktop (SDL exposes none in 2.x).
    return std::nullopt;
}

std::string_view Platform::keyboardLayout() {
    // No layout query wired up on desktop: default to qwerty, overridable via "osk layout".
    return "qwerty";
}

void Platform::addControllerMappings() {
    // No overrides: SDL's controller database matches button labels on desktop.
}
