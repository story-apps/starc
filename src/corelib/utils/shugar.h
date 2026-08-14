#pragma once

#include <QString>

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

//
// Методы для работы со строками в switch
//

// --- Hashing Core ---
constexpr unsigned int fnv1a_base(unsigned int hash, char c)
{
    return (hash ^ static_cast<unsigned int>(c)) * 16777619u;
}
constexpr unsigned int q_switch_hash(const std::string_view& _str)
{
    unsigned int hash = 2166136261u;
    for (char c : _str) {
        hash = fnv1a_base(hash, c);
    }
    return hash;
}
constexpr unsigned int q_switch_hash(const QStringView& _str)
{
    unsigned int hash = 2166136261u;
    for (qsizetype i = 0; i < _str.size(); ++i) {
        hash = fnv1a_base(hash, static_cast<char>(_str[i].cell()));
    }
    return hash;
}

// --- Runtime Helper Overloads ---
// These helpers allow the single switchBy macro to accept any string type dynamically.
inline unsigned int get_hash(QStringView str)
{
    return q_switch_hash(str);
}
inline unsigned int get_hash(const QString& str)
{
    return q_switch_hash(QStringView(str));
}
inline unsigned int get_hash(std::string_view str)
{
    return q_switch_hash(str);
}
inline unsigned int get_hash(const char* str)
{
    return q_switch_hash(std::string_view(str));
}

// --- The Custom Macros ---
#define switchBy(text) switch (get_hash(text))
#define caseBy(literal) case q_switch_hash(literal):
#define defaultBy default:
