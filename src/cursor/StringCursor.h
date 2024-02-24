#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../Cursor.h"

class StringCursor : public Cursor {
public:

    StringCursor();
    virtual ~StringCursor();

    void clear() override;
    size_t size() const override;
    const std::u16string_view stringView(size_t line) const override;
    bool insert(char16_t character) override;
    bool insert(std::u16string text) override;
    bool insert(const char *utf8Text) override;
    bool remove(bool forward) override;
    bool newLine() override;
    void eraseSelection() override;

protected:

    void pushLine(const std::u16string line) override;

private:

    /// @brief Allow debug to inspect
    friend class Debug;
    
    /// @brief Hold lines data
    struct Line {
        /// @brief Start index of the line in mBuffer
        size_t start;
        /// @brief length of the line in characters
        size_t count;
    };

    /// @brief Disallow copy
    StringCursor(const StringCursor& copy);
    /// @brief Disallow copy
    StringCursor& operator=(const StringCursor&);

    /// @brief The vector that hold all lines of this object
    std::u16string mBuffer;

    /// @brief A vector containing the start/end of each line of the buffer
    std::vector<Line> mLines;

};
