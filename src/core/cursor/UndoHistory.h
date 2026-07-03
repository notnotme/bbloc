#ifndef UNDO_HISTORY_H
#define UNDO_HISTORY_H

#include <cstdint>
#include <deque>
#include <optional>
#include <string>


/**
 * @brief Stores full-buffer snapshots used to undo and redo text edits.
 *
 * Snapshots are coalesced through a single boundary flag: a new snapshot is
 * pushed only when the history is at a boundary. Both stacks are capped at
 * MAX_HISTORY_DEPTH entries, dropping the oldest snapshot when full.
 */
class UndoHistory final {
public:
    /** @brief Represents the full state of a buffer at a given point in time. */
    struct Snapshot final {
        std::u16string text; ///< Full buffer content.
        uint32_t line;       ///< Cursor line at capture time.
        uint32_t column;     ///< Cursor column at capture time.
    };

private:
    /** Maximum number of snapshots kept in each stack. */
    static constexpr auto MAX_HISTORY_DEPTH = 64u;

    /** Snapshots available for undo. */
    std::deque<Snapshot> m_undo_stack;

    /** Snapshots available for redo. */
    std::deque<Snapshot> m_redo_stack;

    /** Flag indicating that the next edit must push a new snapshot. */
    bool m_at_boundary;

public:
    /** @brief Deleted copy constructor. */
    UndoHistory(const UndoHistory &) = delete;

    /** @brief Deleted copy assignment operator. */
    UndoHistory &operator=(const UndoHistory &) = delete;

    /** @brief Constructs an empty history, starting at a boundary. */
    explicit UndoHistory();

    /** @brief Marks a boundary so that the next edit pushes a new snapshot. */
    void markBoundary();

    /** @brief Returns true when the next edit must push a new snapshot. */
    [[nodiscard]] bool isAtBoundary() const;

    /**
     * @brief Pushes a snapshot onto the undo stack.
     *
     * Clears the redo stack, drops the oldest snapshot past MAX_HISTORY_DEPTH,
     * and clears the boundary flag.
     *
     * @param snapshot The snapshot to store.
     */
    void push(Snapshot snapshot);

    /**
     * @brief Pops the most recent snapshot from the undo stack.
     *
     * The current state is moved onto the redo stack.
     *
     * @param current The state of the buffer before undoing.
     * @return The snapshot to restore, or std::nullopt if the undo stack is empty.
     */
    [[nodiscard]] std::optional<Snapshot> undo(Snapshot current);

    /**
     * @brief Pops the most recent snapshot from the redo stack.
     *
     * The current state is moved onto the undo stack.
     *
     * @param current The state of the buffer before redoing.
     * @return The snapshot to restore, or std::nullopt if the redo stack is empty.
     */
    [[nodiscard]] std::optional<Snapshot> redo(Snapshot current);

    /** @brief Wipes both stacks and resets the history to a boundary. */
    void clear();
};


#endif //UNDO_HISTORY_H
