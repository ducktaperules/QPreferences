# Requirements: QPreferences

**Defined:** 2026-02-03
**Core Value:** Single-source-of-truth preference definitions with type-safe access and explicit persistence control

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Definition

- [ ] **DEF-01**: Define preference with `PrefKey<T>` template specifying namespace, key name, and default value
- [ ] **DEF-02**: Support int, float, bool, and String value types
- [ ] **DEF-03**: Unified `get(key)` function that returns correct type based on key
- [ ] **DEF-04**: Unified `set(key, value)` function with compile-time type checking
- [ ] **DEF-05**: Compile-time validation that namespace and key names are ≤15 characters

### Persistence

- [ ] **PERS-01**: On boot, load stored values that differ from defaults into RAM
- [ ] **PERS-02**: Changes via `set()` stay in RAM only (not written to NVS)
- [ ] **PERS-03**: Explicit `save()` function persists RAM values to NVS
- [ ] **PERS-04**: When saving, remove NVS entry if value matches default (don't store defaults)
- [ ] **PERS-05**: `save()` batches writes to minimize flash wear

### Tracking

- [ ] **TRCK-01**: `isModified(key)` returns true if current value differs from default
- [ ] **TRCK-02**: `isDirty(key)` returns true if current RAM value differs from NVS
- [ ] **TRCK-03**: Iterate over all registered preferences with access to key name and status
- [ ] **TRCK-04**: Filter iteration by namespace

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Serialization

- **SER-01**: Export all preferences to JSON string
- **SER-02**: Export preferences from single namespace to JSON
- **SER-03**: Import preferences from JSON string

### Advanced

- **ADV-01**: Validation callbacks on set (reject invalid values)
- **ADV-02**: Change callbacks (notify when value changes)
- **ADV-03**: Schema versioning and migrations

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Binary blob storage | Not needed for typical preferences, adds complexity |
| ESP-IDF native support | Arduino framework only for v1 |
| Encryption | Use ESP32's built-in NVS encryption if needed |
| Remote sync | This is local storage only |
| JSON serialization | Keeping v1 scope tight; can add in v2 |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DEF-01 | Phase 1 | Complete |
| DEF-02 | Phase 1 | Complete |
| DEF-03 | Phase 1 | Complete |
| DEF-04 | Phase 1 | Complete |
| DEF-05 | Phase 1 | Complete |
| PERS-01 | Phase 3 | Complete |
| PERS-02 | Phase 2 | Complete |
| PERS-03 | Phase 3 | Complete |
| PERS-04 | Phase 3 | Complete |
| PERS-05 | Phase 3 | Complete |
| TRCK-01 | Phase 2 | Complete |
| TRCK-02 | Phase 2 | Complete |
| TRCK-03 | Phase 4 | Pending |
| TRCK-04 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0

---
*Requirements defined: 2026-02-03*
*Last updated: 2026-02-03 after roadmap creation*
