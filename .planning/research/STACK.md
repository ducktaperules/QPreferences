# Technology Stack

**Project:** QPreferences - ESP32 Arduino NVS Wrapper Library
**Researched:** 2026-02-03
**Confidence:** HIGH

## Recommended Stack

### Core Framework
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Arduino ESP32 | 3.3.6+ | ESP32 Arduino framework | Latest stable release (Dec 2025), based on ESP-IDF 5.5, supports all ESP32 variants. Version 3.x required for modern chip support (C6, H2). |
| PlatformIO | Latest | Build system | Superior dependency management, testing support, CI integration. Handles both Arduino and ESP-IDF frameworks. |
| Preferences.h | Built-in | ESP32 NVS wrapper | Official ESP32 Arduino NVS library. Provides type-safe API for 13 data types. No external dependency needed. |

**Rationale:** Arduino ESP32 3.3.6 is the current stable release based on ESP-IDF 5.5. Using PlatformIO over Arduino IDE provides professional tooling with built-in unit testing, better dependency management, and easier CI/CD integration.

### Language & Toolchain
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| C++11 | Default | Template language | Arduino ESP32 enables C++11 by default. Sufficient for template metaprogramming, type traits, and variadic templates needed for PrefKey<T>. |
| GCC (xtensa-esp32) | Bundled | Compiler | Bundled with Arduino ESP32 core, supports all necessary C++11 features for templates. |

**Rationale:** C++11 provides the minimum needed for type-safe templates (variadic templates, type traits, constexpr). No need for C++14/17 features. ESP32 Arduino enables C++11 by default, so no configuration overhead.

### Supporting Libraries
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| ArduinoJson | 7.2.1+ | JSON serialization | For JSON export feature. Current stable is 7.4.2. Use v7 for reduced memory footprint (50% size reduction for arrays on 32-bit). |
| Unity | 2.5.2+ | Unit testing framework | For both native (PC) and embedded (ESP32) unit tests. Built into PlatformIO. |

**Rationale:**
- **ArduinoJson 7.x**: Version 7 provides significant memory improvements over v6 (arrays are half the size on 32-bit). ESP32-compatible, header-only option available, supports custom allocators for PSRAM if needed.
- **Unity**: Designed for constrained embedded devices. Works with PlatformIO's dual test environment setup (native for fast iteration, embedded for hardware validation).

### Development Tools
| Tool | Version | Purpose | Why |
|------|---------|---------|-----|
| VSCode + PlatformIO IDE | Latest | IDE | Industry standard for PlatformIO development. Integrated debugging, testing, serial monitor. |
| Doxygen | 1.9.x+ | Documentation generation | Standard for C++ library documentation. Generates HTML docs from inline comments. |
| arduino-lint | Latest | Library validation | Arduino's official linting tool. Validates library.properties, folder structure, compliance with Arduino Library specification. |

**Rationale:** VSCode + PlatformIO IDE is the de facto standard for ESP32 development in 2025. Doxygen is the standard for C++ API documentation and integrates well with GitHub Pages.

### Testing Infrastructure
| Component | Technology | Purpose |
|-----------|-----------|---------|
| Unit Testing Framework | Unity | Assertions, test organization |
| Test Runner | PlatformIO Test | Dual environment (native + embedded) |
| Native Environment | Native (x86/x64) | Fast iteration without hardware |
| Embedded Environment | ESP32 (espressif32) | Real hardware validation |
| CI/CD | GitHub Actions | Automated build & test on push |

**Configuration:**
```ini
; platformio.ini
[env:native]
platform = native
test_ignore = test_embedded

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
test_ignore = test_desktop
```

**Rationale:** Dual test environment allows TDD workflow. Native tests run in seconds (compile and run on PC), embedded tests validate hardware behavior (flash wear, NVS persistence).

## Library Structure

### File Organization
```
QPreferences/
├── src/
│   ├── QPreferences.h          # Main public API
│   ├── PrefKey.h               # Template class definition
│   └── QPreferencesImpl.h      # Template implementation (if needed)
├── examples/
│   ├── BasicUsage/
│   │   └── BasicUsage.ino
│   ├── DirtyTracking/
│   │   └── DirtyTracking.ino
│   └── JsonExport/
│       └── JsonExport.ino
├── test/
│   ├── test_desktop/           # Native tests (fast iteration)
│   │   └── test_prefkey.cpp
│   └── test_embedded/          # ESP32 tests (hardware validation)
│       └── test_nvs_persistence.cpp
├── library.properties          # Arduino Library Manager metadata
├── library.json                # PlatformIO library metadata
├── keywords.txt                # Arduino IDE syntax highlighting
├── LICENSE                     # MIT or Apache-2.0
├── README.md
└── platformio.ini              # PlatformIO configuration
```

**Why this structure:**
- `src/` contains all library code (Arduino Library spec)
- Separate native and embedded tests (PlatformIO best practice)
- Both `library.properties` and `library.json` for dual distribution
- Examples demonstrate each feature (required for Arduino Library Manager)

### Template Implementation Strategy

**Option 1: Header-Only (Recommended)**
- Define template implementation in header files
- No `.cpp` files needed for template classes
- Simpler distribution, no linking issues

**Option 2: Explicit Instantiation**
- Define templates in `.h`, implement in `.cpp`
- Explicitly instantiate for int, float, bool, String
- Better compile times for users, but more rigid

**Recommendation:** Use **header-only** approach. QPreferences will have limited template types (int, float, bool, String), so compilation overhead is minimal. Header-only simplifies distribution and avoids linker issues.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Build System | PlatformIO | Arduino IDE | PlatformIO has superior testing, CI, and dependency management |
| JSON Library | ArduinoJson 7.x | ESP32 cJSON (ESP-IDF) | ArduinoJson is Arduino-native, more portable, better documented |
| Testing Framework | Unity | ArduinoUnit | Unity works better with PlatformIO, better embedded support |
| Testing Framework | Unity | AUnit | AUnit requires EpoxyDuino for native tests, Unity is simpler |
| C++ Version | C++11 | C++14/17 | C++11 is default, sufficient for templates, no config overhead |
| License | MIT | Apache-2.0 | MIT is simpler, most popular (1.53M pageviews in 2025), sufficient patent protection for libraries |
| License | MIT | LGPL/GPL | Copyleft licenses create friction for commercial users |
| Documentation | Doxygen | Manual Markdown | Doxygen auto-generates from code, stays in sync |

## Distribution Strategy

### Arduino Library Manager
**Requirements:**
- `library.properties` with correct metadata
- Name must be unique, start with letter/number, no "Arduino" prefix
- Semantic versioning (e.g., 1.0.0, 1.2.1)
- Architecture: `esp32` (ESP32-only) or `*` (universal)
- OSI-approved license (MIT recommended)
- GitHub releases with tags matching versions
- Submit repository URL to Arduino Library Registry

**library.properties template:**
```properties
name=QPreferences
version=1.0.0
author=Your Name
maintainer=Your Name <email@example.com>
sentence=Type-safe preferences library for ESP32 with explicit persistence control
paragraph=Wraps ESP32 Preferences.h with PrefKey<T> template for single-source-of-truth preference definitions, RAM-first changes with explicit save(), smart storage, dirty tracking, and JSON serialization.
category=Data Storage
url=https://github.com/yourusername/QPreferences
architectures=esp32
includes=QPreferences.h
depends=ArduinoJson@^7.2.1
```

### PlatformIO Registry
**Requirements:**
- `library.json` with metadata
- Automatic indexing from GitHub releases
- No manual submission needed after initial setup

**library.json template:**
```json
{
  "name": "QPreferences",
  "version": "1.0.0",
  "description": "Type-safe preferences library for ESP32 with explicit persistence control",
  "keywords": ["esp32", "preferences", "nvs", "storage", "type-safe"],
  "repository": {
    "type": "git",
    "url": "https://github.com/yourusername/QPreferences.git"
  },
  "authors": [
    {
      "name": "Your Name",
      "email": "email@example.com",
      "url": "https://github.com/yourusername"
    }
  ],
  "license": "MIT",
  "frameworks": ["arduino", "espidf"],
  "platforms": ["espressif32"],
  "dependencies": [
    {
      "name": "bblanchon/ArduinoJson",
      "version": "^7.2.1"
    }
  ]
}
```

### Both Files Recommended
PlatformIO supports both formats but recommends `library.json` for better compatibility. Including both ensures:
- Arduino IDE users can install via Library Manager
- PlatformIO users get better dependency resolution
- Library works in both ecosystems

## Installation

### For Library Users

**Arduino Library Manager:**
```
Sketch > Include Library > Manage Libraries... > Search "QPreferences" > Install
```

**PlatformIO:**
```ini
; platformio.ini
[env:esp32dev]
platform = espressif32
framework = arduino
lib_deps =
    QPreferences@^1.0.0
```

**Manual Installation:**
```bash
cd ~/Arduino/libraries
git clone https://github.com/yourusername/QPreferences.git
```

### For Library Development

**Setup PlatformIO project:**
```bash
# Clone repository
git clone https://github.com/yourusername/QPreferences.git
cd QPreferences

# PlatformIO will auto-install dependencies
pio lib install

# Run native tests (fast)
pio test -e native

# Run embedded tests (hardware required)
pio test -e esp32dev

# Build examples
pio ci examples/BasicUsage --board=esp32dev
```

## CI/CD Configuration

**GitHub Actions workflow (.github/workflows/ci.yml):**
```yaml
name: PlatformIO CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - name: Install PlatformIO
        run: pip install platformio
      - name: Run native tests
        run: pio test -e native
      - name: Build library
        run: pio lib install && pio ci src --board=esp32dev
      - name: Build examples
        run: |
          pio ci examples/BasicUsage --board=esp32dev
          pio ci examples/DirtyTracking --board=esp32dev
          pio ci examples/JsonExport --board=esp32dev
      - name: Run arduino-lint
        uses: arduino/arduino-lint-action@v1
        with:
          library-manager: update
```

**Why this setup:**
- Runs on every push and PR
- Tests compile on Linux (most common CI environment)
- Validates native unit tests (fast)
- Validates library builds for ESP32
- Validates all examples compile
- Runs Arduino Library spec compliance checks

## Key Dependencies & Versions

| Dependency | Minimum Version | Current Stable | Notes |
|------------|----------------|---------------|-------|
| Arduino ESP32 | 3.0.0 | 3.3.6 | Version 3.x required (ESP-IDF 5.x) |
| PlatformIO | 6.1.0+ | Latest | For Unity testing support |
| ArduinoJson | 7.0.0 | 7.4.2 | Version 7.x for memory improvements |
| C++ Standard | C++11 | C++11 | Enabled by default in Arduino ESP32 |
| Unity | 2.5.2+ | Built-in | Bundled with PlatformIO |

**Version pinning strategy:**
- **Arduino ESP32**: Pin to `^3.0.0` (allow 3.x updates, not 4.x)
- **ArduinoJson**: Pin to `^7.2.1` (allow 7.x updates, not 8.x)
- **PlatformIO**: Use latest (developer tool, not library dependency)

## Platform Constraints

### ESP32 NVS Limitations
| Constraint | Value | Impact on QPreferences |
|------------|-------|----------------------|
| Namespace name length | 15 characters max | Enforce in PrefKey constructor |
| Key name length | 15 characters max | Enforce in PrefKey constructor |
| Maximum namespaces | 254 | Document limitation, no runtime enforcement |
| Single namespace open | One at a time | QPreferences manages open/close internally |
| Minimum NVS partition | 12 KiB (3 pages) | Document in requirements |

**Design implications:**
- PrefKey constructor should validate namespace/key length at compile time if possible (constexpr), runtime otherwise
- Cache currently open namespace to avoid unnecessary close/open cycles
- Document that users should keep namespace/key names under 15 characters

### Memory Considerations
| Resource | Usage | Mitigation |
|----------|-------|-----------|
| RAM per PrefKey | ~24-32 bytes (pointer, type info, metadata) | Self-registration means RAM scales with number of defined keys |
| RAM per namespace | 128-640 bytes (NVS overhead) | Acceptable for typical use (< 10 namespaces) |
| Flash wear | ~100K erase cycles | Explicit save() gives user control over write frequency |
| Compilation time | Template instantiation | Header-only may slow compilation, but only during library development |

## Licensing Recommendation

**Recommended: MIT License**

**Rationale:**
- Most popular open source license (1.53M pageviews, 925K visitors in 2025)
- Simplest terms: just require attribution
- No patent clause needed for library code
- Maximizes adoption (commercial-friendly)
- Matches most Arduino ecosystem libraries

**Alternative: Apache-2.0**
- Choose if you want explicit patent protection
- More verbose license text
- Still permissive and commercial-friendly
- 2nd most popular (344K pageviews in 2025)

**Avoid: GPL/LGPL**
- Copyleft requirements create friction
- Many commercial users avoid GPL dependencies
- LGPL is complex (dynamic linking requirements)
- Not common in ESP32 Arduino ecosystem

## Sources

### Official Documentation
- [Creating an Arduino Library for ESP32 (Dec 2025)](https://developer.espressif.com/blog/2025/12/arduino-library-creation/)
- [ESP32 Preferences Library Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
- [Arduino ESP32 Core 3.3.6 Release](https://github.com/espressif/arduino-esp32/releases)
- [PlatformIO Library Creation](https://docs.platformio.org/en/latest/librarymanager/creating.html)
- [Arduino Library Specification](https://arduino.github.io/arduino-cli/0.19/library-specification/)
- [ESP32 NVS Flash Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html)

### Libraries & Tools
- [ArduinoJson Official Site](https://arduinojson.org/)
- [ArduinoJson v7.2.1 on ESP Component Registry](https://components.espressif.com/components/bblanchon/arduinojson/versions/7.2.1)
- [PlatformIO Unity Testing Framework](https://docs.platformio.org/en/latest/advanced/unit-testing/frameworks/unity.html)
- [Arduino Library Registry](https://github.com/arduino/library-registry)

### Best Practices & Guides
- [PlatformIO ESP32 Unit Testing Guide (2025)](https://ibrahimmansur4.medium.com/unit-testing-on-esp32-with-platformio-a-step-by-step-guide-d33f3241192b)
- [Adafruit Doxygen Guide (Updated Jan 2025)](https://learn.adafruit.com/the-well-automated-arduino-library/doxygen)
- [MIT and Apache 2.0 Lead Open Source Licensing in 2025](https://linuxiac.com/mit-and-apache-2-0-lead-open-source-licensing-in-2025/)
- [C++ STL with ESP32 Discussion](https://community.platformio.org/t/c-stl-library-for-esp32/28313)

### Community Resources
- [ESP32 Arduino Core GitHub](https://github.com/espressif/arduino-esp32)
- [PlatformIO Community Forums](https://community.platformio.org/)
- [Arduino Library Registry FAQ](https://github.com/arduino/library-registry/blob/main/FAQ.md)

---

**Last Updated:** 2026-02-03
**Research Confidence:** HIGH (verified with official documentation and current 2025 releases)
