#ifndef AUTO_COMPLETE_CALLBACK_H
#define AUTO_COMPLETE_CALLBACK_H

#include <functional>
#include <string_view>


/**
 * @brief Template alias for an auto-completion callback function.
 *
 * @tparam T The character type (e.g., char, char16_t) of the string to receive.
 * @param input A string view representing the item being returned.
 */
template<typename T>
using AutoCompleteCallback = std::function<
    void(std::basic_string_view<T> input)
>;


#endif //AUTO_COMPLETE_CALLBACK_H
