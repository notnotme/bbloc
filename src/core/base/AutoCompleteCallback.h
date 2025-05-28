#ifndef AUTO_COMPLETE_CALLBACK_H
#define AUTO_COMPLETE_CALLBACK_H

#include <functional>
#include <string_view>


/**
 * @brief Template alias for an auto-completion callback function.
 *
 * This callback is called for each item that potentially matches with the input.
 *
 * @param input A string view representing the item being returned.
 */
using AutoCompleteCallback = std::function<
    void(std::u16string_view input)
>;


#endif //AUTO_COMPLETE_CALLBACK_H
