/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef TYPED_CVAR_H
#define TYPED_CVAR_H
#include "../base/CVar.h"


/**
 * @brief Template class representing a typed configuration variable.
 *
 * Inherits from CVar and provides storage and initialization for a specific type.
 * Specializations of this class should implement the setValueFromStrings and getStringValue methods.
 *
 * @tparam T The type of the configuration variable.
 */
template<typename T>
class TypedCVar : public CVar {
public:
    /** The current value of the typed CVar. */
    T m_value;

public:
    /**
     * @brief Constructs a TypedCVar with the given value and read-only flag.
     *
     * @param value The initial value of this CVar.
     * @param isReadOnly Whether this CVar is read-only.
     */
    explicit TypedCVar(T value, bool isReadOnly);
};

template<typename T>
TypedCVar<T>::TypedCVar(T value, const bool isReadOnly)
    : CVar(isReadOnly),
      m_value(value) {}


#endif //TYPED_CVAR_H
