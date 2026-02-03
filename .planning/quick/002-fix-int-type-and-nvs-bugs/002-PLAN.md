---
quick: 002
type: execute
description: Fix int type support and NVS namespace creation bugs
files_modified:
  - src/QPreferences/CacheEntry.h
  - src/QPreferences/QPreferences.h
autonomous: true
---

<objective>
Fix two bugs discovered during user testing:
1. `int` type not properly supported in ValueVariant (only `int32_t`)
2. NVS `begin()` fails with NOT_FOUND when namespace doesn't exist on first run

Purpose: Make the library work correctly on fresh ESP32 devices and with standard `int` declarations.
Output: Both source files patched so tests pass on fresh NVS.
</objective>

<context>
@.planning/STATE.md
@src/QPreferences/CacheEntry.h
@src/QPreferences/QPreferences.h
</context>

<tasks>

<task type="auto">
  <name>Task 1: Add int to ValueVariant</name>
  <files>src/QPreferences/CacheEntry.h</files>
  <action>
In CacheEntry.h line 17, the ValueVariant only includes `int32_t`:
```cpp
using ValueVariant = std::variant<int32_t, float, bool, String>;
```

Add `int` to the variant so both `int` and `int32_t` are supported:
```cpp
using ValueVariant = std::variant<int, int32_t, float, bool, String>;
```

This allows users to declare `PrefKey<int, ...>` and have the type stored correctly in the variant. The `int` and `int32_t` are distinct types in the variant even if they're the same size on ESP32.

Update the doc comment on lines 12-15 to mention both `int` and `int32_t`.
  </action>
  <verify>File compiles: `grep "std::variant<int, int32_t" src/QPreferences/CacheEntry.h` returns the updated line</verify>
  <done>ValueVariant includes both `int` and `int32_t` types</done>
</task>

<task type="auto">
  <name>Task 2: Fix NVS namespace NOT_FOUND error</name>
  <files>src/QPreferences/QPreferences.h</files>
  <action>
The bug: In get() on line 56, `prefs.begin(KeyType::namespace_name, true)` opens read-only. When the namespace doesn't exist (first boot), ESP32 Preferences returns NOT_FOUND and the `begin()` fails. The code ignores the failure and tries to read anyway, getting defaults.

Fix approach: Open read-write instead of read-only. The Preferences library creates the namespace if it doesn't exist. Since we immediately call `end()` after reading, this is safe and doesn't actually write anything.

In the get() function around line 56, change:
```cpp
prefs.begin(KeyType::namespace_name, true);  // true = read-only
```
to:
```cpp
prefs.begin(KeyType::namespace_name, false);  // false = read-write (creates namespace if needed)
```

Update the comment to explain why read-write is used:
```cpp
// false = read-write mode creates namespace if it doesn't exist
// (read-only mode fails with NOT_FOUND on first boot)
```
  </action>
  <verify>Grep for the fix: `grep -A1 "prefs.begin.*namespace_name" src/QPreferences/QPreferences.h` shows `false` in the get() function</verify>
  <done>get() opens NVS in read-write mode so namespaces are created on first access</done>
</task>

</tasks>

<verification>
1. Both files modified without syntax errors
2. `grep "std::variant<int, int32_t" src/QPreferences/CacheEntry.h` returns match
3. `grep -n "prefs.begin" src/QPreferences/QPreferences.h` shows all begin() calls use `false` for read-write mode
</verification>

<success_criteria>
- ValueVariant supports both `int` and `int32_t` types
- get() opens NVS in read-write mode to auto-create namespaces
- Library works on fresh ESP32 without NVS errors
- Users can declare `PrefKey<int, ...>` without issues
</success_criteria>

<output>
After completion, create `.planning/quick/002-fix-int-type-and-nvs-bugs/002-SUMMARY.md`
</output>
