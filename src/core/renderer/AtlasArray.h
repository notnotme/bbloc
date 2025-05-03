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
    /** Width and height of the atlas texture (square). */
    uint8_t m_texture_size;

    /** Number of layers in the atlas texture. */
    uint8_t m_layers;

    /** Current horizontal insertion point in the character layer. */
    int32_t m_next_character_x;

    /** Current vertical insertion point in the character layer. */
    int32_t m_next_character_y;

    /** Maximum height of the current row (used for packing). */
    int32_t m_max_row_height;

    /** Index of the current layer used for character insertion. */
    int32_t m_character_layer;

    /** Map storing character entries by codepoint. */
    std::unordered_map<char16_t, AtlasEntry> m_characters;

public:
    /** @brief Deleted copy constructor. */
    AtlasArray(const AtlasArray &) = delete;

    /** @brief Deleted copy assignment operator. */
    AtlasArray &operator=(const AtlasArray &) = delete;

    /** @brief Constructs an uninitialized AtlasArray. */
    explicit AtlasArray();

    /**
     * @brief Initializes the atlas with the given size and number of layers.
     * @param size Width and height of the square texture (in pixels).
     * @param layers Number of texture layers.
     */
    void create(uint8_t size, uint8_t layers);

    /** @brief Destroys the atlas and clears all stored characters. */
    void destroy();

    /**
     * @brief Inserts a new character into the atlas.
     * @param character The Unicode codepoint to insert.
     * @param width Width of the glyph in pixels.
     * @param height Height of the glyph in pixels.
     * @param bearingX Horizontal bearing (offset from origin).
     * @param bearingY Vertical bearing (offset from baseline).
     * @return Reference to the inserted AtlasEntry.
     * @throws std::runtime_error if the character already exists or there's no space left.
     */
    [[nodiscard]] const AtlasEntry& insert(char16_t character, uint8_t width, uint8_t height, int32_t bearingX, int32_t bearingY);

    /**
     * @brief Retrieves a character entry from the atlas.
     * @param character The Unicode codepoint.
     * @return Pointer to the corresponding AtlasEntry, or nullptr if not found.
     */
    [[nodiscard]] const AtlasEntry* get(char16_t character) const;

    /** @brief Gets the current layer index used for character insertion. */
    [[nodiscard]] uint8_t getCurrentLayer() const;

    /** @brief Clears all character entries and resets character layers. */
    void clearCharacters();
};


#endif //ATLAS_ARRAY_H
