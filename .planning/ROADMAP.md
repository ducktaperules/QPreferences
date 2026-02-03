# Roadmap: QPreferences

## Overview

QPreferences delivers a type-safe ESP32 preferences library through four focused phases. Starting with template-based PrefKey definitions and compile-time validation, we layer on RAM caching with dirty tracking, then add smart NVS persistence that eliminates defaults and batches writes, and finally enable iteration for debugging and factory reset. Each phase builds on the previous, creating a production-ready library that solves flash wear and type safety problems plaguing raw Preferences.h usage.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Foundation & Type Safety** - Template infrastructure and registry
- [ ] **Phase 2: RAM Cache & Dirty Tracking** - Memory-first change tracking
- [ ] **Phase 3: Smart Persistence** - Optimized NVS storage with batching
- [ ] **Phase 4: Iteration & Examples** - Enumeration and documentation

## Phase Details

### Phase 1: Foundation & Type Safety
**Goal**: Developer can define type-safe preferences and access them uniformly
**Depends on**: Nothing (first phase)
**Requirements**: DEF-01, DEF-02, DEF-03, DEF-04, DEF-05
**Success Criteria** (what must be TRUE):
  1. Developer can define a PrefKey with namespace, key name, and default value using template syntax
  2. Compiler rejects PrefKey definitions with namespace or key names longer than 15 characters
  3. Developer can call get(key) and receive correct type without casting
  4. Developer can call set(key, value) and compiler enforces type matching
  5. All supported types (int, float, bool, String) work with unified get/set API
**Plans**: 2 plans

Plans:
- [ ] 01-01-PLAN.md — Core templates (StringLiteral + PrefKey with compile-time validation)
- [ ] 01-02-PLAN.md — Unified get/set API + library structure + example

### Phase 2: RAM Cache & Dirty Tracking
**Goal**: Changes are tracked in memory without immediate flash writes
**Depends on**: Phase 1
**Requirements**: PERS-02, TRCK-01, TRCK-02
**Success Criteria** (what must be TRUE):
  1. After calling set(key, value), the value exists only in RAM and not written to NVS
  2. Developer can call isModified(key) to check if current value differs from default
  3. Developer can call isDirty(key) to check if current value differs from what's stored in NVS
  4. Multiple set() calls to same key update RAM cache without triggering flash writes
**Plans**: TBD

Plans:
- TBD (planned during /gsd:plan-phase 2)

### Phase 3: Smart Persistence
**Goal**: Persistence is explicit, optimized, and flash-friendly
**Depends on**: Phase 2
**Requirements**: PERS-01, PERS-03, PERS-04, PERS-05
**Success Criteria** (what must be TRUE):
  1. On boot, values stored in NVS are loaded into RAM automatically
  2. Developer can call save() to persist all dirty values to NVS in single operation
  3. When saved value matches default, the key is removed from NVS (not stored)
  4. Multiple dirty keys in same namespace are written in single begin/end cycle (batched)
  5. After save() completes, isDirty() returns false for all saved keys
**Plans**: TBD

Plans:
- TBD (planned during /gsd:plan-phase 3)

### Phase 4: Iteration & Examples
**Goal**: Developer can enumerate and inspect all preferences
**Depends on**: Phase 3
**Requirements**: TRCK-03, TRCK-04
**Success Criteria** (what must be TRUE):
  1. Developer can iterate over all registered PrefKey instances with access to namespace, key name, and current value
  2. Developer can filter iteration to only keys in specific namespace
  3. Library includes working example sketches demonstrating basic usage, dirty tracking, and namespace grouping
  4. Library includes factory reset function that clears all NVS entries and restores defaults
**Plans**: TBD

Plans:
- TBD (planned during /gsd:plan-phase 4)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation & Type Safety | 0/2 | Planned | - |
| 2. RAM Cache & Dirty Tracking | 0/TBD | Not started | - |
| 3. Smart Persistence | 0/TBD | Not started | - |
| 4. Iteration & Examples | 0/TBD | Not started | - |
