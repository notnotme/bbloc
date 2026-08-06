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
#ifndef GL_BACKEND_H
#define GL_BACKEND_H

#include <cstdint>

/**
 * @brief OpenGL context version required by the gl43 (bind-based) renderer backend.
 *
 * Each renderer source set ships its own GlBackend.h; CMake points the include path at the
 * selected set, so the window always requests the context version matching the compiled backend.
 */
class GlBackend final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    GlBackend() = delete;

    /** OpenGL context major version requested at window creation. */
    static constexpr int32_t CONTEXT_MAJOR_VERSION = 4;

    /** OpenGL context minor version requested at window creation. */
    static constexpr int32_t CONTEXT_MINOR_VERSION = 3;
};


#endif //GL_BACKEND_H
