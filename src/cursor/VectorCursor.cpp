#include "VectorCursor.h"

#include <stdexcept>
#include <fstream>
#include <filesystem>

#define VECTOR_LINES_RESERVE 8192 // u16string reserved by cursor
#define LINES_RESERVE 2048 // char reserved per lines

VectorCursor::VectorCursor() :
Cursor() {
    mLines.reserve(VECTOR_LINES_RESERVE);
    mLines.emplace_back(std::u16string());
}

VectorCursor::~VectorCursor() {
}

void VectorCursor::load(const std::string path) {
    clear();

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(std::string("Not a regular file: ").append(path));
    }

    std::ifstream fileStream(path, std::ios::in | std::ios::binary);
    if(!fileStream.is_open()) {
        throw std::runtime_error(std::string("Can't open ").append(path));
    }
   
    std::string line;
    line.reserve(LINES_RESERVE);

    while (std::getline(fileStream, line)) {
        auto string = mConverter.from_bytes(line);
        mLines.emplace_back(string);
    }

    if (mLines.empty()) {
        mLines.emplace_back(std::u16string());
    }
    
    fileStream.close();
}

void VectorCursor::clear() {
    Cursor::clear();
    mLines.resize(0);
}

size_t VectorCursor::size() const {
    return mLines.size();
}

const std::u16string_view VectorCursor::stringView(size_t line) const {
    return mLines[line];
}

bool VectorCursor::insert(char16_t character) {
    mLines[mPosition.y].insert(mLines[mPosition.y].begin() + mPosition.x, character);
    ++mPosition.x;

    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool VectorCursor::insert(std::u16string text) {
    mLines[mPosition.y].insert(mPosition.x, text);
    mPosition.x += text.length();

    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool VectorCursor::insert(const char *utf8Text) {
    auto text = mConverter.from_bytes(utf8Text);
    mLines[mPosition.y].insert(mPosition.x, text);
    mPosition.x += text.length();

    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool VectorCursor::remove() {
    auto removed = true;
    if (!mLines[mPosition.y].empty() && mPosition.x > 0) {
        // Case 1, the caret is in the middle of the line, we delete one characters
        mLines[mPosition.y].erase(mPosition.x-1, 1);
        --mPosition.x;
        // Only one line change
        mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT });
    } else {
        // Case 2, the caret is at the very begining
        if (mPosition.y > 0) {
            // - we remove that line but keep the reminder
            auto reminder = mLines[mPosition.y].substr(mPosition.x);
            mLines.erase(mLines.begin() + mPosition.y);
         
            // - move the caret up at the end of the line
            --mPosition.y;
            mPosition.x = mLines[mPosition.y].length();
      
            // - past the reminder of the deleted line (what was after the caret position)
            mLines[mPosition.y].append(reminder);

            // One line changed, we deleted the other
            mEventStack.emplace((Event) { LINE_DELETED, mPosition.y + 1 });
            mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
            mEventStack.emplace((Event) { CARET_MOVED, RIGHT | UP });
        } else {
            removed = false;
        }
    }

    return removed;
}

bool VectorCursor::newLine() {
    if (mPosition.x == mLines[mPosition.y].length()) {
        // Case 1, the caret is at the very end of the line, we just add one line below
        mLines.insert(mLines.begin() + mPosition.y + 1, std::u16string());
        ++mPosition.y;
        mPosition.x = 0;
        // Not need to trigger line change
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { CARET_MOVED, DOWN });
    } else {
        // Case 2, the caret is in the middle of the string
        // - we cut the line after the caret position and keep the reminder
        auto reminder = mLines[mPosition.y].substr(mPosition.x);
        mLines[mPosition.y].erase(mPosition.x);
        // - move the caret down at the begning of the line
        ++mPosition.y;
        mPosition.x = 0;
      
        // - Inser the reminder of the old line (what was after the caret position) as new line
        mLines.insert(mLines.begin() + mPosition.y, reminder);

        // Fill events
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y - 1 });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT | DOWN });
    }

    return true;
};
