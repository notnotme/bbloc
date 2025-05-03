#ifndef ITEM_CALLBACK_H
#define ITEM_CALLBACK_H

#include <functional>
#include <string_view>


/**
 * @brief Template alias for an item callback function.
 *
 * An ItemCallback is used to pass individual items (e.g., completion suggestions)
 * back to a consumer during iterative processing. This is used by the
 * autocompletion system where suggestions are generated one at a time by other systems.
 *
 * @tparam T The character type (e.g., char, char16_t) of the string to receive.
 * @param input A string view representing the item being returned.
 */
template<typename T>
using ItemCallback = std::function<
    void(std::basic_string_view<T> input)
>;


#endif //ITEM_CALLBACK_H
