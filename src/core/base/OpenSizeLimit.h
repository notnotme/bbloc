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
#ifndef OPEN_SIZE_LIMIT_H
#define OPEN_SIZE_LIMIT_H

#include <cstdint>


/**
 * @brief Tells whether a file is large enough for the open command to ask before loading it.
 *
 * The whole file is held in memory at once, so opening past the limit deserves a confirmation.
 * A limit of zero (or less, which the CVar callback clamps away) disables the guard entirely.
 * The comparison is strict: a file of exactly the limit still opens silently.
 *
 * @param sizeBytes The file size in bytes, as reported by std::filesystem::file_size.
 * @param limitMb The open_size_limit CVar value, in megabytes (1024 * 1024 bytes).
 * @return true when the size exceeds the limit and a confirmation should be asked.
 */
[[nodiscard]] inline bool exceedsOpenSizeLimit(const uintmax_t sizeBytes, const int32_t limitMb) {
    return limitMb > 0 && sizeBytes > static_cast<uintmax_t>(limitMb) * 1024 * 1024;
}


#endif //OPEN_SIZE_LIMIT_H
