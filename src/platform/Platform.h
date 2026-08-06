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
#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/**
 * @brief Platform seam: the few places where desktop and Nintendo Switch behavior differ.
 *
 * Exactly one implementation is compiled in, selected by CMake: PlatformDesktop.cpp on desktop,
 * PlatformSwitch.cpp on Switch (the only file allowed to include <switch.h>).
 */
class Platform final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    Platform() = delete;

    /** @brief System-wide color scheme preference. */
    enum class ColorScheme : uint8_t {
        Light,
        Dark,
    };

    /**
     * @brief Resolves a `romfs/`-relative asset path for the running platform.
     *
     * Paths are written `romfs/...` everywhere (code, autoexec, exec scripts); on Switch the
     * prefix is rewritten to the `romfs:/` device, on desktop the path is returned unchanged.
     *
     * @param relative The path as written, e.g. "romfs/autoexec". UTF-8.
     * @return The path to hand to the file system. UTF-8.
     */
    [[nodiscard]] static std::string assetPath(std::string_view relative);

    /**
     * @brief Queries the system-wide light/dark preference.
     *
     * @return The console color set on Switch; std::nullopt on desktop (no query wired up).
     */
    [[nodiscard]] static std::optional<ColorScheme> preferredColorScheme();
};


#endif //PLATFORM_H
