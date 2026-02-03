#ifndef QPREFERENCES_QPREFERENCES_H
#define QPREFERENCES_QPREFERENCES_H

#include <Preferences.h>
#include <type_traits>
#include <variant>
#include "PrefKey.h"
#include "CacheEntry.h"

namespace QPrefs {

namespace detail {
    /**
     * @brief Get unique cache ID for a preference key type.
     *
     * Uses static variable initialization to ensure each unique KeyType
     * gets a single, persistent ID throughout program lifetime.
     *
     * @tparam KeyType The PrefKey type
     * @return Unique ID for this key (index into cache_entries array)
     */
    template<typename KeyType>
    size_t get_key_id() {
        static size_t id = QPreferences::register_key();
        return id;
    }
} // namespace detail

/**
 * @brief Get a preference value with automatic type deduction and RAM caching.
 *
 * First access: Reads from NVS and caches in RAM.
 * Subsequent access: Returns cached value without NVS access.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @return The cached value, or default if never set
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   int value = QPrefs::get(countKey);  // Returns int automatically
 */
template<typename KeyType>
typename KeyType::value_type get(const KeyType& key) {
    using T = typename KeyType::value_type;
    auto& entry = QPreferences::cache_entries[detail::get_key_id<KeyType>()];

    // Lazy initialization: load from NVS only on first access
    if (!entry.is_initialized()) {
        Preferences prefs;
        prefs.begin(KeyType::namespace_name, true);  // true = read-only

        T nvs_result;

        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
            nvs_result = prefs.getInt(KeyType::key_name, key.default_value);
        } else if constexpr (std::is_same_v<T, float>) {
            nvs_result = prefs.getFloat(KeyType::key_name, key.default_value);
        } else if constexpr (std::is_same_v<T, bool>) {
            nvs_result = prefs.getBool(KeyType::key_name, key.default_value);
        } else if constexpr (std::is_same_v<T, String>) {
            nvs_result = prefs.getString(KeyType::key_name, key.default_value);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for QPreferences: supported types are int, float, bool, String");
        }

        prefs.end();

        // Store in both current value and NVS value
        entry.value = nvs_result;
        entry.nvs_value = nvs_result;
        entry.dirty = false;
    }

    // Return cached value
    return std::get<T>(entry.value);
}

/**
 * @brief Set a preference value in RAM cache only (no NVS write).
 *
 * Updates the cached value in RAM and marks the entry as dirty.
 * Does NOT write to NVS flash - use save() to persist changes.
 * The value type must match the key's value_type at compile time.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @param value The value to store (must match KeyType::value_type)
 * @return true (set always succeeds in RAM)
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   QPrefs::set(countKey, 42);      // OK: int matches int, RAM only
 *   // QPrefs::set(countKey, 3.14); // Compile error: float doesn't match int
 */
template<typename KeyType>
bool set(const KeyType& key, typename KeyType::value_type value) {
    using T = typename KeyType::value_type;
    auto& entry = QPreferences::cache_entries[detail::get_key_id<KeyType>()];

    // Ensure cache is initialized (loads nvs_value for isDirty comparison)
    if (!entry.is_initialized()) {
        // Lazy load from NVS to populate nvs_value
        get(key);
    }

    // Store value in RAM cache only
    entry.value = value;
    entry.dirty = true;  // Mark as dirty (RAM differs from NVS)

    return true;  // RAM write always succeeds
}

/**
 * @brief Check if a preference value differs from its default.
 *
 * Returns true if the current cached value (in RAM) is different from
 * the key's default_value, regardless of whether it's been saved to NVS.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @return true if current value != default_value, false otherwise
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   QPrefs::set(countKey, 0);
 *   bool modified = QPrefs::isModified(countKey);  // false (same as default)
 *   QPrefs::set(countKey, 42);
 *   modified = QPrefs::isModified(countKey);  // true (differs from default)
 */
template<typename KeyType>
bool isModified(const KeyType& key) {
    using T = typename KeyType::value_type;
    auto& entry = QPreferences::cache_entries[detail::get_key_id<KeyType>()];

    // Ensure cache is initialized
    if (!entry.is_initialized()) {
        get(key);  // Triggers lazy load
    }

    T current = std::get<T>(entry.value);
    return current != key.default_value;
}

/**
 * @brief Check if a preference value has unsaved changes.
 *
 * Returns true if the current cached value (in RAM) differs from the
 * last value read from or written to NVS flash storage.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key definition
 * @return true if RAM value differs from NVS, false otherwise
 *
 * Usage:
 *   PrefKey<int, "myapp", "count"> countKey{0};
 *   QPrefs::get(countKey);  // Load from NVS
 *   bool dirty = QPrefs::isDirty(countKey);  // false (just loaded)
 *   QPrefs::set(countKey, 42);
 *   dirty = QPrefs::isDirty(countKey);  // true (RAM differs from NVS)
 */
template<typename KeyType>
bool isDirty(const KeyType& key) {
    auto& entry = QPreferences::cache_entries[detail::get_key_id<KeyType>()];

    // Ensure cache is initialized
    if (!entry.is_initialized()) {
        get(key);  // Triggers lazy load
    }

    return entry.is_dirty();
}

} // namespace QPrefs

// Convenience: bring PrefKey into global scope for cleaner usage
using QPreferences::PrefKey;

#endif // QPREFERENCES_QPREFERENCES_H
