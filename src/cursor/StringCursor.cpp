#include "StringCursor.h"

#include <algorithm>

#define VECTOR_LINES_RESERVE 8192 // u16string reserved by cursor
#define LINES_RESERVE 2048 // char reserved per lines 
#define BUFFER_RESERVE VECTOR_LINES_RESERVE * LINES_RESERVE // lots of char

StringCursor::StringCursor() :
Cursor() {
    mBuffer.reserve(BUFFER_RESERVE);
    mLines.reserve(VECTOR_LINES_RESERVE);
    mLines.emplace_back((Line) { 0, 0 });
}

StringCursor::~StringCursor() {
}

void StringCursor::clear() {
    Cursor::clear();
    mBuffer.resize(0);
    mLines.resize(0);
}

size_t StringCursor::size() const {
    return mLines.size();
}

const std::u16string_view StringCursor::stringView(size_t line) const {
    return static_cast<std::u16string_view>(mBuffer)
        .substr(mLines[line].start, mLines[line].count);
}

bool StringCursor::insert(char16_t character) {
    mBuffer.insert(mBuffer.begin() + mLines[mPosition.y].start + mPosition.x, character);
    ++mLines[mPosition.y].count;
    ++mPosition.x;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        ++line.start;
    });
    
    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool StringCursor::insert(std::u16string text) {
    mBuffer.insert(mLines[mPosition.y].start + mPosition.x, text);

    auto len = text.length();
    mLines[mPosition.y].count += len;
    mPosition.x += len;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        line.start += len;
    });
    
    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool StringCursor::insert(const char *utf8Text) {
    auto text = mConverter.from_bytes(utf8Text);
    mBuffer.insert(mLines[mPosition.y].start + mPosition.x, text);

    auto len = text.length();
    mLines[mPosition.y].count += len;
    mPosition.x += len;

    // Update the remaining lines
    std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
        line.start += len;
    });
    
    mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
    mEventStack.emplace((Event) { CARET_MOVED, RIGHT });
    return true;
}

bool StringCursor::remove(bool forward) {
    auto removed = true;
    if (!forward) {
        if (mPosition.x > 0) {
            // Case 1, the caret is after the first character, we delete one characters
            mBuffer.erase(mLines[mPosition.y].start + mPosition.x - 1, 1);
            --mLines[mPosition.y].count;
            --mPosition.x;

            // Update the remaining lines
            std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
                --line.start;
            });

            // Only one line change
            mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
            mEventStack.emplace((Event) { CARET_MOVED, LEFT });
        } else {
            // Case 2, the caret is at the very begining, we delete the line
            if (mPosition.y > 0) {
                // - delete the line but keep the string
                auto reminder = mLines[mPosition.y].count;
                mLines.erase(mLines.begin() + mPosition.y);

                // - move the caret up at the end of the line
                --mPosition.y;
                mPosition.x = mLines[mPosition.y].count;
                mLines[mPosition.y].count += reminder;

                // One line changed, we deleted the other
                mEventStack.emplace((Event) { LINE_DELETED, mPosition.y + 1 });
                mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
                mEventStack.emplace((Event) { CARET_MOVED, RIGHT | UP });
            } else {
                removed = false;
            }
        }
    } else {
        // Case 1, the line is not empty and the caret is not at the end of it, we delete one characters
        if (mLines[mPosition.y].count > 0 && mPosition.x < mLines[mPosition.y].count) {
            mBuffer.erase(mLines[mPosition.y].start + mPosition.x, 1);
            --mLines[mPosition.y].count;

            // Update the remaining lines
            std::for_each(mLines.begin() + mPosition.y + 1, mLines.end(), [&](Line& line) {
                --line.start;
            });

            // Only one line change
            mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
            mEventStack.emplace((Event) { CARET_MOVED, LEFT });
        } else {
            // Case 2, the caret is at the end of the line
             if (mPosition.y < mLines.size() - 1) {
                // - delete the line but keep the string
                auto reminder = mLines[mPosition.y + 1].count;
                mLines.erase(mLines.begin() + mPosition.y + 1);
                mLines[mPosition.y].count += reminder;

                // One line changed, we deleted the other
                mEventStack.emplace((Event) { LINE_DELETED, mPosition.y + 1 });
                mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y });
                mEventStack.emplace((Event) { CARET_MOVED, 0 });
            } else {
                removed = false;
            }
        }
    }


    return removed;
}

bool StringCursor::newLine() {
    if (mPosition.x == mLines[mPosition.y].count) {
        // Case 1, the caret is at the very end of the line
        // we need to create a new Line struct
        mLines.insert(mLines.begin() + mPosition.y + 1, (Line) { mLines[mPosition.y].start + mLines[mPosition.y].count, 0 });
        ++mPosition.y;
        mPosition.x = 0;

        // Not need to trigger line change
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { CARET_MOVED, DOWN });
    } else {
        // Case 2, the caret is in the middle of the string
        // - we just put a new line in mLines
        auto reminderStart = mLines[mPosition.y].start + mPosition.x;
        auto reminderLen = mLines[mPosition.y].count - mPosition.x;

        // we need to create a new Line struct
        mLines[mPosition.y].count -= reminderLen;
        mLines.insert(mLines.begin() + mPosition.y + 1, (Line) { reminderStart, reminderLen });

        // - move the caret down at the begning of the line
        ++mPosition.y;
        mPosition.x = 0;

        // Fill events
        mEventStack.emplace((Event) { LINE_CREATED, mPosition.y });
        mEventStack.emplace((Event) { LINE_CHANGED, mPosition.y - 1 });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT | DOWN });
    }

    return true;
};


void StringCursor::eraseSelection() {
    if (mSelection.empty()) {
        return;
    }

    auto totalCount = 0;
    if (mSelection.start.y == mSelection.end.y) {
        // Start / end on the same line
        if (mSelection.end.x < mSelection.start.x) {
            // Check if we need to invert X axis
            std::swap(mSelection.start, mSelection.end);
        }

        totalCount = mSelection.end.x - mSelection.start.x;
        mLines[mPosition.y].count -= totalCount;
        mBuffer.erase(mLines[mSelection.start.y].start + mSelection.start.x, totalCount);
        mEventStack.emplace((Event) { LINE_CHANGED, mSelection.start.y });
        mEventStack.emplace((Event) { CARET_MOVED, LEFT });
    } else {
        // Start / end on multiple line
        if (mSelection.end.y < mSelection.start.y) {
            // Check if we need to invert Y axis
            std::swap(mSelection.start, mSelection.end);
        }

        for (auto line = mSelection.start.y; line <= mSelection.end.y; ++line) {
            auto& string = mLines[line];
            if (line == mSelection.start.y) {
                auto count = string.count - mSelection.start.x;
                string.count -= count;
                totalCount += count;
                mEventStack.emplace((Event) { LINE_CHANGED, line });
            } else if (line == mSelection.end.y) {
                string.count -= mSelection.end.x;
                totalCount += mSelection.end.x;
                mEventStack.emplace((Event) { LINE_DELETED, line });
            } else {
                totalCount += string.count;
                mEventStack.emplace((Event) { LINE_DELETED, line });
            }
        }

        mLines[mSelection.start.y].count += mLines[mSelection.end.y].count;
        mBuffer.erase(mLines[mSelection.start.y].start + mSelection.start.x, totalCount);
        mLines.erase(mLines.begin() + mSelection.start.y + 1, mLines.begin() + mSelection.end.y + 1);
        mEventStack.emplace((Event) { CARET_MOVED, LEFT | UP });
    }

    // Update the remaining lines
    std::for_each(mLines.begin() + mSelection.start.y + 1, mLines.end(), [&](Line& line) {
        line.start -= totalCount;
    });

    mPosition = mSelection.start;

    // Reset internal struct
    Cursor::eraseSelection();
}

void StringCursor::pushLine(const std::u16string line) {
    mLines.emplace_back((Line) { mBuffer.length(), line.length() });
    mBuffer.append(line);
}
