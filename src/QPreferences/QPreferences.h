#ifndef QPREFERENCES_QPREFERENCES_H
#define QPREFERENCES_QPREFERENCES_H

#include <Preferences.h>
#include <type_traits>
#include <variant>
#include <cstring>
#include "PrefKey.h"
#include "CacheEntry.h"

namespace QPrefs {

namespace detail {
    /**
     * @brief Get unique cache ID for a preference key type.
     *
     * Uses static variable initialization to ensure each unique KeyType
     * gets a single, persistent ID throughout program lifetime.
     * Also registers the namespace and key name for runtime access by save().
     *
     * @tparam KeyType The PrefKey type
     * @return Unique ID for this key (index into cache_entries array)
     */
    template<typename KeyType>
    size_t get_key_id() {
        static size_t id = QPreferences::register_key(
            KeyType::namespace_name,
            KeyType::key_name
        );
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
        // false = read-write mode creates namespace if it doesn't exist
        // (read-only mode fails with NOT_FOUND on first boot)
        prefs.begin(KeyType::namespace_name, false);

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

/**
 * @brief Persist a single preference key to NVS flash.
 *
 * If the current value equals the default, removes the key from NVS (PERS-04).
 * If the current value differs from default, writes to NVS.
 * After save, isDirty(key) returns false.
 *
 * @tparam KeyType The PrefKey type (automatically deduced)
 * @param key The preference key to save
 */
template<typename KeyType>
void save(const KeyType& key) {
    using T = typename KeyType::value_type;
    auto& entry = QPreferences::cache_entries[detail::get_key_id<KeyType>()];
    auto& meta = QPreferences::key_metadata[detail::get_key_id<KeyType>()];

    if (!entry.is_initialized() || !entry.is_dirty()) {
        return;  // Nothing to save
    }

    Preferences prefs;
    prefs.begin(meta.namespace_name, false);  // false = read-write

    T current = std::get<T>(entry.value);

    if (current == key.default_value) {
        // Remove from NVS if equals default (PERS-04)
        prefs.remove(meta.key_name);
        entry.nvs_value.reset();  // Mark as no NVS value
    } else {
        // Write to NVS
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
            prefs.putInt(meta.key_name, current);
        } else if constexpr (std::is_same_v<T, float>) {
            prefs.putFloat(meta.key_name, current);
        } else if constexpr (std::is_same_v<T, bool>) {
            prefs.putBool(meta.key_name, current);
        } else if constexpr (std::is_same_v<T, String>) {
            prefs.putString(meta.key_name, current);
        }
        entry.nvs_value = entry.value;
    }

    prefs.end();
    entry.dirty = false;
}

/**
 * @brief Persist all dirty preference values to NVS flash in a single operation.
 *
 * Groups dirty entries by namespace and writes all entries in the same namespace
 * within a single begin/end cycle to minimize flash wear (PERS-05).
 *
 * Note: Unlike save(key), this function does NOT perform default value comparison
 * because it operates without template context. Values are always written.
 * Use save(key) for individual keys if you want default removal behavior.
 *
 * After save() completes, isDirty() returns false for all saved keys.
 */
inline void save() {
    Preferences prefs;
    const char* current_namespace = nullptr;

    for (size_t i = 0; i < QPreferences::cache_entries.size(); ++i) {
        auto& entry = QPreferences::cache_entries[i];

        // Skip uninitialized or clean entries
        if (!entry.is_initialized() || !entry.is_dirty()) {
            continue;
        }

        auto& meta = QPreferences::key_metadata[i];

        // Open new namespace if needed (namespace batching)
        if (current_namespace == nullptr || std::strcmp(current_namespace, meta.namespace_name) != 0) {
            if (current_namespace != nullptr) {
                prefs.end();  // Close previous namespace
            }
            prefs.begin(meta.namespace_name, false);  // false = read-write
            current_namespace = meta.namespace_name;
        }

        // Write value based on type stored in variant
        std::visit([&prefs, &meta, &entry](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int32_t>) {
                prefs.putInt(meta.key_name, val);
                entry.nvs_value = entry.value;
            } else if constexpr (std::is_same_v<T, float>) {
                prefs.putFloat(meta.key_name, val);
                entry.nvs_value = entry.value;
            } else if constexpr (std::is_same_v<T, bool>) {
                prefs.putBool(meta.key_name, val);
                entry.nvs_value = entry.value;
            } else if constexpr (std::is_same_v<T, String>) {
                prefs.putString(meta.key_name, val);
                entry.nvs_value = entry.value;
            }
        }, entry.value);

        entry.dirty = false;  // Clear dirty flag after write
    }

    if (current_namespace != nullptr) {
        prefs.end();  // Close final namespace
    }
}

/**
 * @brief Iterate over all registered preference keys.
 *
 * Calls the callback for each registered key with a PrefInfo struct
 * containing metadata and status. Does not expose raw values.
 * Use get(key) with your typed key to access values.
 *
 * @tparam Callback Callable accepting (const PrefInfo&)
 * @param callback Function to call for each registered key
 *
 * Usage:
 *   QPrefs::forEach([](const QPreferences::PrefInfo& info) {
 *       Serial.printf("%s/%s: %s\n",
 *           info.namespace_name, info.key_name,
 *           info.is_dirty ? "dirty" : "clean");
 *   });
 */
template<typename Callback>
void forEach(Callback callback) {
    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];
        auto& entry = QPreferences::cache_entries[i];

        QPreferences::PrefInfo info{
            meta.namespace_name,
            meta.key_name,
            i,
            entry.is_initialized(),
            entry.is_dirty()
        };
        callback(info);
    }
}

/**
 * @brief Iterate over registered keys in a specific namespace.
 *
 * Same as forEach() but only calls callback for keys matching the namespace.
 *
 * @tparam Callback Callable accepting (const PrefInfo&)
 * @param ns The namespace to filter by
 * @param callback Function to call for each matching key
 *
 * Usage:
 *   QPrefs::forEachInNamespace("myapp", [](const QPreferences::PrefInfo& info) {
 *       Serial.printf("  %s\n", info.key_name);
 *   });
 */
template<typename Callback>
void forEachInNamespace(const char* ns, Callback callback) {
    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];
        if (std::strcmp(meta.namespace_name, ns) == 0) {
            auto& entry = QPreferences::cache_entries[i];

            QPreferences::PrefInfo info{
                meta.namespace_name,
                meta.key_name,
                i,
                entry.is_initialized(),
                entry.is_dirty()
            };
            callback(info);
        }
    }
}

/**
 * @brief Clear all NVS entries and reset cache to uninitialized state.
 *
 * Groups keys by namespace and calls Preferences::clear() for each namespace.
 * Resets all cache entries to uninitialized state (nvs_value.reset(), dirty=false).
 * After factory reset, get(key) will return default values.
 *
 * WARNING: This permanently deletes all stored preference values from flash!
 */
inline void factoryReset() {
    Preferences prefs;
    const char* last_ns = nullptr;

    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];

        // Clear namespace if different from last (batch by namespace)
        if (last_ns == nullptr || std::strcmp(last_ns, meta.namespace_name) != 0) {
            if (last_ns != nullptr) {
                prefs.end();
            }
            prefs.begin(meta.namespace_name, false);  // false = read-write
            prefs.clear();  // Delete all keys in this namespace
            last_ns = meta.namespace_name;
        }

        // Reset cache entry to uninitialized state
        auto& entry = QPreferences::cache_entries[i];
        entry.nvs_value.reset();
        entry.dirty = false;
    }

    if (last_ns != nullptr) {
        prefs.end();
    }
}

} // namespace QPrefs

// Convenience: bring PrefKey into global scope for cleaner usage
using QPreferences::PrefKey;

#endif // QPREFERENCES_QPREFERENCES_H
