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
 * @brief Register a new preference key and get its unique ID.
 * @return Unique ID for this key (index into cache_entries array)
 */
inline size_t register_key() {
    return next_key_id++;
}

} // namespace QPreferences

#endif // QPREFERENCES_CACHEENTRY_H
