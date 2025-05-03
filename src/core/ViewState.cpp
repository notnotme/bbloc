#include "ViewState.h"


ViewState::ViewState()
    : m_position_x(0),
      m_position_y(0),
      m_width(0),
      m_height(0) {}

int16_t ViewState::getPositionX() const {
    return m_position_x;
}

int16_t ViewState::getPositionY() const {
    return m_position_y;
}

uint16_t ViewState::getWidth() const {
    return m_width;
}

uint16_t ViewState::getHeight() const {
    return m_height;
}

void ViewState::setPosition(const int16_t x, const int16_t y) {
    m_position_x = x;
    m_position_y = y;
}

void ViewState::setSize(const uint16_t width, const uint16_t height) {
    m_width = width;
    m_height = height;
}