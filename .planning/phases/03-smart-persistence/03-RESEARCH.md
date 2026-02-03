# Phase 3: Smart Persistence - Research

**Researched:** 2026-02-03
**Domain:** ESP32 NVS batch write operations, flash-friendly persistence, explicit save patterns
**Confidence:** HIGH

## Summary

Phase 3 implements the explicit persistence layer for the QPreferences library, transforming the Phase 2 RAM cache with dirty tracking into a flash-friendly persistence system. The research confirms that ESP32's NVS architecture is inherently optimized for batch operations through its begin/end lifecycle pattern, where multiple key operations are buffered and written atomically.

The standard approach for embedded systems with flash storage involves three key patterns: (1) explicit save operations that batch dirty writes to minimize flash operations, (2) default value optimization where values matching defaults are removed from storage rather than written (reducing NVS consumption and improving cache efficiency), and (3) namespace-aware batching where all keys within the same namespace are written in a single begin/end cycle to leverage NVS's atomic commit behavior.

ESP32's Preferences library provides remove() for key deletion and wraps the underlying NVS's wear-leveling system that reduces flash erase frequency by a factor of 126. The library automatically calls nvs_commit() on each putX() operation, making individual writes durable but also making them separate flash operations. Phase 3's save() function will batch multiple dirty keys, opening each namespace once and writing all dirty keys within that namespace before moving to the next namespace.

**Primary recommendation:** Implement save() to iterate through dirty cache entries grouped by namespace, opening each namespace once in read-write mode, writing all dirty entries (or removing if value matches default), and closing the namespace before proceeding to the next. Clear dirty flags only after successful write. Handle boot-time loading separately with lazy initialization (already in Phase 2) to avoid unnecessary NVS reads.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Preferences.h | ESP32 Arduino 3.0+ | NVS batch operations via begin/end | Built-in namespace lifecycle, automatic nvs_commit(), remove() for key deletion |
| std::variant | C++17 | Type-safe cache value storage | From Phase 2 cache; enables type dispatch for putX operations |
| std::array | C++17 | Fixed-size cache entry storage | From Phase 2; provides iteration for dirty entry detection |
| std::optional | C++17 | Three-state tracking (uninitialized/clean/dirty) | From Phase 2; differentiates never-loaded from default-equals-stored |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| <algorithm> | C++17 | std::sort, std::for_each for namespace grouping | Efficient namespace batching if multiple namespaces have dirty keys |
| <type_traits> | C++17 | std::is_same_v for type dispatch | Dispatch to correct putX method based on variant type |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Namespace batching | Write each key individually | Batching reduces begin/end cycles from N operations to K namespaces |
| Remove when equals default | Always write values | Removal saves NVS space (4KB pages) and improves cache locality |
| Explicit save() | Auto-save on set() | Explicit control minimizes flash wear; auto-save wastes cycles |
| Dirty flag clearing | Transaction rollback pattern | Simple clear-on-write sufficient; rollback adds complexity without benefit |

**Installation:**
```bash
# No additional dependencies - builds on Phase 1 & 2 stack
# Requires ESP32 Arduino core 3.0+ for C++17 support
platform = espressif32@^6.0.0
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── QPreferences/
│   ├── QPreferences.h        # Main API with save() function
│   ├── PrefKey.h              # Key definitions (Phase 1)
│   ├── CacheEntry.h           # Cache storage (Phase 2)
│   └── StringLiteral.h        # Compile-time strings (Phase 1)
```

### Pattern 1: Namespace-Grouped Batch Write
**What:** Group dirty cache entries by namespace, write all entries in same namespace within single begin/end cycle
**When to use:** When multiple keys may be dirty across different namespaces (typical use case)
**Example:**
```cpp
// Source: Synthesized from https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
// and Phase 2 cache architecture
void save() {
    Preferences prefs;

    // Build namespace -> [cache_entry_indices] map
    // For simplicity, iterate cache and group by namespace
    const char* current_namespace = nullptr;

    for (size_t i = 0; i < cache_entries.size(); ++i) {
        auto& entry = cache_entries[i];

        if (!entry.is_dirty()) continue;  // Skip clean entries

        // Get namespace for this cache entry (requires metadata)
        const char* entry_namespace = get_namespace_for_entry(i);

        // Open new namespace if needed
        if (current_namespace == nullptr || strcmp(current_namespace, entry_namespace) != 0) {
            if (current_namespace != nullptr) {
                prefs.end();  // Close previous namespace
            }
            prefs.begin(entry_namespace, false);  // false = read-write
            current_namespace = entry_namespace;
        }

        // Write or remove based on comparison to default
        write_or_remove_entry(prefs, entry, i);
        entry.dirty = false;  // Clear dirty flag after successful write
    }

    if (current_namespace != nullptr) {
        prefs.end();  // Close final namespace
    }
}
```

### Pattern 2: Default Value Optimization (Remove if Equals Default)
**What:** Compare cached value to default; if equal, remove key from NVS rather than writing
**When to use:** Always - reduces NVS consumption and improves read performance
**Example:**
```cpp
// Source: Synthesized from https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
// and embedded systems storage best practices
template<typename KeyType>
void write_or_remove(Preferences& prefs, const KeyType& key) {
    using T = typename KeyType::value_type;
    auto& entry = cache_entries[detail::get_key_id<KeyType>()];

    T current_value = std::get<T>(entry.value);

    // If current value matches default, remove from NVS (don't store defaults)
    if (current_value == key.default_value) {
        prefs.remove(KeyType::key_name);  // Returns true on success
    } else {
        // Value differs from default - store it
        if constexpr (std::is_same_v<T, int32_t>) {
            prefs.putInt(KeyType::key_name, current_value);
        } else if constexpr (std::is_same_v<T, float>) {
            prefs.putFloat(KeyType::key_name, current_value);
        } else if constexpr (std::is_same_v<T, bool>) {
            prefs.putBool(KeyType::key_name, current_value);
        } else if constexpr (std::is_same_v<T, String>) {
            prefs.putString(KeyType::key_name, current_value);
        }
    }

    // Update nvs_value to reflect what's now in storage
    if (current_value == key.default_value) {
        entry.nvs_value = std::nullopt;  // Not in NVS
    } else {
        entry.nvs_value = current_value;
    }

    entry.dirty = false;  // Clear dirty flag
}
```

### Pattern 3: Lazy Boot-Time Loading (No Eager Load)
**What:** Don't load all keys from NVS at boot; rely on Phase 2's lazy initialization in get()
**When to use:** Always - Phase 2 already implements this; Phase 3 only adds save()
**Example:**
```cpp
// Source: Phase 2 research - lazy initialization pattern
// NO BOOT-TIME LOADING NEEDED

// Phase 2's get() already loads on first access:
template<typename KeyType>
typename KeyType::value_type get(const KeyType& key) {
    auto& entry = cache_entries[detail::get_key_id<KeyType>()];

    if (!entry.is_initialized()) {
        // First access - load from NVS
        Preferences prefs;
        prefs.begin(KeyType::namespace_name, true);  // read-only

        using T = typename KeyType::value_type;
        T nvs_val = /* read from NVS with type dispatch */;

        entry.value = nvs_val;
        entry.nvs_value = nvs_val;  // Track what's in NVS
        entry.dirty = false;

        prefs.end();
    }

    return std::get<typename KeyType::value_type>(entry.value);
}

// Phase 3 does NOT need explicit boot loading - lazy init handles it
```

### Pattern 4: Dirty Flag Management
**What:** Clear dirty flag only after successful write/remove; preserve on failure
**When to use:** Always - ensures data integrity if write fails
**Example:**
```cpp
// Source: Dirty flag pattern from https://gameprogrammingpatterns.com/dirty-flag.html
template<typename KeyType>
bool save_single(const KeyType& key) {
    auto& entry = cache_entries[detail::get_key_id<KeyType>()];

    if (!entry.is_dirty()) {
        return true;  // Nothing to save
    }

    Preferences prefs;
    if (!prefs.begin(KeyType::namespace_name, false)) {
        return false;  // Failed to open namespace
    }

    bool success = write_or_remove(prefs, key);
    prefs.end();

    if (success) {
        entry.dirty = false;  // Clear only on success
    }

    return success;
}
```

### Anti-Patterns to Avoid
- **Eager boot-time NVS loading:** Wastes RAM and startup time; lazy init (Phase 2) is sufficient
- **Writing defaults to NVS:** Wastes 4KB NVS pages and slows reads; remove keys that match defaults
- **Individual namespace open/close per key:** Multiplies flash operations; batch by namespace
- **Clearing dirty flag before write confirmation:** Risk of data loss if write fails
- **Opening namespace in read-only mode for save():** putX() and remove() will silently fail

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Atomic commit of multiple keys | Manual transaction log | Preferences begin/end lifecycle | NVS already buffers writes; begin/end provides atomic commit |
| Flash wear management | Manual write counting | NVS built-in wear leveling | 126x reduction in erase cycles through page architecture |
| Key deletion from NVS | Writing sentinel values | Preferences.remove() | Sentinel values still consume NVS space; remove() frees it |
| Default value comparison | Custom comparator per type | operator== (works for int/float/bool/String) | Arduino String and primitives already implement correct comparison |
| Namespace grouping algorithm | Custom sorting | Linear iteration with last_namespace tracking | For small N (<64 keys), sorting overhead exceeds benefit |

**Key insight:** ESP32 NVS is transactional at the namespace level. The begin/end pattern already provides atomic commit semantics - multiple putX() calls within one begin/end are committed together. Don't add a separate transaction layer; leverage the existing one.

## Common Pitfalls

### Pitfall 1: Float NaN Comparison Returns False
**What goes wrong:** If default value is NaN, comparison current_value == key.default_value always returns false
**Why it happens:** IEEE 754 specifies NaN != NaN for all comparisons, including equality
**How to avoid:** Document that default values should not be NaN, or add special case: if (isnan(default_value) && isnan(current_value)) { treat as equal }
**Warning signs:** Keys with NaN defaults are always written to NVS even when value is unchanged

### Pitfall 2: Clearing Dirty Flag Before Write Completes
**What goes wrong:** Write fails but dirty flag already cleared; data is lost on next reboot
**Why it happens:** Optimistically clearing dirty flag at start of write rather than after success
**How to avoid:** Always clear dirty flag AFTER putX()/remove() succeeds and prefs.end() completes
**Warning signs:** Data loss occurs intermittently when flash is full or power cycles during write

### Pitfall 3: Only One Namespace Can Be Open at a Time
**What goes wrong:** Opening second namespace without closing first invalidates first handle; writes fail
**Why it happens:** Preferences.begin() implicitly closes any previously open namespace
**How to avoid:** Always call prefs.end() before calling prefs.begin() with different namespace
**Warning signs:** Keys in first namespace not saved; logs show nvs_set_X fail: handle closed

### Pitfall 4: Removing Key That Doesn't Exist
**What goes wrong:** Preferences.remove() returns false but doesn't indicate whether key existed
**Why it happens:** NVS returns ESP_ERR_NVS_NOT_FOUND but Preferences converts to simple bool
**How to avoid:** Ignore return value of remove() - removing non-existent key is safe operation
**Warning signs:** Spurious save() failures reported when actually successful

### Pitfall 5: String Comparison Creates Temporary Objects
**What goes wrong:** String operator== may allocate temporaries, fragmenting heap during comparison
**Why it happens:** Arduino String overloads can create intermediate String objects
**How to avoid:** For String types, use equals() method or compare c_str() with strcmp if performance critical
**Warning signs:** Heap fragmentation increases after repeated save() operations with String keys

### Pitfall 6: nvs_value Not Updated After Write/Remove
**What goes wrong:** isDirty() returns true immediately after save() because nvs_value still reflects old value
**Why it happens:** Forgetting to update nvs_value after successful write to match what's now in NVS
**How to avoid:** After write/remove, set nvs_value = current_value (or std::nullopt if removed)
**Warning signs:** isDirty() always true even after successful save()

### Pitfall 7: Iterating Uninitialized Cache Entries
**What goes wrong:** Attempting to write uninitialized entries (never accessed) causes variant access error
**Why it happens:** Cache entries are created at compile time but only initialized on first get()
**How to avoid:** In save(), skip entries where !entry.is_initialized() - nothing to save if never loaded
**Warning signs:** std::bad_variant_access exception during save() operation

## Code Examples

Verified patterns from official sources:

### Preferences begin/end Batching
```cpp
// Source: https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
Preferences preferences;

// Open namespace once
preferences.begin("myNamespace", false);  // false = read-write mode

// Multiple writes within single begin/end - batched in NVS
preferences.putInt("key1", 42);
preferences.putFloat("key2", 3.14f);
preferences.putBool("key3", true);

// Close namespace - commits all pending writes atomically
preferences.end();
```

### Removing Keys from NVS
```cpp
// Source: https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
Preferences prefs;
prefs.begin("namespace", false);

// Remove key from NVS - frees storage space
bool removed = prefs.remove("keyName");  // true if removed, false if not found

prefs.end();
```

### Comparing Values to Defaults
```cpp
// Source: Arduino String comparison - https://docs.arduino.cc/built-in-examples/strings/StringComparisonOperators/
// Primitives use standard operator==
int current = 42;
int defaultVal = 0;
if (current == defaultVal) { /* matches default */ }

// Arduino String uses operator==
String current = "hello";
String defaultVal = "world";
if (current == defaultVal) { /* matches default */ }

// Alternative for String: equals() method
if (current.equals(defaultVal)) { /* matches default */ }
```

### Iterating Cache with Namespace Grouping
```cpp
// Source: Synthesized from std::array iteration patterns
#include <array>
#include <algorithm>

// Simple approach: track last namespace, minimize begin/end cycles
void save_all_dirty() {
    Preferences prefs;
    const char* current_namespace = nullptr;
    bool namespace_open = false;

    for (size_t i = 0; i < cache_entries.size(); ++i) {
        auto& entry = cache_entries[i];

        // Skip uninitialized or clean entries
        if (!entry.is_initialized() || !entry.is_dirty()) {
            continue;
        }

        // Get namespace for this entry (requires metadata)
        const char* ns = entry_metadata[i].namespace_name;

        // Switch namespace if needed
        if (!namespace_open || strcmp(current_namespace, ns) != 0) {
            if (namespace_open) {
                prefs.end();
            }
            prefs.begin(ns, false);
            current_namespace = ns;
            namespace_open = true;
        }

        // Write or remove entry
        write_entry(prefs, entry, i);
        entry.dirty = false;
    }

    if (namespace_open) {
        prefs.end();
    }
}
```

### NVS Wear Leveling Information
```cpp
// Source: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
// NVS reduces flash erase frequency by factor of 126 through page organization
// Flash lifetime: typically 100K erase cycles per page
// With wear leveling: 100K * 126 = 12.6M effective write cycles

// Example: 16KB NVS partition, 64-byte keys
// Can write same key ~256 times before page erase needed
// With 126x leveling: 256 * 126 = 32,256 writes before flash wear concern

// Best practice: Batch writes to same namespace to leverage single begin/end
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Auto-save on every set() | Explicit save() with dirty tracking | Standard pattern | Reduces flash writes from thousands to <10 per session |
| Write defaults to NVS | Remove keys that equal defaults | Embedded best practice | Saves NVS space, improves cache efficiency |
| Individual key writes | Namespace-batched writes | NVS architecture | Reduces begin/end overhead from N to K (namespace count) |
| Eager boot loading | Lazy initialization on first access | Phase 2 pattern | Saves RAM and startup time; only load what's used |
| Manual nvs_commit() | Preferences auto-commits in end() | Preferences library | Simpler API; durability guaranteed automatically |

**Deprecated/outdated:**
- **EEPROM-style commit() pattern:** ESP32 NVS doesn't need explicit commit; Preferences.end() handles it
- **Writing sentinel values for deletion:** Use Preferences.remove() to free NVS space
- **Synchronous boot-time loading:** Lazy init is faster and more RAM-efficient

## Open Questions

Things that couldn't be fully resolved:

1. **Metadata Storage for Namespace Association**
   - What we know: Each cache entry needs namespace/key name for write operations
   - What's unclear: Best way to associate cache_entries[i] with KeyType's namespace/key at runtime
   - Recommendation: Add parallel array of metadata structs with namespace/key pointers, or embed in CacheEntry

2. **Save Return Value Semantics**
   - What we know: Individual putX() and remove() return success/failure bools
   - What's unclear: Should save() return bool (all succeeded), count (N keys saved), or void (fire-and-forget)?
   - Recommendation: Return void for simplicity; add optional save_with_status() that returns detailed result later

3. **Partial Save on Failure**
   - What we know: If namespace A succeeds but namespace B fails, some keys are saved
   - What's unclear: Should dirty flags be cleared for successful namespace only, or none?
   - Recommendation: Clear per-namespace - partial success is still progress; user can retry for failed namespace

4. **Float Equality with Epsilon**
   - What we know: Direct float equality (==) may fail for nearly-equal values due to rounding
   - What's unclear: Should default comparison use epsilon tolerance or exact equality?
   - Recommendation: Use exact equality for simplicity; document that float defaults should be exactly representable (like 0.0f, 1.0f)

5. **Cache Entry Initialization During save()**
   - What we know: Uninitialized entries should be skipped (nothing to save)
   - What's unclear: Should save() trigger lazy load to check if NVS value needs removal?
   - Recommendation: Skip uninitialized entries; if never accessed, assume nothing to persist

## Sources

### Primary (HIGH confidence)
- ESP-IDF NVS Documentation: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
- Preferences Library Tutorial: https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
- Preferences.h API: https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
- Arduino String Comparison: https://docs.arduino.cc/built-in-examples/strings/StringComparisonOperators/
- Dirty Flag Pattern: https://gameprogrammingpatterns.com/dirty-flag.html
- std::variant Comparison: https://en.cppreference.com/w/cpp/utility/variant/operator_cmp

### Secondary (MEDIUM confidence)
- ESP32 NVS Best Practices (ESP32 Forum): https://www.esp32.com/viewtopic.php?t=3990
- Embedded Storage Best Practices: https://www.embeddedrelated.com/showthread/comp.arch.embedded/213853-1.php
- Boot Time Optimization Patterns: https://theembeddedkit.io/blog/boot-time-embedded-linux/
- NVS Comprehensive Guide: https://medium.com/engineering-iot/nvs-data-storage-and-reading-in-esp32-a-comprehensive-guide-12bdbc6325ac
- Random Nerd Tutorials Preferences: https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

### Tertiary (LOW confidence)
- Arduino String equals() discussion: https://arduinogetstarted.com/reference/arduino-string-equals
- ESP32 Preferences remove() usage: https://forum.arduino.cc/t/how-to-use-the-library-preferences-h/1375772
- Float NaN comparison behavior: https://www.modernescpp.com/index.php/the-autogenerated-equality-operator/

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Preferences API verified from official source code and ESP-IDF docs
- Architecture: HIGH - Namespace batching confirmed from Preferences lifecycle model; default removal is standard embedded practice
- Pitfalls: HIGH - Float NaN behavior from IEEE 754 standard; namespace limitations from official docs; dirty flag management from game programming patterns

**Research date:** 2026-02-03
**Valid until:** ~90 days (stable domain: ESP32 NVS architecture stable, Preferences API mature, patterns timeless)
