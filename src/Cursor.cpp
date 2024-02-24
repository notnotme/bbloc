#include "Cursor.h"

#include <stdexcept>
#include <fstream>
#include <filesystem>

#define LINES_RESERVE 2048 // char reserved per lines

Cursor::Cursor() :
mPosition({ 0, 0 }),
mSelection({ false, false, { 0, 0 }, { 0, 0 } }) {
}

Cursor::~Cursor() {
}

void Cursor::load(const std::string path) {
    clear();

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(std::string("Not a regular file: ").append(path));
    }

    std::ifstream fileStream(path);
    if(!fileStream.is_open()) {
        throw std::runtime_error(std::string("Can't open ").append(path));
    }
   
    std::string line;
    line.reserve(LINES_RESERVE);

    while (std::getline(fileStream, line)) {
        auto string = mConverter.from_bytes(line);
        pushLine(string);
    }

    if (size() == 0) {
        // Never load a 0 lines Cursor
        pushLine(std::u16string());
    }
    
    fileStream.close();
}

void Cursor::save(const std::string path) {
    std::ofstream fileStream(path);
    if(!fileStream.is_open()) {
        throw std::runtime_error(std::string("Can't open ").append(path));
    }

    auto lineCount = size();
    for (size_t line = 0; line < lineCount; ++line) {
        auto str = stringView(line);
        fileStream << mConverter.to_bytes(str.begin(), str.end());

        if(line < lineCount - 1) {
            fileStream << "\n";
        }
    }
    
    fileStream.close();
}

void Cursor::clear() {
    mPosition = { 0, 0 };
    mSelection = { false, false, { 0, 0 }, { 0, 0 } };
    while (!mEventStack.empty()) {
        mEventStack.pop();
    }
}

glm::u32vec2 Cursor::position() const {
    return mPosition;
}

Cursor::Selection Cursor::selection() const {
    return mSelection;
}

void Cursor::position(uint32_t line, uint32_t character) {
    auto move = 0;

    if (line < 0) {
        line = 0;
    } else if (line > size() - 1) {
        line = size() - 1;
    }
    
    auto string = stringView(line);
    if (character < 0) {
        character = 0;
    } else if (character > string.length()) {
        character = string.length();
    }

    if (mPosition.x < character) {
        move |= LEFT;
    } else if (mPosition.x > character) {
        move |= RIGHT;
    }

    if (mPosition.y < line) {
        move |= UP;
    } else if (mPosition.y > line) {
        move |= DOWN;
    }

    if (move != 0) {
        mEventStack.emplace((Event) { CARET_MOVED, (uint64_t) move });
        mPosition.x = character;
        mPosition.y = line;
    }
}

bool Cursor::move(Direction direction) {
    auto line = stringView(mPosition.y);
    auto move = 0;

    switch (direction) {
    case UP:
        if (mPosition.y > 0) {
            auto lineAboveLine = stringView(mPosition.y - 1);
            if (mPosition.x > lineAboveLine.length()) {
                // The caret can't stay at the same X position, put it at the end of next line
                mPosition.x = lineAboveLine.length();

                // no need to find direction, it's to trigger border check with the caret
                // (both side are checked when CURSOR_MOVED is sent, RIGHT and LEFT, or UP and DOWN)
                move |= LEFT;
            }
            --mPosition.y;
            move |= UP;
        }
    break;
    case DOWN:
        if (mPosition.y < size()-1) {
            auto lineBelowLine = stringView(mPosition.y + 1);
            if (mPosition.x > lineBelowLine.length()) {
                // The caret can't stay at the same X position, put it at the end of next line
                mPosition.x = lineBelowLine.length();

                // no need to find the right direction, it's to trigger border check with the caret
                // (both side are checked when CURSOR_MOVED is sent, RIGHT and LEFT, or UP and DOWN)
                move |= LEFT;
            }
            ++mPosition.y;
            move |= DOWN;
        }
    break;
    case LEFT:
        if (mPosition.x == 0) {
            // At the very begining of a line
            if (mPosition.y > 0) {
                // Can go backward 
                auto lineAboveLine = stringView(mPosition.y - 1);
                mPosition.x = lineAboveLine.length();
                --mPosition.y;
                move |= RIGHT | UP;
            }
        } else {
            // Everywhere but in the begining
            --mPosition.x;
            move |= LEFT;
        }
    break;
    case RIGHT:
        if (mPosition.x == line.length()) {
            // At the very end of a line
            if (mPosition.y < size()-1) {
                // Can go further
                mPosition.x = 0;
                ++mPosition.y;

                move |= LEFT | DOWN;
            }
        } else {
            // Everywhere but in the end
            ++mPosition.x;

            move |= RIGHT;
        }
    break;
    }

    if (mSelection.edit) {
        mSelection.end = mPosition;
    } else if (mSelection.visible) {
        mSelection.visible = false;
    }

    auto moved = direction != 0;
    if (moved) {
        mEventStack.emplace((Event) { CARET_MOVED, (uint64_t) move });
    }

    return moved;
}

const Cursor::Event* Cursor::event() const {
    if (mEventStack.empty()) {
        return nullptr;
    } else {
        return &mEventStack.top();
    }
}

void Cursor::popEvent() {
    if (!mEventStack.empty()) {
        mEventStack.pop();
    }
}

bool Cursor::selectionVisible() const {
    return mSelection.visible;
}

void Cursor::enterSelection() {
    if (!mSelection.visible) {
        mSelection.visible = true;
    }
    if (!mSelection.edit) {
        mSelection.edit = true;
        mSelection.start = mPosition;
        mSelection.end = mPosition;
    }
}

void Cursor::exitSelection() {
    if (mSelection.edit) {
        mSelection.edit = false;
    }
}

void Cursor::hideSelection() {
    if (mSelection.visible) {
        mSelection.visible = false;
    }
}

void Cursor::toggleSelection() {
    mSelection.visible = !mSelection.visible;
}

void Cursor::eraseSelection() {
    mSelection.edit = false;
    mSelection.visible = false;
    mSelection.start = { 0, 0 };
    mSelection.end = { 0, 0 };
}

void Cursor::eof() {
    auto lastLine = stringView(size() - 1);
    if (mPosition.x < lastLine.length() || mPosition.y < size() - 1) {
        mPosition.x = lastLine.length();
        mPosition.y = size() - 1;
        mEventStack.emplace((Event) { CARET_MOVED, DOWN | RIGHT });
    }
}

void Cursor::bof() {
    if (mPosition.x > 0 || mPosition.y > 0) {
        mPosition.x = 0;
        mPosition.y = 0;
        mEventStack.emplace((Event) { CARET_MOVED, UP | LEFT });
    }
}

void Cursor::eol() {
    auto currentLine = stringView(mPosition.y);
    if (mPosition.x < currentLine.length()) {
        mPosition.x = currentLine.length();
        mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    }
}

void Cursor::bol() {
    if (mPosition.x > 0) {
        mPosition.x = 0;
        mEventStack.emplace((Event) { CARET_MOVED, LEFT });
    }
}
