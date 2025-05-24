#ifndef COLOR_H
#define COLOR_H


/**
 * @brief Represents an RGBA color.
 *
 * Each component is stored as an 8-bit unsigned byte ranging from 0 to 255.
 */
struct Color {
    uint8_t red;   ///< Red component of the color.
    uint8_t green; ///< Green component of the color.
    uint8_t blue;  ///< Blue component of the color.
    uint8_t alpha; ///< Alpha (transparency) component of the color.
};


#endif //COLOR_H
