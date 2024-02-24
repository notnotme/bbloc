#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../Cursor.h"

class VectorCursor : public Cursor {
public:

    VectorCursor();
    virtual ~VectorCursor();

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

    /// @brief Disallow copy
    VectorCursor(const VectorCursor& copy);
    /// @brief Disallow copy
    VectorCursor& operator=(const VectorCursor&);

    /// @brief The vector that hold all lines of this object
    std::vector<std::u16string> mLines;

};
