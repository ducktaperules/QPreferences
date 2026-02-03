# Phase 4: Iteration & Examples - Research

**Researched:** 2026-02-03
**Domain:** C++ iteration patterns for embedded systems, ESP32 NVS API, Arduino library conventions
**Confidence:** HIGH

## Summary

This phase implements iteration over registered preferences and creates example sketches demonstrating the library. The existing codebase already has the infrastructure needed for iteration: `KeyMetadata` parallel array stores namespace and key names for each registered key, and `cache_entries` stores current values. Iteration simply exposes these internal arrays through a clean API.

The ESP32 Preferences library provides `clear()` to delete all keys in a namespace, which enables factory reset functionality. However, there is no native method to enumerate keys in NVS - iteration must use our internal registry (which is sufficient since we track all keys ourselves).

Arduino library examples must follow strict conventions: lowercase `examples/` folder, each example in its own subfolder with matching `.ino` filename. The existing `BasicUsage` example follows this pattern and can be extended.

**Primary recommendation:** Implement foreach-style iteration using callbacks over the existing `key_metadata[]` and `cache_entries[]` arrays, with namespace filtering via string comparison. Use ESP32's `Preferences::clear()` for factory reset per namespace.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ESP32 Preferences | Built-in | NVS storage, clear() for reset | Official ESP32 Arduino library |
| std::array | C++17 | Fixed-size storage | Already used, no heap allocation |
| std::function | C++17 | Callback storage | Type-safe callbacks with lambda support |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cstring | Standard | strcmp for namespace filtering | Compare namespace names |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| std::function callback | Function pointer | Less flexible but smaller footprint; std::function preferred for lambda support |
| Custom iterator class | Callback-based forEach | Iterator class adds complexity without benefit for simple enumeration |
| ESP-IDF nvs_entry_find | Our internal registry | NVS API would enumerate all NVS, not just QPreferences keys; internal registry is cleaner |

**Installation:**
No additional libraries needed. All dependencies are already available.

## Architecture Patterns

### Recommended Project Structure
```
src/QPreferences/
├── QPreferences.h    # Add forEach(), forEachInNamespace(), factoryReset()
├── CacheEntry.h      # Add PrefInfo struct for iteration callback
├── PrefKey.h         # (unchanged)
└── StringLiteral.h   # (unchanged)

examples/
├── BasicUsage/       # (existing - needs save() demo)
├── DirtyTracking/    # NEW: demonstrates isDirty/isModified + save()
└── NamespaceGroups/  # NEW: demonstrates namespace filtering + factory reset
```

### Pattern 1: Callback-Based Iteration
**What:** Use callbacks (std::function or function pointer) to iterate over entries
**When to use:** When user needs to process each registered preference
**Example:**
```cpp
// Source: Embedded systems callback pattern - industry standard

// PrefInfo struct to pass to callbacks
struct PrefInfo {
    const char* namespace_name;
    const char* key_name;
    size_t index;                    // Index into cache_entries
    bool is_initialized;
    bool is_dirty;
};

// forEach using std::function for lambda support
template<typename Callback>
void forEach(Callback callback) {
    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];
        auto& entry = QPreferences::cache_entries[i];

        PrefInfo info{
            meta.namespace_name,
            meta.key_name,
            i,
            entry.is_initialized(),
            entry.is_dirty()
        };
        callback(info);
    }
}
```

### Pattern 2: Namespace-Filtered Iteration
**What:** Filter forEach to only process keys in a specific namespace
**When to use:** When user wants to inspect/reset keys from a single component
**Example:**
```cpp
// Source: Extension of callback pattern with filter predicate

template<typename Callback>
void forEachInNamespace(const char* ns, Callback callback) {
    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];
        if (std::strcmp(meta.namespace_name, ns) == 0) {
            auto& entry = QPreferences::cache_entries[i];
            PrefInfo info{
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
```

### Pattern 3: Factory Reset via clear()
**What:** Clear all NVS entries and restore RAM to defaults
**When to use:** When user wants to restore device to original state
**Example:**
```cpp
// Source: ESP32 Preferences.clear() - official API

inline void factoryReset() {
    // Collect unique namespaces
    // Clear each namespace in NVS
    // Reset cache entries to uninitialized state

    Preferences prefs;
    const char* last_ns = nullptr;

    for (size_t i = 0; i < QPreferences::next_key_id; ++i) {
        auto& meta = QPreferences::key_metadata[i];

        // Clear namespace if different from last
        if (last_ns == nullptr || std::strcmp(last_ns, meta.namespace_name) != 0) {
            if (last_ns != nullptr) prefs.end();
            prefs.begin(meta.namespace_name, false);
            prefs.clear();  // Deletes all keys in namespace
            last_ns = meta.namespace_name;
        }

        // Reset cache entry
        auto& entry = QPreferences::cache_entries[i];
        entry.nvs_value.reset();
        entry.dirty = false;
    }

    if (last_ns != nullptr) prefs.end();
}
```

### Anti-Patterns to Avoid
- **Using NVS iterator API to enumerate:** The ESP-IDF `nvs_entry_find` API enumerates ALL NVS entries, not just QPreferences. Using our internal registry is cleaner and more reliable.
- **Returning std::vector from iteration:** Heap allocation is problematic on ESP32. Use callbacks instead.
- **Exposing raw cache_entries array:** Leaks implementation details. Expose through typed PrefInfo struct.
- **Template-based value access in forEach:** Without template context, we cannot know the type. Return index/metadata only; user accesses value via get(key).

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Clear all keys in namespace | Manual key removal loop | Preferences::clear() | Official API, handles internals correctly |
| Unique namespace collection | std::set or manual dedup | Iterate and track last_ns | Simpler, no heap allocation needed |
| Callback type | Raw function pointers | Template parameter (auto-deduces) | Supports lambdas, std::function, function pointers |

**Key insight:** The ESP32 Preferences library provides `clear()` which efficiently removes all keys in a namespace. Don't iterate and remove keys individually - use the batch operation.

## Common Pitfalls

### Pitfall 1: Accessing Value Type in forEach
**What goes wrong:** Trying to return current value from forEach callback
**Why it happens:** Without template context, we don't know the value type stored in std::variant
**How to avoid:** Return metadata only (namespace, key name, index, status). User uses get(key) for values.
**Warning signs:** Attempting to std::get<T> without knowing T

### Pitfall 2: Heap Allocation in Iteration
**What goes wrong:** Collecting results into std::vector causes heap fragmentation
**Why it happens:** ESP32 has limited heap, frequent allocations fragment memory
**How to avoid:** Use callback pattern - process each entry inline, no collection needed
**Warning signs:** Using new, std::vector, std::string in iteration loop

### Pitfall 3: Incomplete Factory Reset
**What goes wrong:** Only clearing NVS but not resetting cache state
**Why it happens:** Forgetting that cache_entries still holds old values
**How to avoid:** Reset cache_entries[i].nvs_value to empty (uninitialized state) after clearing NVS
**Warning signs:** isDirty() returns unexpected values after factory reset

### Pitfall 4: Modifying During Iteration
**What goes wrong:** Calling set() or save() during forEach iteration
**Why it happens:** User tries to update values while iterating
**How to avoid:** Document that forEach is for inspection only; modifications during iteration are undefined
**Warning signs:** Loop counter issues, skipped entries

### Pitfall 5: Example Naming Convention
**What goes wrong:** Examples not appearing in Arduino IDE menu
**Why it happens:** Folder or file naming doesn't match Arduino spec
**How to avoid:** Folder must be lowercase `examples/`, each example in subfolder with matching `.ino` name
**Warning signs:** Examples visible in filesystem but not in IDE

## Code Examples

Verified patterns from official sources:

### Preferences::clear() for Factory Reset
```cpp
// Source: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html
// clear() deletes all keys and values from the currently opened namespace

Preferences prefs;
prefs.begin("myapp", false);  // false = read-write
prefs.clear();                 // Deletes ALL keys in "myapp" namespace
prefs.end();
```

### Callback Iteration Template
```cpp
// Source: Embedded systems pattern - allows lambdas, function pointers, functors

template<typename Callback>
void forEach(Callback callback) {
    for (size_t i = 0; i < count; ++i) {
        callback(entries[i]);
    }
}

// Usage with lambda:
forEach([](const PrefInfo& info) {
    Serial.printf("Key: %s/%s\n", info.namespace_name, info.key_name);
});

// Usage with function pointer:
void printKey(const PrefInfo& info) {
    Serial.printf("Key: %s/%s\n", info.namespace_name, info.key_name);
}
forEach(printKey);
```

### Arduino Example Structure
```
// Source: https://arduino.github.io/arduino-cli/0.20/library-specification/
// Each example must be in its own folder with matching .ino filename

examples/
├── DirtyTracking/
│   └── DirtyTracking.ino      // Name matches folder
└── NamespaceGroups/
    └── NamespaceGroups.ino    // Name matches folder
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| ESP-IDF nvs_entry_find | Internal registry iteration | N/A (library design choice) | Cleaner, only shows QPreferences keys |
| std::function storage | Template parameter deduction | C++17 | Zero overhead for simple callbacks |
| Manual key tracking | KeyMetadata parallel array | Phase 3 (03-01) | Infrastructure already exists |

**Deprecated/outdated:**
- None for this phase - building on existing patterns

## Open Questions

Things that couldn't be fully resolved:

1. **Should forEach return count of processed entries?**
   - What we know: Most callback-based APIs return void
   - What's unclear: Whether count would be useful for debugging
   - Recommendation: Start with void return; add count if needed later

2. **Should PrefInfo include value as std::variant?**
   - What we know: std::variant loses type information
   - What's unclear: Whether runtime type access is needed
   - Recommendation: Don't include value; users call get(key) with their typed key. If needed, add getType() returning enum.

3. **Should factoryReset take optional namespace filter?**
   - What we know: Current requirement is full reset
   - What's unclear: Whether partial reset would be useful
   - Recommendation: Implement full factoryReset(); add resetNamespace(ns) if needed

## Sources

### Primary (HIGH confidence)
- [ESP32 Preferences API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html) - clear(), remove() methods verified
- [Arduino Library Specification](https://arduino.github.io/arduino-cli/0.20/library-specification/) - examples folder structure
- [ESP-IDF NVS Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html) - NVS iterator API (for reference)
- [arduino-esp32 Preferences.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h) - Full API reference

### Secondary (MEDIUM confidence)
- [Embedded Artistry Callbacks](https://embeddedartistry.com/blog/2017/02/01/improving-your-callback-game/) - Callback patterns for embedded
- [Stratify Labs Callbacks](https://blog.stratifylabs.dev/device/2022-12-01-Callback-and-Lambdas-in-embedded-cpp/) - Lambda patterns in embedded C++

### Tertiary (LOW confidence)
- Community forum discussions on NVS iteration - Confirms lack of native key enumeration in Preferences

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Using existing ESP32 Preferences API and standard C++ patterns
- Architecture: HIGH - Building on existing KeyMetadata infrastructure from Phase 3
- Pitfalls: HIGH - Well-documented embedded systems patterns, verified ESP32 behavior

**Research date:** 2026-02-03
**Valid until:** 60 days (stable embedded patterns, unlikely to change)
