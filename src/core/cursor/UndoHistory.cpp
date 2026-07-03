#include "UndoHistory.h"


UndoHistory::UndoHistory()
    : m_at_boundary(true) {}

void UndoHistory::markBoundary() {
    m_at_boundary = true;
}

bool UndoHistory::isAtBoundary() const {
    return m_at_boundary;
}

void UndoHistory::push(Snapshot snapshot) {
    m_redo_stack.clear();
    m_undo_stack.emplace_back(std::move(snapshot));
    if (m_undo_stack.size() > MAX_HISTORY_DEPTH) {
        m_undo_stack.pop_front();
    }

    m_at_boundary = false;
}

std::optional<UndoHistory::Snapshot> UndoHistory::undo(Snapshot current) {
    if (m_undo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();

    m_redo_stack.emplace_back(std::move(current));
    if (m_redo_stack.size() > MAX_HISTORY_DEPTH) {
        m_redo_stack.pop_front();
    }

    return snapshot;
}

std::optional<UndoHistory::Snapshot> UndoHistory::redo(Snapshot current) {
    if (m_redo_stack.empty()) {
        return std::nullopt;
    }

    auto snapshot = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();

    m_undo_stack.emplace_back(std::move(current));
    if (m_undo_stack.size() > MAX_HISTORY_DEPTH) {
        m_undo_stack.pop_front();
    }

    return snapshot;
}

void UndoHistory::clear() {
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_at_boundary = true;
}
