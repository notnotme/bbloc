#pragma once

#include <string>
#include <vector>
#include <ext/rope>
#include <glm/glm.hpp>
#include "../Cursor.h"

// I don't know if I make good use of the rope here or if it have real advantage over the StringCursor, because
// after each internal string modification, we need to call c_str() on the rope, which will recreate the entire string
// and change it's pointer location.
// This is needed because Cursor need to return u16string_view, and  it seem that views are no longer valid after rope modification :/
class RopeCursor : public Cursor {
public:

    RopeCursor();
    virtual ~RopeCursor();

    void load(const std::string path) override;
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
        /// @brief Start index of the line in mRope
        size_t start;
        /// @brief length of the line in characters
        size_t count;
    };

    /// @brief Disallow copy
    RopeCursor(const RopeCursor& copy);
    /// @brief Disallow copy
    RopeCursor& operator=(const RopeCursor&);

    /// @brief The rope that hold all lines of this object
    __gnu_cxx::rope<char16_t> mRope;

    /// @brief A vector containing the start/end of each line of the buffer
    std::vector<Line> mLines;

    /// @brief String view of the current data
    std::u16string_view mDataView;

    /// @brief This need to be called if the underlying data representating the text change
    void refreshView();

};
