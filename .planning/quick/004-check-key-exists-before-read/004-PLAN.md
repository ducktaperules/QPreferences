---
phase: quick
plan: 004
type: execute
wave: 1
depends_on: []
files_modified:
  - src/QPreferences/QPreferences.h
autonomous: true

must_haves:
  truths:
    - "Reading non-existent float key does not log ESP32 Preferences errors"
    - "Reading non-existent String key does not log ESP32 Preferences errors"
    - "nvs_value remains empty when key doesn't exist in NVS"
    - "value contains default when key doesn't exist in NVS"
  artifacts:
    - path: "src/QPreferences/QPreferences.h"
      provides: "get() function with isKey() check before reading"
      contains: "prefs.isKey"
  key_links:
    - from: "get() function"
      to: "prefs.isKey()"
      via: "conditional before read operations"
      pattern: "isKey.*key_name"
---

<objective>
Add key existence check before reading from NVS to prevent ESP32 Preferences error logging.

Purpose: ESP32's Preferences library logs errors when reading non-existent keys via getFloat() (blob storage) and getString() (nvs_get_str). Using prefs.isKey() before reading eliminates these errors.

Output: Updated get() function that checks key existence and only reads if key is present.
</objective>

<execution_context>
@C:\Users\duckt\.claude/get-shit-done/workflows/execute-plan.md
@C:\Users\duckt\.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@src/QPreferences/QPreferences.h
</context>

<tasks>

<task type="auto">
  <name>Task 1: Add isKey() check before NVS reads in get()</name>
  <files>src/QPreferences/QPreferences.h</files>
  <action>
In the get() function (lines 65-88), update the "namespace exists" branch to check if the key exists before reading.

Current code (lines 65-88):
```cpp
} else {
    // Namespace exists - read from NVS
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
        static_assert(sizeof(T) == 0, "Unsupported type...");
    }

    prefs.end();

    // Store in both current value and NVS value
    entry.value = nvs_result;
    entry.nvs_value = nvs_result;
    entry.initialized = true;
    entry.dirty = false;
}
```

Replace with:
```cpp
} else {
    // Namespace exists - check if key exists before reading
    if (prefs.isKey(KeyType::key_name)) {
        // Key exists in NVS - read it
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

        entry.value = nvs_result;
        entry.nvs_value = nvs_result;  // Key exists in NVS
    } else {
        // Key doesn't exist in this namespace - use default
        entry.value = key.default_value;
        // Leave nvs_value empty - nothing in NVS for this key
    }

    prefs.end();
    entry.initialized = true;
    entry.dirty = false;
}
```

Key semantic change: When namespace exists but key doesn't:
- entry.value = default (same as before)
- entry.nvs_value = empty (std::nullopt) - because key isn't in NVS
- This aligns with fresh device behavior: dirty tracking compares against default_value
  </action>
  <verify>
Compilation check:
```bash
cd examples/basic_usage && pio run
```
No compilation errors.
  </verify>
  <done>
- get() uses prefs.isKey() before calling getFloat/getString/etc.
- No ESP32 error logging for missing keys
- nvs_value remains empty when key doesn't exist (dirty tracks against default)
  </done>
</task>

</tasks>

<verification>
- Code compiles without errors
- isKey() call appears before the type-specific read operations
- Both branches (key exists, key missing) set entry.initialized and entry.dirty correctly
- prefs.end() is called in both cases (outside the if/else)
</verification>

<success_criteria>
- No ESP32 Preferences errors logged when reading non-existent float or String keys
- Existing functionality preserved (values returned correctly, dirty tracking works)
- Code compiles successfully
</success_criteria>

<output>
After completion, create `.planning/quick/004-check-key-exists-before-read/004-SUMMARY.md`
</output>
