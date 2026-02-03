# Feature Landscape

**Domain:** ESP32 Preferences/Settings Management Libraries
**Researched:** 2026-02-03

## Table Stakes

Features users expect. Missing = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Key-value storage for primitives | Core requirement for preferences - users expect to store int, float, bool, string | Low | Preferences.h supports: char, Uchar, short, Ushort, int, Uint, long, Ulong, long64, Ulong64, float, double, bool, string, bytes |
| Namespace organization | Prevents key collisions between modules/libraries. Standard practice in ESP32 ecosystem | Low | Namespace names limited to 15 chars max, 254 namespaces per partition |
| Persistent storage (NVS-backed) | Data survives power loss and resets. Fundamental requirement for "preferences" | Low | All ESP32 preference libraries wrap NVS flash storage |
| Basic CRUD operations | Users need to create, read, update, delete preferences | Low | putX(), getX(), remove(), clear() methods |
| Default values on read | Prevents crashes when reading non-existent keys | Low | All getX() methods support optional default parameter |
| Type-specific get/put methods | Users expect putInt(), getInt(), etc. to prevent type errors | Low | Standard pattern: users must track types manually across writes/reads |

## Differentiators

Features that set product apart. Not expected, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Type-safe preference definitions | Single source of truth for key name, type, namespace, and default. Prevents type mismatch bugs | Medium | QPreferences' PrefKey<T> template approach. Standard Preferences.h requires manual type tracking |
| Unified get/set API regardless of type | Simpler API - don't need to remember putInt vs putFloat | Medium | Enables template-based access: `prefs.get(myKey)` instead of `prefs.getInt("key")` |
| RAM-first with explicit save() | Batches NVS writes, reduces wear leveling cycles, improves performance | Medium | Standard Preferences writes immediately to NVS on every put. Critical for high-write scenarios |
| Smart storage (omit defaults from NVS) | Saves NVS space, reduces wear on unchanged values | Medium | Most libraries store everything. Smart approach: only persist changed values |
| Dirty tracking | Know what changed without manual bookkeeping | Low-Medium | Enables selective saving, change notifications, debugging |
| Iteration over all preferences | Enumerate registered preferences for UI generation, debugging, backup | Medium | Standard Preferences.h lacks key iteration (requested feature since 2018). ESP-IDF NVS has nvs_entry_find() but Arduino wrapper doesn't expose it |
| JSON serialization/deserialization | Export/import all settings as JSON for backup, web UI, migration | Medium-High | Requires integration with ArduinoJson or RapidJSON. esp-config-state library demonstrates this pattern |
| Factory reset with defaults | One-call restoration to known-good state | Low | Most libraries require manual deletion + re-initialization |
| Namespace-aware factory reset | Reset specific module settings without affecting others | Low-Medium | Useful for multi-module systems |
| Validation constraints (min/max/enum) | Prevent invalid values from being stored | Medium | Not provided by standard libraries - must be implemented manually |
| Change callbacks/observers | React to preference changes (e.g., update display, reconfigure hardware) | Medium | Enables reactive programming patterns |
| Preference grouping/categories | Organize related preferences for UI presentation | Low | Metadata for human organization, not technical requirement |
| Migration support | Handle preference schema changes across firmware versions | High | Critical for production devices. Usually requires manual version tracking |
| Atomic multi-preference updates | All-or-nothing updates to prevent partial state | Medium-High | Important for related preferences (e.g., WiFi SSID + password) |
| Size optimization reporting | Show NVS usage, warn before partition full | Low-Medium | freeEntries() exists but not integrated into high-level APIs |
| Cross-platform abstraction | Same API works on ESP8266, RP2040, etc. | Medium | vshymanskyy/Preferences library demonstrates this |

## Anti-Features

Features to explicitly NOT build. Common mistakes in this domain.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Frequent auto-save on every change | Wears out flash rapidly. NVS has ~100K write cycles per sector. Writing once/second fills 16KB partition in <1 year | RAM-first with explicit save(). Batch writes. Let users control persistence timing |
| Storing large/binary blobs in NVS | NVS optimized for many small values, not large blobs. Performance degrades, space inefficient | Use SPIFFS/LittleFS for files. Keep preferences small (<100 bytes per key) |
| Single global namespace | Key collisions between modules. Hard to reset specific feature settings | Always require namespace parameter. Document namespace conventions |
| Immediate NVS writes without dirty tracking | Unnecessary writes wear flash even when value unchanged | Track dirty state. Skip writes if value identical to current |
| String-based type selection (setInt/getString caller decides) | Type mismatches cause runtime bugs. Easy to forget what type was stored | Type-safe definitions. Compiler enforces correct type usage |
| Built-in WiFi credential storage | Security risk - credentials in plain NVS. Outside library scope | Let users decide encryption approach. Provide example patterns |
| Synchronous blocking writes | NVS writes can take milliseconds, blocking other tasks | Acceptable for preferences (infrequent). Async would add complexity without much benefit for this use case |
| Complex query/filtering API | Preferences are key-value, not a database | Keep API simple. Advanced queries belong in application layer |
| Automatic background persistence | Unpredictable flash writes. Hard to debug. Violates principle of explicit control | Explicit save() method. Optionally: auto-save on timer with clear documentation |

## Feature Dependencies

```
Core Storage (NVS-backed persistence)
  ├─→ Key-Value CRUD
  │     ├─→ Type-specific get/put
  │     ├─→ Default values
  │     └─→ Namespace organization
  │
  ├─→ Type-Safe Definitions (PrefKey<T>)
  │     ├─→ Unified get/set API
  │     └─→ Compile-time type checking
  │
  ├─→ RAM-first Architecture
  │     ├─→ Dirty Tracking (required for smart save)
  │     ├─→ Explicit save()
  │     └─→ Smart Storage (omit defaults)
  │
  └─→ Advanced Features
        ├─→ Iteration (requires registry of PrefKey instances)
        ├─→ JSON Serialization (requires iteration + type info)
        ├─→ Validation (requires constraint metadata in PrefKey)
        └─→ Change Callbacks (requires observer registry)
```

## MVP Recommendation

For MVP (QPreferences v0.1), prioritize:

1. **Type-safe PrefKey<T> definitions** (core differentiator)
   - Template class with namespace, key, default value
   - Compile-time type safety

2. **Unified get/set API** (simplicity win)
   - `prefs.get(myKey)` returns T
   - `prefs.set(myKey, value)` accepts T

3. **RAM-first with explicit save()** (performance + wear leveling)
   - Changes held in RAM
   - `save()` writes dirty values to NVS

4. **Dirty tracking** (enables smart save)
   - Track which preferences changed
   - Only write dirty values on save()

5. **Smart storage** (space efficiency)
   - Don't write values that match defaults
   - `remove()` when value set back to default

6. **Basic iteration** (debugging, export)
   - Iterate over registered PrefKey instances
   - Not full NVS key enumeration (deferred)

Defer to post-MVP:

- **JSON serialization**: Useful but adds ArduinoJson dependency. Can be added incrementally
- **Validation constraints**: Nice-to-have, but users can validate before set()
- **Change callbacks**: Complex, limited use cases for MVP
- **Migration support**: Needed for production, not for initial library validation
- **Atomic updates**: Advanced feature, requires transaction semantics
- **Cross-platform support**: Focus on ESP32 first, port later if demand exists

## Competitive Analysis

| Feature | Preferences.h (Arduino-ESP32) | ArduinoNvs | esp-config-state | QPreferences (Proposed) |
|---------|-------------------------------|------------|------------------|------------------------|
| Type-specific API | Yes (putInt, getString, etc.) | Yes | Yes | Unified via templates |
| Type safety | Manual tracking required | Manual tracking | Manual tracking | Compile-time (PrefKey<T>) |
| Namespace support | Yes | Yes | Yes | Yes (in PrefKey) |
| Default values | Yes (getX default param) | Yes | Partial | Yes (in PrefKey definition) |
| Dirty tracking | No | No | No | Yes |
| RAM-first | No (immediate NVS write) | No | No | Yes |
| Smart storage | No (stores everything) | No | No | Yes (omit defaults) |
| Key iteration | No (requested since 2018) | Limited | Unknown | Yes (registered keys) |
| JSON export | No | No | Yes (with RapidJSON) | Planned post-MVP |
| Validation | No | No | No | Planned post-MVP |
| Complexity | Low | Low-Medium | Medium | Medium |

## Usage Patterns from Ecosystem

Based on community usage, preferences libraries are commonly used for:

1. **WiFi credentials** - SSID, password, connection settings
2. **Application state** - Last mode, active profile, user selections
3. **Calibration data** - Sensor offsets, correction factors
4. **Feature flags** - Enable/disable optional features
5. **Configuration values** - Thresholds, timeouts, limits
6. **Statistics/counters** - Boot count, runtime hours, error counts
7. **UI preferences** - Theme, language, display settings

Anti-patterns observed:

- Storing log data (use filesystem instead)
- Storing large binary files (use SPIFFS/LittleFS)
- Writing on every loop iteration (causes premature flash wear)
- Storing >100 key-value pairs in single namespace (performance degrades)

## Sources

- [Preferences - Arduino ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html) - Official API reference (MEDIUM confidence - WebSearch verified)
- [ESP32 Save Data Permanently - Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/) - Common usage patterns (MEDIUM confidence)
- [arduino-esp32/Preferences.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h) - Source code (HIGH confidence - official repository)
- [vshymanskyy/Preferences](https://github.com/vshymanskyy/Preferences) - Cross-platform implementation (MEDIUM confidence)
- [esp-config-state](https://github.com/mdvorak-iot/esp-config-state) - JSON serialization approach (MEDIUM confidence)
- [ArduinoNvs](https://github.com/rpolitex/ArduinoNvs) - Alternative NVS wrapper (MEDIUM confidence)
- [NVS and wear leveling discussion](https://www.esp32.com/viewtopic.php?t=3380) - Flash wear concerns (LOW confidence - forum discussion)
- [Key iteration feature request](https://github.com/espressif/arduino-esp32/issues/2179) - Confirmed missing feature (HIGH confidence - official issue tracker)
- [Non-Volatile Storage Library - ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html) - Underlying NVS implementation (HIGH confidence - official documentation)
- [ESP32 NVS Common Mistakes](https://www.esp32.com/viewtopic.php?t=3990) - Write frequency pitfalls (LOW-MEDIUM confidence - community discussion)
