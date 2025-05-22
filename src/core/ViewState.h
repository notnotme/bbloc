#ifndef VIEW_STATE_H
#define VIEW_STATE_H

#include <cstdint>


/**
 * @brief Abstract base class representing a UI view state.
 *
 * This class is designed to be inherited by concrete view state implementations.
 */
class ViewState {
protected:
    /** X offset of the InfoBar within the window (in pixels). */
    int16_t m_position_x;

    /** Y offset of the InfoBar within the window (in pixels). */
    int16_t m_position_y;

    /** Width of the InfoBar (in pixels). */
    uint16_t m_width;

    /** Height of the InfoBar (in pixels). */
    uint16_t m_height;

public:
    /** @brief Deleted copy constructor. */
    ViewState(const ViewState &) = delete;

    /** @brief Deleted copy assignment operator. */
    ViewState &operator=(const ViewState &) = delete;

    /** @brief Create a ViewState with default values. */
    explicit ViewState();

    /** @brief For inheritance */
    virtual ~ViewState() = default;

    /** @brief Returns the X position of the view. */
    [[nodiscard]] int16_t getPositionX() const;

    /** @brief Returns the Y position of the view. */
    [[nodiscard]] int16_t getPositionY() const;

    /** @brief Returns the width of the view. */
    [[nodiscard]] uint16_t getWidth() const;

    /** @brief Returns the height of the view. */
    [[nodiscard]] uint16_t getHeight() const;

    /**
     * @brief Sets the position of the info bar view.
     * @param x New X position.
     * @param y New Y position.
     */
    void setPosition(int16_t x, int16_t y);

    /**
     * @brief Sets the size of the info bar view.
     * @param width New width.
     * @param height New height.
     */
    void setSize(uint16_t width, uint16_t height);
};


#endif //VIEW_STATE_H
