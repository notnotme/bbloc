#ifndef HIGH_LIGHT_ID_H
#define HIGH_LIGHT_ID_H


/**
 * @brief Identifies the syntax highlighting mode.
 *
 * This enum is used to describe the highlight for a given mode.
 */
enum class HighLightId {
    None, ///< No highlighting applied.
    Cpp,  ///< C++ syntax highlighting.
    Json  ///< JSON syntax highlighting.
};

#endif //HIGH_LIGHT_ID_H
