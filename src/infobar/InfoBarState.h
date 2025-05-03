#ifndef INFO_BAR_STATE_H
#define INFO_BAR_STATE_H

#include "../core/ViewState.h"


/**
 * @brief Stores layout and geometry information for the InfoBar view.
 *
 * This state class tracks position and size values, enabling the InfoBar
 * to adapt to window changes and maintain layout consistency.
 */
class InfoBarState final : public ViewState {
public:
    /** @brief Deleted copy constructor. */
    InfoBarState(const InfoBarState &) = delete;

    /** @brief Deleted copy assignment operator. */
    InfoBarState &operator=(const InfoBarState &) = delete;

    /** @brief Constructs an InfoBarState with default values. */
    explicit InfoBarState() = default;
};


#endif //INFO_BAR_STATE_H
