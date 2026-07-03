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
#ifndef ATLAS_ARRAY_H
#define ATLAS_ARRAY_H

#include <cstdint>
#include <unordered_map>

#include "AtlasEntry.h"


/**
 * @brief Manages a virtual texture atlas for storing character glyphs or sprites.
 *
 * This class handles glyph placement, naive packing, and lookup inside a multi-layered
 * texture atlas.
 */
class AtlasArray final {
private:
    /** Maximum height of the current row (used for packing). */
    uint8_t m_max_row_height;

    /** Index of the current layer used for character insertion. */
    uint8_t m_character_layer;

    /** Current horizontal insertion point in the character layer. */
    int32_t m_next_character_x;

    /** Current vertical insertion point in the character layer. */
    int32_t m_next_character_y;

    /** Map storing character entries by codepoint. */
    std::unordered_map<char16_t, AtlasEntry> m_characters;

public:
    /** @brief Deleted copy constructor. */
    AtlasArray(const AtlasArray &) = delete;

    /** @brief Deleted copy assignment operator. */
    AtlasArray &operator=(const AtlasArray &) = delete;

    /** @brief Constructs an uninitialized AtlasArray. */
    explicit AtlasArray();

    /** @brief Initializes the atlas array. */
    void create();

    /** @brief Destroys the atlas and clears all stored characters. */
    void destroy();

    /**
     * @brief Inserts a new character into the atlas.
     *
     * @param character The Unicode codepoint to insert.
     * @param width Width of the glyph in pixels.
     * @param height Height of the glyph in pixels.
     * @param bearingX Horizontal bearing (offset from origin).
     * @param bearingY Vertical bearing (offset from baseline).
     * @return Reference to the inserted AtlasEntry.
     */
    [[nodiscard]] const AtlasEntry &insert(char16_t character, uint32_t width, uint32_t height, int32_t bearingX, int32_t bearingY);

    /**
     * @brief Retrieves a character entry from the atlas.
     *
     * @param character The Unicode codepoint.
     * @return Pointer to the corresponding AtlasEntry, or nullptr if not found.
     */
    [[nodiscard]] const AtlasEntry *get(char16_t character) const;

    /** @brief Gets the current layer index used for character insertion. */
    [[nodiscard]] uint8_t getCurrentLayer() const;

    /** @brief Clears all character entries and resets character layers. */
    void clearCharacters();
};


#endif //ATLAS_ARRAY_H
