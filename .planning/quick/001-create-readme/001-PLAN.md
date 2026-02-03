---
type: quick
task: 001-create-readme
wave: 1
autonomous: true
files_modified:
  - README.md
---

<objective>
Create a concise README.md documenting QPreferences - a type-safe ESP32 preferences library.

Purpose: Provide installation instructions and usage documentation so users can quickly adopt the library.
Output: README.md in repository root
</objective>

<context>
@.planning/STATE.md
@src/QPreferences/QPreferences.h
@src/QPreferences/PrefKey.h
@examples/BasicUsage/BasicUsage.ino
@library.json
</context>

<tasks>

<task type="auto">
  <name>Task 1: Create README.md</name>
  <files>README.md</files>
  <action>
Create README.md with the following sections (keep it concise, no marketing fluff):

**Header**
- Title: QPreferences
- One-line description: Type-safe ESP32 preferences with compile-time validation

**Features** (bullet list, factual)
- PrefKey<T, "namespace", "key">{default} for compile-time type safety
- Namespace/key length validation (15 char ESP32 NVS limit)
- RAM cache with dirty tracking (isDirty, isModified)
- Explicit persistence (save(), save(key), factoryReset())
- Iteration (forEach, forEachInNamespace)
- Supported types: int, float, bool, String

**Requirements**
- ESP32 (PlatformIO or Arduino IDE)
- C++20 (for constexpr string templates)

**Installation**
PlatformIO:
```ini
lib_deps =
    https://github.com/ducktaperules/QPreferences
build_flags = -std=gnu++20
```

Arduino IDE:
- Download ZIP, Sketch > Include Library > Add .ZIP Library
- Requires ESP32 Arduino Core with C++20 support

**Quick Start** (minimal working example)
```cpp
#include <QPreferences/QPreferences.h>

PrefKey<int, "myapp", "count"> bootCount{0};

void setup() {
    int count = QPrefs::get(bootCount);
    QPrefs::set(bootCount, count + 1);
    QPrefs::save(bootCount);
}

void loop() {}
```

**API Reference** (table format)
| Function | Description |
|----------|-------------|
| `QPrefs::get(key)` | Get value (auto-typed, lazy-loads from NVS) |
| `QPrefs::set(key, value)` | Set value in RAM (no flash write) |
| `QPrefs::isDirty(key)` | True if RAM differs from NVS |
| `QPrefs::isModified(key)` | True if value differs from default |
| `QPrefs::save(key)` | Persist single key (removes if default) |
| `QPrefs::save()` | Persist all dirty keys |
| `QPrefs::factoryReset()` | Clear all NVS, restore defaults |
| `QPrefs::forEach(callback)` | Iterate all registered keys |

**Examples**
Point to examples/ directory:
- BasicUsage - Core get/set/save usage
- DirtyTracking - isDirty vs isModified, selective saves
- NamespaceGroups - forEach, forEachInNamespace, factoryReset

**License**
MIT

Avoid: flowery language, "easy to use", "powerful", "elegant", badge proliferation, excessive headers
  </action>
  <verify>README.md exists and contains: Features, Installation, Quick Start, API Reference sections</verify>
  <done>README.md provides complete installation and usage documentation in concise format</done>
</task>

</tasks>

<verification>
- [ ] README.md exists at repository root
- [ ] Contains PlatformIO installation instructions with lib_deps
- [ ] Contains working Quick Start code example
- [ ] API reference covers all public functions
- [ ] No marketing language or excessive verbosity
</verification>

<success_criteria>
README.md created with installation instructions, quick start example, and API reference that allows a new user to install and use QPreferences within 5 minutes of reading.
</success_criteria>

<output>
Create `.planning/quick/001-create-readme/001-SUMMARY.md` after completion.
</output>
