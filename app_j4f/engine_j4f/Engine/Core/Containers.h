#pragma once

#include <array>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace engine {

    template <typename...Args>
    using array = std::array<Args...>;

    template <typename...Args>
    using deque = std::deque<Args...>;

    template <typename...Args>
    using list = std::list<Args...>;

    template <typename...Args>
    using map = std::map<Args...>;

    template <typename...Args>
    using queue = std::queue<Args...>;

    template <typename...Args>
    using span = std::span<Args...>;

    template <typename...Args>
    using set = std::set<Args...>;

    using string = std::string;

    using string_view = std::string_view;

    template <typename...Args>
    using unordered_map = std::unordered_map<Args...>;

    template <typename...Args>
    using unordered_set = std::unordered_set<Args...>;

    template <typename...Args>
    using vector = std::vector<Args...>;

}