#ifndef QPREFERENCES_STRINGLITERAL_H
#define QPREFERENCES_STRINGLITERAL_H

#include <cstddef>

namespace QPreferences {

/**
 * @brief Compile-time string literal capture for C++20 NTTP usage.
 *
 * This structural type allows string literals to be used as non-type
 * template parameters. The string is copied into the struct at compile
 * time, making it available for template metaprogramming.
 *
 * @tparam N The size of the string including null terminator
 *
 * Usage:
 *   template<StringLiteral Str>
 *   struct Example {
 *       static constexpr const char* value = Str.value;
 *   };
 *
 *   Example<"hello"> e;  // e.value == "hello"
 */
template<std::size_t N>
struct StringLiteral {
    char value[N];

    /**
     * @brief Construct from a string literal.
     * @param str The string literal (array of char)
     */
    constexpr StringLiteral(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) {
            value[i] = str[i];
        }
    }

    /**
     * @brief Get the length of the string (excluding null terminator).
     * @return The number of characters in the string
     */
    constexpr std::size_t size() const {
        return N - 1;
    }
};

} // namespace QPreferences

#endif // QPREFERENCES_STRINGLITERAL_H
