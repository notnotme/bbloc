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
     * @brief Resolves the directory holding the user-editable copies of the shipped scripts.
     *
     * On Switch the packaged assets sit on the read-only `romfs:/` device, so the autoexec the
     * user can actually edit lives next to bbloc.nro; the directory is derived from the path
     * hbmenu passes as argv[0]. On desktop the shipped `romfs/` is a plain writable directory,
     * so there is no second copy and the method returns std::nullopt.
     *
     * @param executablePath argv[0], as given to main. UTF-8.
     * @return The directory, trailing separator included; std::nullopt when the shipped copy is
     *         already editable or the path cannot be derived.
     */
    [[nodiscard]] static std::optional<std::string> userConfigDir(std::string_view executablePath);

    /**
     * @brief Queries the system-wide light/dark preference.
     *
     * @return The console color set on Switch; std::nullopt on desktop (no query wired up).
     */
    [[nodiscard]] static std::optional<ColorScheme> preferredColorScheme();

    /**
     * @brief Queries the system keyboard layout, for the on-screen keyboard.
     *
     * @return An OskLayout table name: the console keyboard layout on Switch (the same source
     *         the patched SDL uses); "qwerty" on desktop and for layouts without a table.
     */
    [[nodiscard]] static std::string_view keyboardLayout();

    /**
     * @brief Registers platform game-controller mapping overrides, after SDL_Init.
     *
     * The Switch SDL port maps the console pad positionally (Xbox layout: SDL A = the
     * bottom button, which Nintendo labels B), so `pad:a` bindings would land on the
     * wrong labels. The Switch implementation overrides the pad's mapping with a
     * label-true one; desktop is a no-op (SDL's database already matches labels there).
     */
    static void addControllerMappings();
};


#endif //PLATFORM_H
