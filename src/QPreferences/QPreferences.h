#ifndef QPREFERENCES_QPREFERENCES_H
#define QPREFERENCES_QPREFERENCES_H

#include <Preferences.h>
#include <type_traits>
#include "PrefKey.h"

namespace QPrefs {

/**
 * @brief Get a preference value with automatic type deduction.
 *
 * Reads the preference from NVS using the key's namespace and key name.
 * Returns the default value if the preference has not been set.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @return The stored value, or the default if not found
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   int value = QPrefs::get(countKey);  // Returns int automatically
 */
template<typename KeyType>
typename KeyType::value_type get(const KeyType& key) {
    Preferences prefs;
    prefs.begin(KeyType::namespace_name, true);  // true = read-only

    using T = typename KeyType::value_type;
    T result;

    if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
        result = prefs.getInt(KeyType::key_name, key.default_value);
    } else if constexpr (std::is_same_v<T, float>) {
        result = prefs.getFloat(KeyType::key_name, key.default_value);
    } else if constexpr (std::is_same_v<T, bool>) {
        result = prefs.getBool(KeyType::key_name, key.default_value);
    } else if constexpr (std::is_same_v<T, String>) {
        result = prefs.getString(KeyType::key_name, key.default_value);
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type for QPreferences: supported types are int, float, bool, String");
    }

    prefs.end();
    return result;
}

/**
 * @brief Set a preference value with compile-time type safety.
 *
 * Writes the preference to NVS using the key's namespace and key name.
 * The value type must match the key's value_type at compile time.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @param value The value to store (must match KeyType::value_type)
 * @return true if write was successful, false otherwise
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   QPrefs::set(countKey, 42);      // OK: int matches int
 *   // QPrefs::set(countKey, 3.14); // Compile error: float doesn't match int
 */
template<typename KeyType>
bool set(const KeyType& key, typename KeyType::value_type value) {
    Preferences prefs;
    prefs.begin(KeyType::namespace_name, false);  // false = read-write

    using T = typename KeyType::value_type;
    size_t written = 0;

    if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
        written = prefs.putInt(KeyType::key_name, value);
    } else if constexpr (std::is_same_v<T, float>) {
        written = prefs.putFloat(KeyType::key_name, value);
    } else if constexpr (std::is_same_v<T, bool>) {
        written = prefs.putBool(KeyType::key_name, value);
    } else if constexpr (std::is_same_v<T, String>) {
        written = prefs.putString(KeyType::key_name, value);
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type for QPreferences: supported types are int, float, bool, String");
    }

    prefs.end();
    return written > 0;
}

} // namespace QPrefs

// Convenience: bring PrefKey into global scope for cleaner usage
using QPreferences::PrefKey;

#endif // QPREFERENCES_QPREFERENCES_H
