---
phase: 04-iteration-examples
plan: 01
subsystem: api
tags: [iteration, forEach, factoryReset, PrefInfo, ESP32]

# Dependency graph
requires:
  - phase: 03-smart-persistence
    provides: KeyMetadata parallel array and save() with namespace batching
provides:
  - PrefInfo struct for iteration callbacks
  - forEach() to iterate all registered keys
  - forEachInNamespace() for filtered iteration
  - factoryReset() to clear NVS and reset cache
affects: [04-02-PLAN, examples, debugging, status display]

# Tech tracking
tech-stack:
  added: []
  patterns: [callback-based iteration, namespace batching for NVS operations]

key-files:
  created: []
  modified:
    - src/QPreferences/CacheEntry.h
    - src/QPreferences/QPreferences.h

key-decisions:
  - "PrefInfo excludes raw value - users access via typed get()"
  - "forEach iterates next_key_id not MAX_KEYS for efficiency"
  - "factoryReset clears by namespace batch, not individual keys"

patterns-established:
  - "Callback iteration: forEach(callback) pattern for key enumeration"
  - "Namespace filtering: forEachInNamespace(ns, callback) for scoped iteration"

# Metrics
duration: 1min
completed: 2026-02-03
---

# Phase 4 Plan 1: Iteration API Summary

**PrefInfo struct and forEach/forEachInNamespace/factoryReset iteration functions for preference enumeration and reset**

## Performance

- **Duration:** 1 min
- **Started:** 2026-02-03T09:33:15Z
- **Completed:** 2026-02-03T09:34:20Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- PrefInfo struct provides namespace_name, key_name, index, is_initialized, is_dirty for callbacks
- forEach() iterates all registered preference keys with callback
- forEachInNamespace() filters iteration to specific namespace
- factoryReset() clears all NVS namespaces and resets cache state

## Task Commits

Each task was committed atomically:

1. **Task 1: Add PrefInfo struct for iteration callbacks** - `aaf6e80` (feat)
2. **Task 2: Implement forEach, forEachInNamespace, and factoryReset** - `b2dc72f` (feat)

## Files Created/Modified
- `src/QPreferences/CacheEntry.h` - Added PrefInfo struct for iteration callbacks
- `src/QPreferences/QPreferences.h` - Added forEach, forEachInNamespace, factoryReset functions

## Decisions Made
- **PrefInfo excludes raw value:** Users access values via typed get(key), not through iteration callback
- **Iterate next_key_id not MAX_KEYS:** Only iterate registered keys for efficiency
- **factoryReset uses namespace batching:** Groups keys by namespace and calls clear() once per namespace, matching save() pattern

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Iteration API complete and ready for examples
- factoryReset() available for user reset functionality
- Ready for 04-02-PLAN.md example sketch

---
*Phase: 04-iteration-examples*
*Completed: 2026-02-03*
