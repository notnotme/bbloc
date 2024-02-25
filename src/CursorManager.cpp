#include "CursorManager.h"

#include <algorithm>
#include "cursor/VectorCursor.h" 
#include "cursor/StringCursor.h"
#include "cursor/RopeCursor.h"

CursorManager::CursorManager() :
mActiveCursorIndex(0) {
}

CursorManager::~CursorManager() {
}

bool CursorManager::open(const std::string path) {
    // Check for already opened Cursor
    auto size = mCursorList.size();
    for (size_t index = 0; index < size; ++index) {
        if (mCursorList[index]->path() == path) {
            mActiveCursorIndex = index;
            return true;
        }
    }

    // Try to open a new Cursor
    auto cursor = std::make_shared<RopeCursor>();
    try {
        cursor->load(path);
    } catch (const std::exception& e) {
        // TODO: Handle errors
        return false;
    }

    // Insert the new Cursor and make it active
    mCursorList.emplace_back(cursor);
    mActiveCursorIndex = mCursorList.size() - 1;
    return true;
}

void CursorManager::close() {
    if (mCursorList.empty()) {
        return;
    }

    if (mActiveCursorIndex > mCursorList.size() - 1) {
        return;
    }

    mCursorList.erase(mCursorList.begin() + mActiveCursorIndex);
    next(true);
}

std::shared_ptr<Cursor> CursorManager::get() const {
    if (mCursorList.empty()) {
        return nullptr;
    }
    
    if (mActiveCursorIndex > mCursorList.size() - 1) {
        return nullptr;
    }

    return mCursorList[mActiveCursorIndex];
}

size_t CursorManager::count() const {
    return mCursorList.size();
}

size_t CursorManager::index() const {
    return mActiveCursorIndex;
}

void CursorManager::next(bool loop) {
    if (mCursorList.empty() || mCursorList.size() == 1) {
        return;
    }

    ++mActiveCursorIndex;
    if (mActiveCursorIndex > mCursorList.size() - 1) {
        mActiveCursorIndex = 0;
    }
}

void CursorManager::previous(bool loop) {
    if (mCursorList.empty() || mCursorList.size() == 1) {
        return;
    }

    --mActiveCursorIndex;
    if (mActiveCursorIndex > mCursorList.size()) {
        mActiveCursorIndex = mCursorList.size() - 1;
    }
}
