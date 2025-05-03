#ifndef DIMENSION_ID_H
#define DIMENSION_ID_H


/**
 * @brief Enumeration of dimension identifiers used for layout and spacing.
 *
 * These identifiers represent configurable values (in pixels or units)
 * for padding, indicators, borders, and logical sizes like tab width or page scrolling.
 */
enum class DimensionId {
    PaddingWidth,   ///< Padding applied to the sides of content, in pixels.
    IndicatorWidth, ///< Width of the cursor or indicator, in pixels.
    BorderSize,     ///< Thickness of borders between UI components, in pixels.
    TabToSpace,     ///< Number of spaces to display per tab character.
    PageUpDown      ///< Number of lines to scroll on PageUp/PageDown events.
};


#endif //DIMENSION_ID_H
