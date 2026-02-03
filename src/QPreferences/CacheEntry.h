#ifndef QPREFERENCES_CACHEENTRY_H
#define QPREFERENCES_CACHEENTRY_H

#include <variant>
#include <optional>
#include <array>
#include <WString.h>

namespace QPreferences {

/**
 * @brief Type-safe variant for storing preference values in cache.
 *
 * Supports the four core ESP32 Preferences types: int32_t, float, bool, String.
 * Uses std::variant for fixed-size, RTTI-free storage (no heap allocation).
 */
using ValueVariant = std::variant<int32_t, float, bool, String>;

/**
 * @brief Cache entry for a single preference with three-state tracking.
 *
 * Each cache entry stores:
 * - value: Current cached value (in RAM)
 * - nvs_value: Last-known NVS value (empty = never loaded from NVS)
 * - dirty: Flag indicating if RAM value differs from NVS
 *
 * This enables:
 * - Lazy initialization (load from NVS only on first access)
 * - Dirty tracking (know which values need to be saved)
 * - Modified tracking (know which values differ from defaults)
 */
struct CacheEntry {
    /// Current cached value (in RAM)
    ValueVariant value;

    /// Last-known NVS value (empty = never loaded from NVS)
    std::optional<ValueVariant> nvs_value;

    /// Flag indicating if RAM value differs from NVS
    bool dirty = false;

    /**
     * @brief Check if this entry has been initialized from NVS.
     * @return true if nvs_value has been loaded, false otherwise
     */
    bool is_initialized() const {
        return nvs_value.has_value();
    }

    /**
     * @brief Check if this entry is dirty (RAM differs from NVS).
     * @return true if dirty flag is set, false otherwise
     */
    bool is_dirty() const {
        return dirty;
    }
};

/**
 * @brief Maximum number of unique preference keys supported.
 *
 * 64 keys provides ~2.5KB of cache storage on ESP32, which is a safe
 * balance between memory usage and flexibility for most applications.
 */
static constexpr size_t MAX_KEYS = 64;

/**
 * @brief Global cache storage for all preference entries.
 *
 * Static inline ensures single definition across translation units
 * (header-only implementation compatible with C++17).
 */
static inline std::array<CacheEntry, MAX_KEYS> cache_entries;

/**
 * @brief Counter for assigning unique IDs to preference keys.
 */
static inline size_t next_key_id = 0;

/**
 * @brief Metadata for a preference key, storing namespace and key name pointers.
 *
 * This enables save() to iterate cache entries and access the namespace/key name
 * for each entry without requiring template context at runtime.
 */
struct KeyMetadata {
    const char* namespace_name = nullptr;
    const char* key_name = nullptr;
};

/**
 * @brief Global metadata storage for all preference keys.
 *
 * Parallel array to cache_entries - same index maps to same key.
 */
static inline std::array<KeyMetadata, MAX_KEYS> key_metadata;

/**
 * @brief Register a new preference key and get its unique ID.
 * @param ns The namespace name for this key
 * @param key The key name within the namespace
 * @return Unique ID for this key (index into cache_entries array)
 */
inline size_t register_key(const char* ns, const char* key) {
    size_t id = next_key_id++;
    key_metadata[id] = {ns, key};
    return id;
}

} // namespace QPreferences

#endif // QPREFERENCES_CACHEENTRY_H
