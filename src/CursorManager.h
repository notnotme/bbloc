#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Cursor.h"

class CursorManager {
public:

    CursorManager();

    virtual ~CursorManager();

    /// @brief Get or create a new Cursor and make it active
    /// @param path The path to open with the Cursor
    /// @return True if open success, false otherwise
    bool open(const std::string path);

    /// @brief Close the current active Cursor
    void close();

    /// @return The current active Cursor
    std::shared_ptr<Cursor> get() const;

    /// @brief return the number of cursor in the manager
    size_t count() const;

    /// @brief Switch to next cursor, or does nothing
    /// @param loop True if the function must loop in the cursor list
    void next(bool loop);

    /// @brief Switch to previous cursor, or does nothing
    /// @param loop True if the function must loop in the cursor list
    void previous(bool loop);

private:

    /// @brief Disallow copy
    CursorManager(const CursorManager& copy);

    /// @brief Disallow copy
    CursorManager& operator=(const CursorManager&);

    /// @brief The vector contaning all opened Cursor
    std::vector<std::shared_ptr<Cursor>> mCursorList;

    /// @brief The index of the current active Cursor
    size_t mActiveCursorIndex;

};
