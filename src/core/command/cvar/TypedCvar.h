#ifndef TYPED_CVAR_H
#define TYPED_CVAR_H
#include "CVar.h"


/**
 * @brief Template class representing a typed configuration variable.
 *
 * Inherits from CVar and provides storage and initialization for a specific type.
 * Specializations of this class should implement the setValueFromStrings and getStringValue methods.
 *
 * @tparam T The type of the configuration variable.
 */
template<typename T>
class TypedCvar : public CVar {
public:
    /** @brief The current value of the typed CVar. */
    T m_value;

public:
    /**
     * @brief Constructs a TypedCvar with the given value and read-only flag.
     * @param value The initial value of this CVar.
     * @param isReadOnly Whether this CVar is read-only.
     */
    explicit TypedCvar(T value, bool isReadOnly);
};

template<typename T>
TypedCvar<T>::TypedCvar(T value, const bool isReadOnly)
    : CVar(isReadOnly),
      m_value(value) {}


#endif //TYPED_CVAR_H
