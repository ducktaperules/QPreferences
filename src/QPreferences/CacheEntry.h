#ifndef QPREFERENCES_CACHEENTRY_H
#define QPREFERENCES_CACHEENTRY_H

#include <variant>
#include <optional>
#include <array>
#include <cassert>
#include <WString.h>

namespace QPreferences {

/**
 * @brief Type-safe variant for storing preference values in cache.
 *
 * Supports the core ESP32 Preferences types: int, int32_t, float, bool, String.
 * Both int and int32_t are included as distinct types (even if same size on ESP32).
 * Uses std::variant for fixed-size, RTTI-free storage (no heap allocation).
 */
using ValueVariant = std::variant<int, int32_t, float, bool, String>;

/**
 * @brief Cache entry for a single preference with four-state tracking.
 *
 * Each cache entry stores:
 * - value: Current cached value (in RAM)
 * - nvs_value: Last-known NVS value (empty = no value stored in NVS)
 * - initialized: Whether we've attempted to load from NVS (controls lazy loading)
 * - dirty: Flag indicating if RAM value differs from NVS baseline
 *
 * Key distinction:
 * - initialized tracks "have we tried to load this?" (lazy loading gate)
 * - nvs_value.has_value() tracks "does NVS actually have a stored value?"
 *
 * This enables:
 * - Lazy initialization (load from NVS only on first access)
 * - Smart dirty tracking (compare against NVS value or default appropriately)
 * - Read-only NVS access (don't create namespaces unnecessarily)
 */
struct CacheEntry {
    /// Current cached value (in RAM)
    ValueVariant value;

    /// Last-known NVS value (empty = no value stored in NVS)
    std::optional<ValueVariant> nvs_value;

    /// Whether this entry has been initialized (attempted load from NVS)
    bool initialized = false;

    /// Flag indicating if RAM value differs from NVS baseline
    bool dirty = false;

    /**
     * @brief Check if this entry has been initialized from NVS.
     * @return true if initialization has been attempted, false otherwise
     */
    bool is_initialized() const {
        return initialized;
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
 * Default is 64. Override by defining QPREFERENCES_MAX_KEYS via build flags
 * (e.g., -DQPREFERENCES_MAX_KEYS=96) to increase capacity.
 */
#ifndef QPREFERENCES_MAX_KEYS
#define QPREFERENCES_MAX_KEYS 64
#endif
static constexpr size_t MAX_KEYS = QPREFERENCES_MAX_KEYS;

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
 * @brief Information about a preference, passed to forEach callbacks.
 *
 * Provides access to key metadata and status without exposing
 * the raw value (user accesses value via get(key) with their typed key).
 */
struct PrefInfo {
    const char* namespace_name;   ///< The namespace this key belongs to
    const char* key_name;         ///< The key name within the namespace
    size_t index;                 ///< Index into cache_entries array
    bool is_initialized;          ///< Whether key has been loaded from NVS
    bool is_dirty;                ///< Whether RAM differs from NVS
};

/**
 * @brief Register a new preference key and get its unique ID.
 * @param ns The namespace name for this key
 * @param key The key name within the namespace
 * @return Unique ID for this key (index into cache_entries array)
 */
inline size_t register_key(const char* ns, const char* key) {
    // Guard against exceeding configured capacity; fail-fast in debug.
    assert(next_key_id < MAX_KEYS && "QPreferences: preference key limit exceeded (increase QPREFERENCES_MAX_KEYS)");
    if (next_key_id >= MAX_KEYS) {
        // Fail-safe: avoid out-of-bounds write. Reuse last valid index.
        return MAX_KEYS - 1;
    }
    size_t id = next_key_id++;
    key_metadata[id] = {ns, key};
    return id;
}

} // namespace QPreferences

#endif // QPREFERENCES_CACHEENTRY_H
