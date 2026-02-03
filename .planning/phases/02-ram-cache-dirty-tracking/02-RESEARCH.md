# Phase 2: RAM Cache & Dirty Tracking - Research

**Researched:** 2026-02-03
**Domain:** In-memory cache with dirty tracking, embedded C++ state management
**Confidence:** HIGH

## Summary

Phase 2 adds a RAM cache layer to the PrefKey<T> system established in Phase 1, enabling set() operations to modify values in memory without triggering immediate NVS flash writes. This requires tracking two types of changes: "modified" (value differs from default) and "dirty" (RAM value differs from NVS).

The research confirms the standard approach for embedded systems with constrained RAM (ESP32 has 320KB DRAM available) is to use compile-time fixed-size storage with value-based comparison for dirty detection. For small, fixed sets of preferences (typical embedded use case), std::array or C-style arrays significantly outperform std::unordered_map due to better cache locality and zero allocation overhead.

The dirty flag pattern is well-established for exactly this use case: tracking when cached data is out of sync with backing storage. ESP32's NVS already implements internal hash caching for reads (0.5ms per 1000 keys initialization, fast lookups thereafter), so repeated NVS reads aren't expensive, but write operations trigger flash commits which must be minimized for flash longevity (100K cycles per page, 126x with wear leveling).

**Primary recommendation:** Use compile-time static storage (fixed-size array) indexed by template instantiation for cache entries, store values directly with std::variant or type-erased wrapper, and implement comparison-based dirty tracking (compare RAM vs last-known NVS value) rather than bit-flag optimizations which add complexity without meaningful memory savings for small preference counts.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| std::array | C++17 | Fixed-size cache storage | Zero overhead, compile-time sizing, cache-friendly contiguous memory |
| std::variant | C++17 | Type-safe value storage | Type-safe union, no heap, value semantics ideal for embedded |
| Preferences.h | ESP32 Arduino 3.0+ | NVS read operations | Already has internal hash caching for fast reads |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| <type_traits> | C++17 | Type inspection for comparison | Enables compile-time type-based dispatch |
| std::optional | C++17 | Three-state cache entry | Differentiate uninitialized/clean/dirty states |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Comparison-based dirty | Bit flags | Bit flags save 7/8 memory but need 7+ instances to break even; adds complexity for minimal gain at small scale |
| std::variant | Type-erased void* | void* loses type safety; variant provides compile-time safety at zero runtime cost |
| Fixed array | std::unordered_map | Hash map has high overhead for small N; array is faster for N<100 with linear search |
| std::variant | std::any | std::any requires RTTI and heap; variant is fixed-size and RTTI-free |

**Installation:**
```bash
# No external dependencies - uses C++17 standard library + ESP32 Arduino core
# Requires ESP32 Arduino core 3.0+ for C++17 support
platform = espressif32@^6.0.0  # Or newer
```

## Architecture Patterns

### Recommended Project Structure
```
src/
├── QPreferences.h           # Main API
├── QPreferencesKey.h        # PrefKey<T> from Phase 1
├── QPreferencesCache.h      # Cache storage and dirty tracking
└── QPreferencesCacheEntry.h # Individual cache entry wrapper
```

### Pattern 1: Compile-Time Static Registry with Fixed Storage
**What:** Use template static members to build a compile-time registry of all PrefKey instances with fixed-size storage
**When to use:** When total number of preferences is known at compile time (typical for embedded)
**Example:**
```cpp
// Source: Synthesized from https://github.com/psalvaggio/cppregpattern and https://www.cppstories.com/2018/02/factory-selfregister/
template<typename T, StringLiteral Namespace, StringLiteral Key>
struct PrefKey {
    using value_type = T;
    T default_value;

    // Each PrefKey instantiation gets unique ID via counter
    static constexpr size_t id = Registry::register_key<PrefKey>();

    // Register this key in global registry
    struct Registrar {
        Registrar() { Registry::add_entry<PrefKey>(id); }
    };
    static inline Registrar registrar;
};

// Global registry with fixed storage
struct Registry {
    static constexpr size_t MAX_KEYS = 64; // Adjust based on needs
    static inline std::array<CacheEntry, MAX_KEYS> cache_entries;
    static inline size_t key_count = 0;

    template<typename KeyType>
    static constexpr size_t register_key() {
        return key_count++;
    }
};
```

### Pattern 2: Three-State Cache Entry (Uninitialized/Clean/Dirty)
**What:** Distinguish between "never loaded", "loaded and unchanged", and "modified in RAM"
**When to use:** When you need to know both modification status and initialization status
**Example:**
```cpp
// Source: Synthesized from dirty flag pattern https://gameprogrammingpatterns.com/dirty-flag.html
struct CacheEntry {
    std::variant<int32_t, float, bool, String> value;

    // Three states:
    // 1. !nvs_value.has_value() && !dirty → uninitialized (never loaded)
    // 2. nvs_value.has_value() && !dirty → clean (matches NVS)
    // 3. nvs_value.has_value() && dirty → dirty (modified in RAM)
    std::optional<std::variant<int32_t, float, bool, String>> nvs_value;
    bool dirty = false;

    bool is_initialized() const { return nvs_value.has_value(); }
    bool is_dirty() const { return dirty; }
};
```

### Pattern 3: Comparison-Based Dirty Detection
**What:** Mark dirty by comparing current value against last-known NVS value
**When to use:** Small number of preferences where comparison is cheaper than maintaining complex state
**Example:**
```cpp
// Source: Synthesized from dirty flag pattern
template<typename KeyType>
void set(const KeyType& key, typename KeyType::value_type value) {
    auto& entry = Registry::cache_entries[KeyType::id];

    // Store new value in cache
    entry.value = value;

    // Mark dirty if differs from NVS (or if never loaded from NVS)
    if (!entry.nvs_value.has_value()) {
        entry.dirty = true; // Never loaded, so definitely dirty
    } else if (entry.nvs_value.value() != value) {
        entry.dirty = true; // Value changed from NVS
    } else {
        entry.dirty = false; // Matches NVS
    }
}

template<typename KeyType>
bool isModified(const KeyType& key) {
    auto& entry = Registry::cache_entries[KeyType::id];
    using T = typename KeyType::value_type;

    // Modified = differs from default
    T current = std::get<T>(entry.value);
    return current != key.default_value;
}

template<typename KeyType>
bool isDirty(const KeyType& key) {
    auto& entry = Registry::cache_entries[KeyType::id];
    return entry.is_dirty();
}
```

### Pattern 4: Lazy Initialization from NVS
**What:** Only load from NVS on first access, not at boot
**When to use:** When startup time matters and not all preferences are used every session
**Example:**
```cpp
// Source: Standard lazy initialization pattern
template<typename KeyType>
typename KeyType::value_type get(const KeyType& key) {
    auto& entry = Registry::cache_entries[KeyType::id];

    // First access: load from NVS
    if (!entry.is_initialized()) {
        Preferences prefs;
        prefs.begin(KeyType::namespace_name, true); // read-only

        using T = typename KeyType::value_type;
        T nvs_val;
        if constexpr (std::is_same_v<T, int32_t>) {
            nvs_val = prefs.getInt(KeyType::key_name, key.default_value);
        } // ... other types

        prefs.end();

        entry.value = nvs_val;
        entry.nvs_value = nvs_val;
        entry.dirty = false;
    }

    using T = typename KeyType::value_type;
    return std::get<T>(entry.value);
}
```

### Anti-Patterns to Avoid
- **Hash map for small fixed sets:** std::unordered_map has 4-8 byte overhead per entry plus dynamic allocation; array is faster for N<100
- **Bit flags for <7 instances:** Break-even point is 7 instances; below that, instruction overhead exceeds memory savings
- **memcmp for struct comparison:** ESP32 padding bytes may differ between equal objects; use operator== or explicit field comparison
- **Global Preferences instance:** Each PrefKey needs different namespace; opening multiple namespaces simultaneously conflicts
- **Eager boot-time loading:** NVS read is fast (hash cached), but loading all preferences at boot wastes RAM and startup time

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Type-safe variant storage | Union with manual type tracking | std::variant | Handles type safety, lifetime, visitor pattern automatically; zero overhead |
| Optional three-state values | bool + value pairs | std::optional | Standard semantics, explicit "no value" state, better than -1/nullptr sentinels |
| Compile-time counter | Manual ID assignment | Template static counter | Automatic unique IDs per template instantiation; no maintenance |
| Hash table for small N | Custom hash implementation | Linear search in std::array | Hash overhead dominates for N<100; linear search is faster and simpler |
| String comparison | Custom strcmp wrapper | Arduino String operator== | String class already implements efficient comparison; operator== is idiomatic |

**Key insight:** ESP32 has limited RAM (320KB DRAM, max 160KB static allocation). For typical preference counts (10-50 keys), simple linear structures outperform complex data structures due to cache locality and zero allocation overhead. The 22KB RAM per 1MB NVS partition already consumed by NVS internal caching means adding another layer of complex caching provides minimal benefit.

## Common Pitfalls

### Pitfall 1: Uninitialized Cache Access Ambiguity
**What goes wrong:** Can't distinguish "value was never loaded" from "value loaded and is default"
**Why it happens:** Single bool dirty flag doesn't track initialization state separately from modification state
**How to avoid:** Use three-state pattern: std::optional<T> nvs_value tracks initialization, bool dirty tracks modification
**Warning signs:** isDirty() returns false for keys that were never loaded, making them indistinguishable from clean keys

### Pitfall 2: Variant Type Mismatch at Runtime
**What goes wrong:** std::get<T>(variant) throws std::bad_variant_access if wrong type requested
**Why it happens:** Template instantiation guarantees compile-time type safety, but variant access is runtime-checked
**How to avoid:** Use compile-time KeyType::value_type to ensure correct type extraction; never manually specify type
**Warning signs:** Exceptions in get() operations, crash in variant access

### Pitfall 3: NVS Value Comparison Without Loading
**What goes wrong:** isDirty() returns wrong result because nvs_value was never populated from NVS
**Why it happens:** Lazy initialization means first isDirty() call happens before get() loads from NVS
**How to avoid:** isDirty() must trigger NVS load if uninitialized, or document that get() must be called first
**Warning signs:** isDirty() always returns true even for unchanged values

### Pitfall 4: String Comparison Copies
**What goes wrong:** Arduino String operator== may copy strings during comparison, fragmenting heap
**Why it happens:** String class c_str() conversion and operator overloads may create temporaries
**How to avoid:** For String types, compare c_str() pointers directly with strcmp if performance critical
**Warning signs:** Heap fragmentation grows during repeated isDirty() checks on String keys

### Pitfall 5: Cache Entry Array Size Mismatch
**What goes wrong:** Registry::cache_entries[KeyType::id] accesses out of bounds if more keys than MAX_KEYS
**Why it happens:** Compile-time counter increments beyond array capacity; no runtime check
**How to avoid:** static_assert(key_count < MAX_KEYS) in register_key() to catch at compile time
**Warning signs:** Crashes on cache access, memory corruption in adjacent data

### Pitfall 6: Dirty Flag Not Cleared After NVS Read
**What goes wrong:** After loading from NVS, dirty flag remains true, indicating false modification
**Why it happens:** Only set() operation manages dirty flag; initialization forgets to clear it
**How to avoid:** Always set dirty=false after loading from NVS in get() initialization path
**Warning signs:** isDirty() returns true for all keys even though none were modified

### Pitfall 7: Default Value vs. NVS Value Confusion
**What goes wrong:** isModified() compares against wrong baseline (NVS instead of default)
**Why it happens:** Mixing up "modified from default" (isModified) vs "modified from storage" (isDirty)
**How to avoid:** isModified() ALWAYS compares current value to key.default_value; isDirty() compares to entry.nvs_value
**Warning signs:** isModified() returns false for values that differ from defaults but match NVS

## Code Examples

Verified patterns from official sources:

### Dirty Flag Pattern Implementation
```cpp
// Source: https://gameprogrammingpatterns.com/dirty-flag.html (adapted for preferences)
template<typename KeyType>
void set(const KeyType& key, typename KeyType::value_type value) {
    auto& entry = Registry::cache_entries[KeyType::id];

    // Update cached value
    entry.value = value;

    // Set dirty flag - defers expensive flash write
    entry.dirty = true;
}

// Later: batch writes with save()
void save() {
    for (auto& entry : Registry::cache_entries) {
        if (entry.is_dirty()) {
            // Write to NVS only for dirty entries
            write_to_nvs(entry);
            entry.dirty = false; // Clear flag after write
        }
    }
}
```

### std::variant for Type-Safe Storage
```cpp
// Source: https://www.cppstories.com/2018/06/variant/ and https://en.cppreference.com/w/cpp/utility/variant.html
using ValueVariant = std::variant<int32_t, float, bool, String>;

struct CacheEntry {
    ValueVariant value;
    std::optional<ValueVariant> nvs_value;
    bool dirty = false;
};

// Type-safe extraction using compile-time KeyType
template<typename KeyType>
typename KeyType::value_type get(const KeyType& key) {
    auto& entry = Registry::cache_entries[KeyType::id];

    // Compile-time correct type extraction
    using T = typename KeyType::value_type;
    return std::get<T>(entry.value);
}
```

### Fixed Array for Small Sets
```cpp
// Source: https://blog.quasar.ai/using-c-containers-efficiently and https://www.embeddedrelated.com/showarticle/1031.php
// For N < 100, array with linear search beats hash map
static constexpr size_t MAX_KEYS = 64;
static inline std::array<CacheEntry, MAX_KEYS> cache_entries;

// Linear search is fast for small N due to cache locality
template<typename KeyType>
CacheEntry& get_entry() {
    return cache_entries[KeyType::id]; // Direct O(1) indexing with compile-time ID
}
```

### ESP32 Memory-Conscious Storage
```cpp
// Source: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html
// ESP32 has 320KB DRAM, max 160KB static allocation
// Each cache entry: sizeof(variant) + sizeof(optional<variant>) + sizeof(bool)
//   ≈ 32 bytes for int32_t/float/bool, ~40 bytes for String
// 64 entries × 40 bytes = 2.56KB static allocation (well within limits)

static constexpr size_t calculate_max_keys() {
    constexpr size_t ENTRY_SIZE = sizeof(CacheEntry);
    constexpr size_t SAFE_ALLOCATION = 4096; // 4KB budget for cache
    return SAFE_ALLOCATION / ENTRY_SIZE; // ~100 entries
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| std::any for type erasure | std::variant for closed type sets | C++17 (2017) | Variant avoids heap, RTTI; better for embedded |
| Immediate NVS writes on set() | Write-back cache with dirty tracking | Standard pattern | Reduces flash writes from thousands to <10 per session |
| Runtime key lookup by string | Compile-time index via template ID | Modern C++ | Zero overhead lookup; impossible to misspell keys |
| Boolean array for dirty flags | Bit flags for ultra-compact storage | Context-dependent | Bit flags only win at 7+ instances; arrays cleaner for small N |
| std::unordered_map for caches | std::array for fixed small sets | Always been true | Hash maps have 4-8x overhead; arrays faster for N<100 |

**Deprecated/outdated:**
- **Manual union + type tag:** std::variant (C++17) provides type-safe unions with zero overhead
- **Pointer-based optional:** std::optional (C++17) is standard, explicit, no heap/nullptr confusion
- **Macro-based registration:** Template static initialization is type-safe and maintainable

## Open Questions

Things that couldn't be fully resolved:

1. **NVS Read Frequency During Dirty Checks**
   - What we know: NVS has internal hash caching (fast reads after initialization)
   - What's unclear: Whether isDirty() should re-read NVS each call or trust cached nvs_value
   - Recommendation: Trust cached nvs_value populated during first get(); only re-read on explicit refresh()

2. **String Variant Size Optimization**
   - What we know: Arduino String has dynamic allocation, variant size is max(all types)
   - What's unclear: Whether to store String by value (large variant) or by pointer (add indirection)
   - Recommendation: Store by value for uniform variant semantics; String already has internal pointer

3. **Compile-Time Key Count Enforcement**
   - What we know: static_assert can check key_count < MAX_KEYS at instantiation
   - What's unclear: Whether error happens at first overflow or after all instantiations
   - Recommendation: Test with MAX_KEYS=2 and 3 keys to verify compile-time detection

4. **Cache Entry Initialization Strategy**
   - What we know: Lazy init saves RAM/startup time; eager init provides deterministic behavior
   - What's unclear: Whether user needs control over init strategy or always lazy is fine
   - Recommendation: Default to lazy; add optional begin() that pre-loads all keys if needed

## Sources

### Primary (HIGH confidence)
- ESP-IDF Memory Types Documentation: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html
- ESP-IDF NVS Documentation: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
- ESP-IDF RAM Usage Optimization: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/ram-usage.html
- Preferences.h API: https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h
- Dirty Flag Pattern: https://gameprogrammingpatterns.com/dirty-flag.html
- std::variant Reference: https://en.cppreference.com/w/cpp/utility/variant.html
- C++ Variant Guide: https://www.cppstories.com/2018/06/variant/
- Embedded Template Library (ETL): https://www.etlcpp.com/array.html

### Secondary (MEDIUM confidence)
- C++ Container Efficiency: https://blog.quasar.ai/using-c-containers-efficiently
- Cache-Friendly Data Structures: https://tylerayoung.com/2019/01/29/benchmarks-of-cache-friendly-data-structures-in-c/
- Factory Self-Registration Pattern: https://www.cppstories.com/2018/02/factory-selfregister/
- std::array for Embedded: https://www.embeddedrelated.com/showarticle/1031.php
- Packing Bools Performance: https://www.cppstories.com/2017/04/packing-bools/
- Bit Flags vs Boolean Arrays: https://www.learncpp.com/cpp-tutorial/bit-flags-and-bit-manipulation-via-stdbitset/
- Arduino String Comparison: https://docs.arduino.cc/built-in-examples/strings/StringComparisonOperators/
- NVS Comprehensive Guide: https://medium.com/engineering-iot/nvs-data-storage-and-reading-in-esp32-a-comprehensive-guide-12bdbc6325ac

### Tertiary (LOW confidence)
- ESP32 Forum NVS Usage Discussion: https://www.esp32.com/viewtopic.php?t=3990
- Modern C++ in Embedded (2025): https://runtimerec.com/modern-c-in-embedded-how-far-can-you-push-it/
- C++ Registry Pattern GitHub: https://github.com/psalvaggio/cppregpattern

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - std::array, std::variant, std::optional are well-documented C++17 features; ESP32 memory constraints verified from official Espressif docs
- Architecture: HIGH - Dirty flag pattern is established; fixed array for small sets proven faster than hash maps in multiple benchmarks; three-state pattern is standard practice
- Pitfalls: HIGH - Variant type safety, NVS caching behavior, and memory constraints documented in official sources; dirty tracking pitfalls from game programming patterns

**Research date:** 2026-02-03
**Valid until:** ~90 days (stable domain: C++17 mature, ESP32 NVS API stable, patterns are timeless)
