---
phase: 02-ram-cache-dirty-tracking
plan: 01
subsystem: cache
tags: [cache, variant, optional, ram, dirty-tracking]

# Dependency graph
requires:
  - phase: 01-foundation-type-safety
    provides: PrefKey type-safe definitions and get/set API
provides:
  - RAM cache layer with lazy NVS initialization
  - isModified() API for default value comparison
  - isDirty() API for unsaved change tracking
  - CacheEntry infrastructure for Phase 3 persistence
affects: [03-explicit-save-batch-writes, 04-additional-types]

# Tech tracking
tech-stack:
  added: [std::variant, std::optional, std::array]
  patterns: [lazy initialization, three-state tracking, cache invalidation]

key-files:
  created: [src/QPreferences/CacheEntry.h, test/cache_test/cache_test.ino]
  modified: [src/QPreferences/QPreferences.h]

key-decisions:
  - "Use std::variant over std::any for RTTI-free fixed-size storage"
  - "Lazy NVS initialization - load on first access only"
  - "set() writes RAM only - no automatic persistence"
  - "MAX_KEYS=64 provides safe 2.5KB cache footprint"
  - "Static inline for header-only cache storage (C++17)"

patterns-established:
  - "Cache entry three-state tracking: uninitialized → clean → dirty"
  - "get_key_id() uses static variable for persistent per-type ID"
  - "Both isModified/isDirty trigger lazy load to avoid uninitialized reads"

# Metrics
duration: 2min
completed: 2026-02-03
---

# Phase 02 Plan 01: RAM Cache & Dirty Tracking Summary

**RAM-cached get/set with lazy NVS initialization and isModified/isDirty tracking APIs**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-03T08:46:09Z
- **Completed:** 2026-02-03T08:48:18Z
- **Tasks:** 2
- **Files modified:** 3 (2 source + 1 test)

## Accomplishments
- Set operations now write to RAM only, eliminating flash wear on every change
- Lazy NVS initialization reduces flash reads to first access per key
- isModified() enables UI state comparison against defaults
- isDirty() enables "unsaved changes" warnings before power loss

## Task Commits

Each task was committed atomically:

1. **Task 1: Create CacheEntry structure with three-state tracking** - `3929436` (feat)
2. **Task 2: Add cache layer to get/set and implement tracking APIs** - `6753c70` (feat)

**Plan metadata:** (committed separately after SUMMARY.md)

## Files Created/Modified

**Created:**
- `src/QPreferences/CacheEntry.h` - ValueVariant, CacheEntry struct, cache_entries array
- `test/cache_test/cache_test.ino` - Verification sketch for cache behavior

**Modified:**
- `src/QPreferences/QPreferences.h` - Cache-intercepted get/set, isModified/isDirty APIs

## Decisions Made

**Use std::variant over std::any**
- Rationale: Fixed-size storage, no RTTI overhead, embedded-friendly
- Impact: CacheEntry has predictable memory footprint

**Lazy NVS initialization**
- Rationale: Reduces flash reads - only load on first access per key
- Impact: get() checks is_initialized() before touching NVS

**set() writes RAM only**
- Rationale: Defers persistence for Phase 3 save() API - reduces flash wear
- Impact: User must explicitly call save() to persist (future Phase 3 work)

**MAX_KEYS = 64**
- Rationale: ~2.5KB total cache footprint is safe for ESP32 (minimum 32KB RAM)
- Impact: Supports typical embedded app with room for other allocations

**Static inline for cache storage**
- Rationale: Header-only implementation without ODR violations (C++17 feature)
- Impact: No .cpp file needed, cleaner single-header pattern

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - implementation proceeded as planned.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

**Ready for Phase 3 (Explicit Save & Batch Writes):**
- Cache infrastructure complete with dirty flag tracking
- isDirty() API ready for save() to query which keys need persistence
- nvs_value baseline established for comparison after save()

**No blockers:**
- All Phase 2 must-haves satisfied
- Test sketch demonstrates expected RAM-only behavior
- No dependencies on external services

---
*Phase: 02-ram-cache-dirty-tracking*
*Completed: 2026-02-03*
