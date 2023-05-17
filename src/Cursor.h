#pragma once

#include <vector>
#include <string>
#include <stack>
#include <codecvt>
#include <locale>
#include <glm/glm.hpp>

class Cursor {
public:
    /// @brief Directions of the caret
    enum Direction : uint8_t {
        UP      = 1,
        DOWN    = 2,
        LEFT    = 4,
        RIGHT   = 8
    };

    /// @brief Event types
    enum EventType : uint8_t {
        /// @brief The caret has moved (you get the direction not the coordinate)
        CARET_MOVED         = 1,
        /// @brief A line changed
        LINE_CHANGED        = 2,
        /// @brief A line was created
        LINE_CREATED        = 4,
        /// @brief A line was deleted
        LINE_DELETED        = 8
    };

    /// @brief When the cursor internal state an Event is generated
    struct Event {
        /// @brief The type of event that happened
        EventType   type;
        /// @brief Depending what event type, the data can represent different things (Direction, line number..)
        uint64_t    data;
    };

    /// @brief Hold the current caret selection
    struct Selection {
        /// @brief Start of the selection line/character
        glm::u32vec2 start;
        /// @brief End of the selection line/character
        glm::u32vec2 end;
        /// @brief Tell is this selection actually contains something in it
        /// @return true if the selection is empty, false otherwise
        bool empty() { return start == end; }
        /// @brief Get the number of line that the selection contains
        /// @return The number of lines in the selection
        uint32_t lineCount() { return (end.y - start.y) + 1; }
    };

    Cursor();
    virtual ~Cursor();

    /// @brief Load a new file into this cursor, throw if cannot load
    /// @param path The path of the file to load
    virtual void load(const std::string path);

    /// @brief Save the current cursor, throw if cannot save
    /// @param path The path of the file to save
    virtual void save(const std::string path);

    /// @brief Clear the current cursor from all its lines and reset caret position
    virtual void clear();

    /// @brief Return the number of lines this object contains
    /// @return The number of lines
    virtual size_t size() const = 0;

    /// @brief Get a string_view of a line inside the Cursor
    /// @param line The line number
    /// @return u16string_view of the requested line, throw if out of bounds
    virtual const std::u16string_view stringView(size_t line) const = 0;

    /// @brief Return the position of the caret
    /// @return The position of the caret, not always inside the content of mLines (can be at an edge of a line)
    glm::u32vec2 position() const;

    /// @brief Set the position of the caret (can be at an edge of a line (length))
    void position(uint32_t line, uint32_t character);

    /// @brief Move the caret position to the end of the file
    void eof();

     /// @brief Move the caret position to the begining of the file
   void bof();

    /// @brief Move the caret position to the end of the current line
    void eol();

    /// @brief Move the caret position to the begining of the line
    void bol();

    /// @brief Try to move the caret in the specified direction, by plus or minus one character, or plus or minus one line
    /// @param direction The direction to move
    /// @return true if the caret has moved, false otherwise
    bool move(Direction direction);

    /// @brief Get the caret selection
    /// @return The selection struct with start/end information
    Selection selection() const;

    /// @brief Get the last event from the cursor, if any
    /// @return The event that is on the top of the stack of event, nullptr if no event is in the stack
    const Event* event() const;

    /// @brief Remove the top event from the stack of event of this cursor
    void popEvent();

    /// @brief Insert a character at the caret position
    /// @param character The character to insert
    /// @return true if the insertion succeed, false otherwise
    virtual bool insert(char16_t character) = 0;

    /// @brief Insert a string at the caret position
    /// @param character The string to insert
    /// @return true if the insertion succeed, false otherwise
    virtual bool insert(std::u16string text) = 0;

    /// @brief Insert a utf8 c string at the caret position
    /// @param character The utf8 c string to insert
    /// @return true if the insertion succeed, false otherwise
    virtual bool insert(const char *utf8Text) = 0;

    /// @brief Remove a character at the caret position, delete the line if there is no more characters
    /// @return true if the remove succeed, false otherwise
    virtual bool remove() = 0;

    /// @brief Create a new line at the caret position
    /// @return true if the creation succeed, false otherwise
    virtual bool newLine() = 0;

protected:
    
    /// @brief Allow debug to inspect
    friend class Debug;

    /// @brief The current caret position line/character
    glm::u32vec2 mPosition;

    /// @brief The current carret selection
    Selection mSelection;

    /// @brief The stack of last event of this cursor, must be pulled using popEvent() on a regular basis
    std::stack<Event> mEventStack;

    /// @brief A converter from utf8 text to utf16
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> mConverter;

    /// @brief Append a line in the internal storage of the Cursor
    /// @param line The line to append into the Curso
    virtual void pushLine(const std::u16string line) = 0;

private:

    /// @brief Disallow copy
    Cursor(const Cursor& copy);
    /// @brief Disallow copy
    Cursor& operator=(const Cursor&);

};
