#include "RopeCursor.h"

RopeCursor::RopeCursor() :
Cursor() {
    mLines.emplace_back((Line) { 0, 0 });
}

RopeCursor::~RopeCursor() {
}

void RopeCursor::load(const std::string path) {
    Cursor::load(path);
    refreshView();
};

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

void RopeCursor::eraseSelection() {
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
        mLines[mSelection.start.y].count -= totalCount;
        mRope.erase(mLines[mSelection.start.y].start + mSelection.start.x, totalCount);
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
        mRope.erase(mLines[mSelection.start.y].start + mSelection.start.x, totalCount);
        mLines.erase(mLines.begin() + mSelection.start.y + 1, mLines.begin() + mSelection.end.y + 1);
        mEventStack.emplace((Event) { CARET_MOVED, LEFT | UP });
    }

    // Update the remaining lines
    std::for_each(mLines.begin() + mSelection.start.y + 1, mLines.end(), [&](Line& line) {
        line.start -= totalCount;
    });

    mPosition = mSelection.start;

    // Need to refresh the underlying whole string and reset the selection struct
    refreshView();

    // Reset internal struct
    Cursor::eraseSelection();
}

void RopeCursor::refreshView() {
    mDataView = mRope.c_str();
}

void RopeCursor::pushLine(const std::u16string line) {
    mLines.emplace_back((Line) { mRope.length(), line.length() });
    mRope.append(line.c_str());
}
