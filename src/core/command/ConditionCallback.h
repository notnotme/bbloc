#ifndef CONDITION_CALLBACK_H
#define CONDITION_CALLBACK_H

#include <functional>
#include <string_view>
#include <vector>


using ConditionCallback = std::function<
    bool(const std::vector<std::u16string_view> &commandTokens)
>;


#endif //CONDITION_CALLBACK_H
