---
phase: 03-smart-persistence
plan: 01
subsystem: persistence
tags: [nvs, esp32, flash, preferences, embedded]

# Dependency graph
requires:
  - phase: 02-ram-cache-dirty-tracking
    provides: "CacheEntry with dirty tracking, cache_entries array"
provides:
  - "save(key) function for per-key persistence with default removal"
  - "save() function for batch persistence with namespace grouping"
  - "KeyMetadata struct for runtime namespace/key access"
affects: [04-advanced-features]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Namespace batching for flash wear reduction"
    - "Default value removal optimization"

key-files:
  created:
    - "test/save_test/save_test.ino"
  modified:
    - "src/QPreferences/CacheEntry.h"
    - "src/QPreferences/QPreferences.h"

key-decisions:
  - "KeyMetadata parallel array for runtime namespace/key access without templates"
  - "save() batch function writes all values (no default comparison without template context)"
  - "save(key) has default comparison and removes key if value equals default"

patterns-established:
  - "Namespace batching: group dirty keys by namespace for single begin/end cycle"
  - "Default removal: save(key) removes key from NVS when value equals default"

# Metrics
duration: 4min
completed: 2026-02-03
---

# Phase 3 Plan 1: Explicit Save API Summary

**save(key) and save() functions with namespace batching and default value optimization**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-03T09:15:00Z
- **Completed:** 2026-02-03T09:19:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- KeyMetadata struct for runtime namespace/key access without template context
- save(key) function with default value comparison and NVS removal
- save() batch function with namespace grouping for minimal flash wear
- Test sketch demonstrating all save behaviors with reboot verification

## Task Commits

Each task was committed atomically:

1. **Task 1: Add KeyMetadata and update cache registration** - `85886a7` (feat)
2. **Task 2: Implement save(key) and save() functions** - `781cce1` (feat)

## Files Created/Modified
- `src/QPreferences/CacheEntry.h` - Added KeyMetadata struct and updated register_key()
- `src/QPreferences/QPreferences.h` - Added save(key) and save() functions
- `test/save_test/save_test.ino` - Verification sketch for save API

## Decisions Made
- **KeyMetadata parallel array:** Stores namespace/key pointers alongside cache_entries to enable save() to access metadata without template context at runtime
- **save() always writes values:** Batch save cannot compare to defaults without template context, so values are always written. Users wanting default removal should use save(key)
- **nvs_value.reset() for default removal:** When save(key) removes a key because it equals default, nvs_value is reset to indicate no NVS value exists

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed std::monostate usage in save()**
- **Found during:** Task 2 (implementing save functions)
- **Issue:** Plan code used `entry.nvs_value = std::monostate{}` but nvs_value is `std::optional<ValueVariant>` and ValueVariant doesn't include std::monostate
- **Fix:** Changed to `entry.nvs_value.reset()` for proper optional clearing
- **Files modified:** src/QPreferences/QPreferences.h
- **Verification:** Code compiles correctly with proper type handling
- **Committed in:** 781cce1 (Task 2 commit)

**2. [Rule 1 - Bug] Removed std::monostate handling from std::visit**
- **Found during:** Task 2 (implementing save functions)
- **Issue:** Plan code checked for std::monostate in std::visit lambda but ValueVariant doesn't include it
- **Fix:** Removed the std::monostate check from the visitor
- **Files modified:** src/QPreferences/QPreferences.h
- **Verification:** Code compiles with correct variant type handling
- **Committed in:** 781cce1 (Task 2 commit)

---

**Total deviations:** 2 auto-fixed (2 bugs)
**Impact on plan:** Both auto-fixes necessary for correct compilation. Plan code had type mismatches with the actual ValueVariant definition.

## Issues Encountered
None - execution proceeded smoothly after correcting type issues.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Explicit save API complete and ready for use
- Phase 3 persistence layer is now functional
- Remaining Phase 3 plans can build on this foundation
- Ready for Phase 4 advanced features (if applicable)

---
*Phase: 03-smart-persistence*
*Completed: 2026-02-03*
