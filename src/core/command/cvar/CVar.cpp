#include "CVar.h"


CVar::CVar(const bool isReadOnly)
    : m_is_read_only(isReadOnly) {}

bool CVar::isReadOnly() const {
    return m_is_read_only;
}
