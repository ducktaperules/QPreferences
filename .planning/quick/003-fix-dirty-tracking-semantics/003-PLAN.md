---
phase: quick
plan: 003
type: execute
wave: 1
depends_on: []
files_modified:
  - src/QPreferences/CacheEntry.h
  - src/QPreferences/QPreferences.h
autonomous: true

must_haves:
  truths:
    - "get() uses read-only NVS mode and checks return value"
    - "Fresh device get() returns default with nvs_value empty"
    - "set(key, value) only marks dirty when value differs from baseline"
    - "set(key, default) on fresh device marks dirty = false"
    - "set(key, nvsValue) on device with NVS value marks dirty = false"
  artifacts:
    - path: "src/QPreferences/CacheEntry.h"
      provides: "Separate initialized flag from nvs_value"
      contains: "bool initialized = false"
    - path: "src/QPreferences/QPreferences.h"
      provides: "Smart dirty tracking and read-only NVS"
      contains: "prefs.begin(KeyType::namespace_name, true)"
  key_links:
    - from: "CacheEntry.initialized"
      to: "is_initialized()"
      via: "return initialized instead of nvs_value.has_value()"
    - from: "set()"
      to: "entry.dirty"
      via: "comparison against nvs_value or default_value"
---

<objective>
Fix dirty tracking semantics - proper NVS read-only mode and smart dirty comparison

Purpose: Quick task 002 "fixed" NVS NOT_FOUND by opening in read-write mode for every get(), which is wasteful and semantically wrong. The proper fix keeps read-only mode, adds a separate initialized flag, and computes dirty intelligently by comparing against the right baseline.

Output: CacheEntry with separate initialized flag, get() using read-only mode with return check, set() with smart dirty comparison
</objective>

<execution_context>
@C:\Users\duckt\.claude/get-shit-done/workflows/execute-plan.md
@C:\Users\duckt\.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@src/QPreferences/CacheEntry.h
@src/QPreferences/QPreferences.h
</context>

<tasks>

<task type="auto">
  <name>Task 1: Add initialized flag to CacheEntry</name>
  <files>src/QPreferences/CacheEntry.h</files>
  <action>
Add a separate `bool initialized = false;` field to CacheEntry struct.

Modify `is_initialized()` to return `initialized` instead of `nvs_value.has_value()`.

This separates two concepts:
- `initialized`: Have we attempted to load this key from NVS? (controls lazy loading)
- `nvs_value.has_value()`: Does NVS actually have a stored value for this key?

Update the struct documentation to clarify the distinction.
  </action>
  <verify>Grep for "bool initialized" in CacheEntry.h, verify is_initialized returns initialized</verify>
  <done>CacheEntry has separate initialized flag, is_initialized() returns it</done>
</task>

<task type="auto">
  <name>Task 2: Fix get() to use read-only mode with return check</name>
  <files>src/QPreferences/QPreferences.h</files>
  <action>
Modify get() function:

1. Change `prefs.begin(KeyType::namespace_name, false)` to `prefs.begin(KeyType::namespace_name, true)` (read-only mode)

2. Capture return value: `bool opened = prefs.begin(..., true)`

3. If `!opened` (namespace doesn't exist on fresh device):
   - Set `entry.value = key.default_value`
   - Leave `entry.nvs_value` empty (do NOT set it)
   - Set `entry.initialized = true`
   - Skip the NVS read entirely

4. If `opened` (namespace exists):
   - Read from NVS as before
   - Set both `entry.value` and `entry.nvs_value` to the NVS result
   - Set `entry.initialized = true`

5. Call `prefs.end()` only if `opened` was true

Update the comment explaining the logic.
  </action>
  <verify>Grep for "prefs.begin.*true" in get() function</verify>
  <done>get() uses read-only mode, checks return value, handles fresh device correctly</done>
</task>

<task type="auto">
  <name>Task 3: Fix set() to use smart dirty comparison</name>
  <files>src/QPreferences/QPreferences.h</files>
  <action>
Modify set() function to compute dirty intelligently:

After `entry.value = value;`, replace `entry.dirty = true;` with:

```cpp
// Smart dirty comparison: compare against NVS value if exists, else default
if (entry.nvs_value.has_value()) {
    entry.dirty = (value != std::get<T>(entry.nvs_value.value()));
} else {
    entry.dirty = (value != key.default_value);
}
```

This ensures:
- Fresh device, set(key, default) -> dirty = false (nothing to save)
- Fresh device, set(key, nonDefault) -> dirty = true (needs save)
- Device with NVS, set(key, nvsValue) -> dirty = false (no change)
- Device with NVS, set(key, different) -> dirty = true (needs save)

Update the function documentation to explain the smart comparison.
  </action>
  <verify>Grep for "nvs_value.has_value" in set() function</verify>
  <done>set() computes dirty by comparing against correct baseline (NVS value or default)</done>
</task>

</tasks>

<verification>
1. All code compiles: `pio check --skip-packages`
2. CacheEntry has `bool initialized = false` field
3. `is_initialized()` returns `initialized` not `nvs_value.has_value()`
4. get() uses `prefs.begin(..., true)` and checks return value
5. set() computes dirty with nvs_value/default comparison
</verification>

<success_criteria>
- CacheEntry separates initialization state from NVS presence
- get() is read-only, handles fresh device without creating namespace
- set() only marks dirty when value actually differs from baseline
- Code compiles without errors
</success_criteria>

<output>
After completion, create `.planning/quick/003-fix-dirty-tracking-semantics/003-SUMMARY.md`
</output>
