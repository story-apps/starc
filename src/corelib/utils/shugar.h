#pragma once

#include <iterator>


#define once while (false)

//
// Шаблоны для простой возможности инвертирования контейнеров
//

template<typename T>
struct reversion_wrapper {
    T& iterable;
};

template<typename T>
auto begin(reversion_wrapper<T> w)
{
    return std::rbegin(w.iterable);
}

template<typename T>
auto end(reversion_wrapper<T> w)
{
    return std::rend(w.iterable);
}

template<typename T>
reversion_wrapper<T> reversed(T&& iterable)
{
    return { iterable };
}
