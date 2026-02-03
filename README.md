# QPreferences

Type-safe ESP32 preferences with compile-time validation

## Features

- `PrefKey<T, "namespace", "key">{default}` for compile-time type safety
- Namespace/key length validation (15 char ESP32 NVS limit)
- RAM cache with dirty tracking (`isDirty`, `isModified`)
- Explicit persistence (`save()`, `save(key)`, `factoryReset()`)
- Iteration (`forEach`, `forEachInNamespace`)
- Supported types: `int`, `float`, `bool`, `String`

## Requirements

- ESP32 (PlatformIO or Arduino IDE)
- C++20 (for constexpr string templates)

## Installation

**PlatformIO:**

```ini
lib_deps =
    https://github.com/ducktaperules/QPreferences
build_flags = -std=gnu++20
```

**Arduino IDE:**

- Download ZIP, Sketch > Include Library > Add .ZIP Library
- Requires ESP32 Arduino Core with C++20 support

## Quick Start

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

## API Reference

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

## Examples

- **BasicUsage** - Core get/set/save usage
- **DirtyTracking** - isDirty vs isModified, selective saves
- **NamespaceGroups** - forEach, forEachInNamespace, factoryReset

## License

MIT
