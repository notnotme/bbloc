#ifndef QUAD_VERTEX_H
#define QUAD_VERTEX_H

#include <cstdint>


/**
 * @brief Represents a single vertex used for rendering a textured quad.
 *
 * This structure is used to describe the geometry and visual appearance of a rectangle
 * to be drawn on screen using a texture atlas.
 */
struct QuadVertex final {
    int16_t translation_x;   /**< X translation (in pixels) from the origin. */
    int16_t translation_y;   /**< Y translation (in pixels) from the origin. */
    uint16_t width;          /**< Width of the quad in pixels. */
    uint16_t height;         /**< Height of the quad in pixels. */
    uint8_t texture_s;       /**< Texture coordinate S (left) */
    uint8_t texture_t;       /**< Texture coordinate T (top). */
    uint8_t texture_p;       /**< Texture coordinate P (right). */
    uint8_t texture_q;       /**< Texture coordinate Q (bottom). */
    uint8_t tint_r;          /**< Tint color red component. */
    uint8_t tint_g;          /**< Tint color green component. */
    uint8_t tint_b;          /**< Tint color blue component. */
    uint8_t tint_a;          /**< Tint color alpha component. */

    uint8_t texture_layer;   /**< Index of the texture layer in the atlas. */
};


#endif //QUAD_VERTEX_H
