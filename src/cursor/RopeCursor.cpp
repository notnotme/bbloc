#include "RopeCursor.h"

#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <filesystem>

#define LINES_RESERVE 2048 // char reserved per lines at file read time

RopeCursor::RopeCursor() :
Cursor() {
    mLines.emplace_back((Line) { 0, 0 });
}

RopeCursor::~RopeCursor() {
}

void RopeCursor::load(const std::string path) {
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
        mLines.emplace_back((Line) { mRope.length(), line.length() });
        auto string = mConverter.from_bytes(line);
        mRope.append(string.data());
    }
    
    if (mLines.empty()) {
        mLines.emplace_back((Line) { 0, 0 });
    }

    mDataView = mRope.c_str();
    fileStream.close();
}

void RopeCursor::clear() {
    Cursor::clear();
    mRope.clear();
    mLines.resize(0);
}

size_t RopeCursor::size() const {
    return mLines.size();
}

const std::u16string_view RopeCursor::stringView(size_t line) const {
    return mDataView.substr(mLines[line].start, mLines[line].count);
}

bool RopeCursor::insert(char16_t character) {
    mRope.insert(mLines[mPosition.y].start + mPosition.x, character);
    ++mLines[mPosition.y].count;
    ++mPosition.x;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        ++line.start;
    });

    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });

    // We need to refresh the underlying whole string
    refreshView();
    return true;
}

bool RopeCursor::insert(std::u16string text) {
    mRope.insert(mLines[mPosition.y].start + mPosition.x, text.c_str());

    auto len = text.length();
    mLines[mPosition.y].count += len;
    mPosition.x += len;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        line.start += len;
    });
    
    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });

    refreshView();
    return true;
}

bool RopeCursor::insert(const char *utf8Text) {
    auto text = mConverter.from_bytes(utf8Text);
    mRope.insert(mLines[mPosition.y].start + mPosition.x, text.c_str());

    auto len = text.length();
    mLines[mPosition.y].count += len;
    mPosition.x += len;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        line.start += len;
    });
    
    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });


    refreshView();
    return true;
}

bool RopeCursor::remove() {
    auto removed = true;
    if (mPosition.x > 0) {
        // Case 1, the caret is in the middle of the line, we delete one characters
        mRope.erase(mLines[mPosition.y].start + mPosition.x - 1, 1);
        --mLines[mPosition.y].count;
        --mPosition.x;

        // Update the remaining lines
        std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
            --line.start;
        });

        // Only one line change, and caret move
        mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT });

        // We need to refresh the underlying whole string
        refreshView();
    } else {
        // Case 2, the caret is at the very begining, we delete the line
        if (mPosition.y > 0) {
            // - delete the line but keep the string
            auto reminder = mRope.substr(mLines[mPosition.y].start, mLines[mPosition.y].count);
            mLines.erase(mLines.begin() + mPosition.y);

            // - move the caret up at the end of the line
            --mPosition.y;
            mPosition.x = mLines[mPosition.y].count;
            mLines[mPosition.y].count += reminder.length();
            
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

bool RopeCursor::newLine() {
    if (mPosition.x == mLines[mPosition.y].count) {
        // Case 1, the cursor is at the very end of the line
        // we need to create a new Line struct
        mLines.insert(mLines.begin() + mPosition.y + 1, (Line) { mLines[mPosition.y].start + mLines[mPosition.y].count, 0 });
        ++mPosition.y;
        mPosition.x = 0;

        // Not need to trigger line change
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { CARET_MOVED, DOWN });
    } else {
        // Case 2, the cursor is in the middle of the string
        // - we just put a new line in mLines
        auto reminderStart = mLines[mPosition.y].start + mPosition.x;
        auto reminderLen = mLines[mPosition.y].count - mPosition.x;

        // we need to create a new Line struct
        mLines[mPosition.y].count -= reminderLen;
        mLines.insert(mLines.begin() + mPosition.y + 1, (Line) { reminderStart, reminderLen });

        // - move the cursor down at the begning of the line
        ++mPosition.y;
        mPosition.x = 0;

        // Fill events
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y - 1 });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT | DOWN });
    }

    return true;
};

void RopeCursor::refreshView() {
    mDataView = mRope.c_str();
}
