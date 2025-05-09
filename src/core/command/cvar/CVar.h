#ifndef CVAR_H
#define CVAR_H

#include <string>
#include <string_view>
#include <optional>
#include <vector>


/**
 * @brief Base class representing a configuration variable (CVar).
 *
 * CVars allow the user to view or modify runtime parameters.
 * Derived classes implement support for specific types (e.g., int, color) by inheriting TypedCVar.
 */
class CVar {
private:
    /** Allows access to private members from the command manager. */
    friend class CommandManager;

private:
    /** Indicates whether this CVar is read-only. */
    const bool m_is_read_only;

    /**
     * @brief Sets the CVar value using string arguments (to be implemented by subclasses).
     * This must not take into account the read-only status of the CVar.
     * @param args The parsed arguments list.
     * @return An optional message string in return.
     */
    virtual std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) = 0;

    /** @return The current value as a UTF-16 string. */
    [[nodiscard]] virtual std::u16string getStringValue() const = 0;

public:
    /** @brief Deleted copy constructor. */
    CVar(const CVar &) = delete;

    /** @brief Deleted copy assignment operator. */
    CVar &operator=(const CVar &) = delete;

    /** @brief Virtual destructor for polymorphic base class. */
    virtual ~CVar() = default;

    /**
     * @brief Constructs a CVar.
     * @param isReadOnly Whether this CVar is read-only.
     */
    explicit CVar(bool isReadOnly);

    /** @return true if the CVar cannot be modified. */
    [[nodiscard]] bool isReadOnly() const;
};


#endif //CVAR_H
