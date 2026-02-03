# QPreferences

## What This Is

An ESP32 Arduino library that wraps the Preferences API with a smarter, type-safe layer. Define all your preferences in one place with defaults, get/set them uniformly regardless of type, and control exactly when changes persist to NVS. Built for projects that need clean preference management without boilerplate.

## Core Value

Single-source-of-truth preference definitions with type-safe access and explicit persistence control.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Define preferences with `PrefKey<T>` template (namespace, key name, default value)
- [ ] Unified `get(key)` / `set(key, value)` API that deduces type from key
- [ ] On boot, load stored values that differ from defaults into RAM
- [ ] Changes stay in RAM until explicit `save()` call
- [ ] `save()` persists RAM values to NVS
- [ ] When saved value matches default, remove entry from NVS (don't store defaults)
- [ ] `isModified(key)` — check if current value differs from default
- [ ] `isDirty(key)` — check if current value differs from what's saved in NVS
- [ ] Iterate over all preferences with access to namespace, key name, value, status
- [ ] Filter iteration by namespace
- [ ] Support int, float, bool, and String types

### Out of Scope

- JSON serialization — keeping v1 scope tight; can add in v2
- Binary blob storage — not needed for typical preferences
- ESP-IDF native support — Arduino framework only
- Encryption — use ESP32's NVS encryption if needed
- Remote sync — this is local storage only

## Context

ESP32's built-in Preferences library works but requires:
- Manually tracking what's stored vs default
- No centralized definition of all preferences
- No dirty tracking or deferred persistence
- Separate get/put methods per type

This library wraps Preferences to provide a cleaner developer experience while maintaining full control over when data hits flash (important for flash wear and atomic saves).

## Constraints

- **Platform**: ESP32 with Arduino framework
- **Build**: PlatformIO
- **Dependency**: Uses Arduino Preferences.h internally
- **Memory**: Keys self-register, so RAM scales with number of preferences defined
- **Types**: Limited to int, float, bool, String (no arbitrary structs)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Template-based PrefKey<T> | Compile-time type safety, clean API | — Pending |
| Self-registering keys | Enables iteration without manual registry | — Pending |
| RAM-first with explicit save | Prevents flash wear, enables atomic saves | — Pending |
| Remove defaults from NVS | Saves space, makes "reset to default" trivial | — Pending |

---
*Last updated: 2026-02-03 after requirements definition*
