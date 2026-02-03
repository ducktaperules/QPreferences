# Architecture Patterns

**Domain:** ESP32 Arduino Preferences Library
**Researched:** 2026-02-03
**Confidence:** MEDIUM-HIGH

## Recommended Architecture

QPreferences follows a **layered facade pattern** with type-safe wrappers over ESP32's NVS storage:

```
User Code
    ↓
PrefKey<T> Template Interface (Type-Safe API)
    ↓
PrefKeyBase Registry (Type Erasure)
    ↓
QPreferences Manager (RAM Cache + Dirty Tracking)
    ↓
Preferences.h Wrapper
    ↓
ESP32 NVS (Flash Storage)
```

### Component Boundaries

| Component | Responsibility | Communicates With |
|-----------|---------------|-------------------|
| **PrefKey\<T\>** | Type-safe preference definition with namespace, key, default value | QPreferences (read/write), PrefKeyBase (registration) |
| **PrefKeyBase** | Type-erased base for heterogeneous storage and iteration | Global registry (self-registration), QPreferences (iteration) |
| **QPreferences** | RAM cache, dirty tracking, batch save, smart storage | PrefKey instances, Preferences.h, PrefKeyBase registry |
| **Preferences.h** | NVS wrapper (existing ESP32 library) | ESP32 NVS flash storage |
| **Global Registry** | Static collection of all PrefKey instances | PrefKeyBase constructors (registration) |

### Data Flow

**Read Flow (get):**
```
User calls: myPref.get()
    → PrefKey<T>::get() checks QPreferences RAM cache
    → If not cached: QPreferences reads from Preferences.h
    → Preferences.h reads from NVS flash
    → Value stored in RAM cache
    → Type-safe value returned to user
```

**Write Flow (set + save):**
```
User calls: myPref.set(newValue)
    → PrefKey<T>::set() updates QPreferences RAM cache
    → QPreferences marks key as dirty
    → User calls: QPreferences::save()
    → QPreferences iterates dirty keys via registry
    → For each dirty key: compare to default value
    → If equals default: remove from NVS (smart storage)
    → If differs: write to NVS via Preferences.h
    → Clear dirty flags
```

**Iteration Flow:**
```
User calls: QPreferences::forEach(callback)
    → QPreferences accesses global PrefKeyBase registry
    → Registry provides type-erased pointers
    → For each PrefKeyBase: call virtual method
    → Virtual dispatch to PrefKey<T> implementation
    → Callback receives typed value
```

## Patterns to Follow

### Pattern 1: Self-Registering Template
**What:** PrefKey\<T\> automatically registers with global registry on construction
**When:** Every PrefKey instance creation (typically global scope)
**Why:** Enables iteration without manual bookkeeping

**Example:**
```cpp
template<typename T>
class PrefKey : public PrefKeyBase {
    PrefKey(const char* ns, const char* key, T defaultVal)
        : PrefKeyBase(ns, key), _default(defaultVal) {
        // Constructor registers this instance
        getRegistry().push_back(this);
    }

private:
    static std::vector<PrefKeyBase*>& getRegistry() {
        // Meyer's Singleton: thread-safe, initialization-order-safe
        static std::vector<PrefKeyBase*> registry;
        return registry;
    }
};
```

**Critical detail:** Use static function returning reference to avoid static initialization order fiasco (SIOF). Global objects across translation units have undefined construction order, but function-local statics initialize on first call.

### Pattern 2: Type Erasure via Virtual Interface
**What:** PrefKeyBase provides pure virtual methods for type-agnostic operations
**When:** Iteration, serialization, or any heterogeneous container operation
**Why:** Allows single container to hold PrefKey\<int\>, PrefKey\<String\>, etc.

**Example:**
```cpp
class PrefKeyBase {
public:
    virtual void load() = 0;          // Read from NVS
    virtual void save() = 0;          // Write to NVS if dirty
    virtual bool isDirty() const = 0;
    virtual void toJson(JsonObject&) = 0;  // Serialize current value
protected:
    const char* _namespace;
    const char* _key;
};

template<typename T>
class PrefKey : public PrefKeyBase {
    void load() override { /* type-specific NVS read */ }
    void save() override { /* type-specific NVS write */ }
    // ...
};
```

### Pattern 3: RAM-First Cache with Lazy Persistence
**What:** All changes happen in RAM; explicit save() commits to flash
**When:** User modifies preferences and calls save()
**Why:** Reduces flash wear, improves performance, allows atomic batch updates

**Example:**
```cpp
class QPreferences {
    template<typename T>
    void set(PrefKey<T>& key, T value) {
        _cache[&key] = value;     // Store in RAM
        _dirty.insert(&key);      // Mark for save
    }

    void save() {
        for (auto* key : _dirty) {
            key->save();          // Virtual call writes to NVS
        }
        _dirty.clear();
    }
};
```

### Pattern 4: Smart Storage (Default Elimination)
**What:** Don't write default values to NVS; remove keys when reset to default
**When:** save() operation compares cached value to default
**Why:** Saves flash space, makes factory reset efficient

**Example:**
```cpp
template<typename T>
void PrefKey<T>::save() {
    if (_current == _default) {
        prefs.remove(_key);    // Delete from NVS
    } else {
        prefs.putT(_key, _current);  // Write to NVS
    }
}
```

### Pattern 5: Namespace Grouping
**What:** PrefKey instances specify namespace at construction
**When:** Defining preferences that logically group together
**Why:** NVS isolation, organizational clarity, selective clear operations

**Example:**
```cpp
// wifi.h
namespace WiFiPrefs {
    PrefKey<String> ssid("wifi", "ssid", "");
    PrefKey<String> password("wifi", "password", "");
}

// device.h
namespace DevicePrefs {
    PrefKey<String> name("device", "name", "ESP32");
    PrefKey<int> brightness("device", "brightness", 128);
}

// Clear just WiFi preferences
QPreferences::clearNamespace("wifi");
```

## Anti-Patterns to Avoid

### Anti-Pattern 1: Global Mutable State Without Singleton Protection
**What:** Multiple QPreferences instances managing same NVS data
**Why bad:** Cache inconsistency, race conditions, undefined behavior
**Instead:** Singleton pattern or static-only API

```cpp
// BAD
QPreferences prefs1;
QPreferences prefs2;  // Two caches for same data!

// GOOD - Option A: Singleton
class QPreferences {
public:
    static QPreferences& instance() {
        static QPreferences inst;
        return inst;
    }
private:
    QPreferences() = default;  // Private constructor
};

// GOOD - Option B: Static-only API
class QPreferences {
public:
    template<typename T>
    static void set(PrefKey<T>& key, T value);
    static void save();
private:
    static std::unordered_map<PrefKeyBase*, void*> _cache;
};
```

### Anti-Pattern 2: Storing Complex Types Without Serialization
**What:** Attempting to store structs or classes directly via putBytes
**Why bad:** Pointer corruption, padding issues, non-portability
**Instead:** Use JSON serialization (ArduinoJson) or implement custom serialize/deserialize

```cpp
// BAD
struct Config { String ssid; int port; };
Config cfg;
prefs.putBytes("config", &cfg, sizeof(cfg));  // DANGER: pointers, padding

// GOOD
void Config::save(Preferences& prefs) {
    prefs.putString("ssid", ssid);
    prefs.putInt("port", port);
}

// BETTER (for complex data)
#include <ArduinoJson.h>
String Config::toJson() {
    StaticJsonDocument<256> doc;
    doc["ssid"] = ssid;
    doc["port"] = port;
    String output;
    serializeJson(doc, output);
    return output;
}
```

### Anti-Pattern 3: Immediate NVS Writes on Every Change
**What:** Writing to flash on every set() call
**Why bad:** Flash wear (NVS has ~100k write cycles), performance degradation
**Instead:** RAM cache with explicit save()

```cpp
// BAD
void PrefKey<T>::set(T value) {
    prefs.begin(_namespace, false);
    prefs.putT(_key, value);  // Flash write every time
    prefs.end();
}

// GOOD
void PrefKey<T>::set(T value) {
    _cached = value;
    _dirty = true;  // Defer write until save()
}

void QPreferences::save() {
    prefs.begin(_namespace, false);
    for (auto* key : _dirtyKeys) {
        key->commitToNVS();  // Batch writes
    }
    prefs.end();
}
```

### Anti-Pattern 4: Long Key Names or Large Namespace Names
**What:** Using descriptive but long identifiers
**Why bad:** NVS limits namespace/key names to 15 characters; exceeding causes silent failure or truncation
**Instead:** Use concise names, document meaning externally

```cpp
// BAD (16 chars)
PrefKey<int> brightnessLevel("display", "brightnessLevel", 128);  // FAIL

// GOOD (abbreviate)
PrefKey<int> brightness("display", "bright", 128);  // 6 chars
```

### Anti-Pattern 5: Ignoring NVS Partition Size
**What:** Storing unlimited user data without size checks
**Why bad:** NVS partition is typically 16-32KB; overflow causes failures
**Instead:** Check free space, provide feedback, use filesystem for large data

```cpp
// BAD
void storeLog(String log) {
    static int counter = 0;
    prefs.putString(String(counter++).c_str(), log);  // Eventually fills NVS
}

// GOOD
bool storePreference(const char* key, const String& value) {
    if (prefs.freeEntries() < 10) {  // Reserve headroom
        Serial.println("NVS nearly full!");
        return false;
    }
    prefs.putString(key, value);
    return true;
}

// BETTER (for logs)
#include <LittleFS.h>
void storeLog(String log) {
    File f = LittleFS.open("/logs.txt", "a");  // Use filesystem
    f.println(log);
    f.close();
}
```

## Build Order Implications

### Phase 1: Foundation (No Dependencies)
**Components:**
- PrefKeyBase abstract interface
- Global registry infrastructure (Meyer's Singleton pattern)

**Why first:** No dependencies; establishes contracts for all other components

### Phase 2: Type-Safe Templates (Depends: Foundation)
**Components:**
- PrefKey\<T\> template class
- Self-registration in constructor
- Type-specific get/set methods

**Why second:** Builds on PrefKeyBase; provides user-facing API

**Critical implementation detail:** Template specializations for all Preferences.h supported types (bool, char, int, String, etc.)

### Phase 3: RAM Cache Manager (Depends: Foundation, Templates)
**Components:**
- QPreferences class with cache storage
- Dirty tracking mechanism
- Load/save orchestration

**Why third:** Requires PrefKey instances to exist; coordinates persistence

### Phase 4: Smart Features (Depends: All Prior)
**Components:**
- Default value elimination
- Namespace operations (clearNamespace)
- Iteration support (forEach, toJson)

**Why last:** Builds on complete system; user-facing conveniences

### Dependency Graph
```
PrefKeyBase (Phase 1)
    ↓
PrefKey<T> (Phase 2)
    ↓
QPreferences Cache (Phase 3)
    ↓
Smart Features (Phase 4)
```

## Memory Architecture

### Static Storage (Program Lifetime)
- **PrefKey instances:** Typically global/static scope (0.1-1KB total)
- **Global registry:** std::vector\<PrefKeyBase*\> (~24 bytes + N pointers)
- **QPreferences singleton:** Static instance (~100 bytes)

### Dynamic Storage (Runtime)
- **RAM cache:** std::unordered_map or similar (~24 bytes overhead + data)
  - Recommendation: Store void* and cast, or use std::variant\<supported_types\>
  - Size estimate: N_keys × (pointer + largest_type) ≈ N × 64 bytes worst case
- **Dirty set:** std::unordered_set\<PrefKeyBase*\> (~24 bytes + N pointers when dirty)

### Flash Storage (NVS Partition)
- **Default partition size:** 16-32KB (check partition table)
- **Per-entry overhead:** ~16-24 bytes (NVS internal structures)
- **String overhead:** Variable (length + null terminator)

### Optimization Strategies
1. **Lazy cache loading:** Only cache accessed keys (not all on begin)
2. **Typed union storage:** Use std::variant to avoid void* casting
3. **Reserved capacity:** Pre-reserve vector/map sizes if key count known
4. **Flash-only mode:** For rarely-changed preferences, skip RAM cache

## ESP32 Platform Constraints

### NVS Limitations (Must Handle)
- **15-character limit:** Namespace and key names
- **Partition size:** Typically 16-32KB, configurable
- **Write cycles:** ~100,000 erase cycles per sector
- **Single namespace open:** Only one Preferences.begin() active at a time

### Implementation Consequences
```cpp
class QPreferences {
    void save() {
        // Group by namespace to minimize begin/end calls
        std::map<String, std::vector<PrefKeyBase*>> grouped;
        for (auto* key : _dirty) {
            grouped[key->getNamespace()].push_back(key);
        }

        for (auto& [ns, keys] : grouped) {
            prefs.begin(ns.c_str(), false);
            for (auto* key : keys) {
                key->commitToNVS(prefs);  // Pass open Preferences object
            }
            prefs.end();  // Close namespace before next
        }
    }
};
```

### Flash Wear Considerations
- **Avoid:** Saving on every change (use RAM cache + explicit save)
- **Monitor:** Track save frequency; warn if excessive
- **Strategy:** Batch writes, eliminate defaults, use change detection

### Memory Constraints
- **ESP32 RAM:** ~520KB total, ~200KB typically available for app
- **Cache strategy:** For 100 preferences × 64 bytes = 6.4KB cached data (acceptable)
- **Large data:** Use LittleFS/SPIFFS for >1KB items, not Preferences

## Testing Architecture

### Unit Test Boundaries
**PrefKey\<T\> tests (isolated):**
- Template instantiation for all supported types
- Self-registration verification
- Type safety enforcement

**QPreferences tests (mocked NVS):**
- Cache hit/miss logic
- Dirty tracking correctness
- Save batching behavior
- Default elimination

**Integration tests (real NVS):**
- Persistence across resets
- Namespace isolation
- Error handling (full partition, invalid keys)

### Mock Strategy
```cpp
// Create mockable wrapper
class NVSInterface {
public:
    virtual bool begin(const char* ns, bool readOnly) = 0;
    virtual int getInt(const char* key, int defaultVal) = 0;
    // ...
};

class RealNVS : public NVSInterface { /* Delegates to Preferences.h */ };
class MockNVS : public NVSInterface { /* In-memory fake */ };

// Inject dependency
class QPreferences {
    explicit QPreferences(NVSInterface* nvs) : _nvs(nvs) {}
private:
    NVSInterface* _nvs;
};
```

## PlatformIO Integration

### Library Structure
```
QPreferences/
├── library.json              (PlatformIO metadata)
├── library.properties         (Arduino metadata)
├── src/
│   ├── QPreferences.h         (Main API)
│   ├── QPreferences.cpp
│   ├── PrefKey.h              (Template definitions)
│   └── PrefKeyBase.h          (Type-erased base)
├── examples/
│   ├── BasicUsage/
│   ├── NamespaceGroups/
│   └── JsonSerialization/
└── test/
    ├── test_prefkey/
    ├── test_cache/
    └── test_integration/
```

### Dependencies Declaration
```json
// library.json
{
  "name": "QPreferences",
  "version": "1.0.0",
  "frameworks": ["arduino"],
  "platforms": ["espressif32"],
  "dependencies": {
    "bblanchon/ArduinoJson": "^7.1.0"  // For JSON serialization feature
  },
  "build": {
    "flags": ["-std=gnu++17"]  // Requires C++17 for std::variant (optional)
  }
}
```

## Scalability Considerations

| Concern | Small Project (10 prefs) | Medium Project (50 prefs) | Large Project (200 prefs) |
|---------|--------------------------|---------------------------|---------------------------|
| **RAM usage** | <1KB cached data | ~3-5KB cached data | 10-15KB cached data (consider lazy loading) |
| **Flash usage** | <1KB NVS | 2-4KB NVS | 8-12KB NVS (monitor partition usage) |
| **Save duration** | <10ms | 20-50ms | 100-200ms (batch by namespace) |
| **Iteration speed** | Negligible | <1ms | 2-5ms (virtual call overhead acceptable) |
| **Registration time** | <1ms startup | <5ms startup | 10-20ms startup (acceptable for embedded) |

**Recommendation for large projects:** Implement lazy cache loading—only load preferences on first access rather than caching all at initialization.

## Advanced Pattern: Typed Iteration

**Challenge:** Iterate heterogeneous PrefKey\<T\> with type safety

**Solution: Visitor Pattern**
```cpp
class PrefKeyVisitor {
public:
    virtual void visit(PrefKey<int>& key) {}
    virtual void visit(PrefKey<String>& key) {}
    virtual void visit(PrefKey<bool>& key) {}
    // ... for all supported types
};

class PrefKeyBase {
public:
    virtual void accept(PrefKeyVisitor& visitor) = 0;
};

template<typename T>
class PrefKey : public PrefKeyBase {
    void accept(PrefKeyVisitor& visitor) override {
        visitor.visit(*this);  // Type-safe dispatch
    }
};

// Usage: Print all preferences
class PrintVisitor : public PrefKeyVisitor {
    void visit(PrefKey<int>& key) override {
        Serial.printf("%s = %d\n", key.getKey(), key.get());
    }
    void visit(PrefKey<String>& key) override {
        Serial.printf("%s = %s\n", key.getKey(), key.get().c_str());
    }
};

void printAll() {
    PrintVisitor printer;
    for (auto* key : getRegistry()) {
        key->accept(printer);
    }
}
```

## Sources

**HIGH Confidence (Official Documentation):**
- [ESP32 Preferences API](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html) - Official ESP32 Preferences documentation
- [Preferences.cpp Implementation](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.cpp) - Official source code
- [Arduino Library Creation for ESP32](https://developer.espressif.com/blog/2025/12/arduino-library-creation/) - Espressif official guide (Dec 2025)
- [Arduino Library Specification](https://arduino.github.io/arduino-cli/0.20/library-specification/) - Official Arduino library format

**MEDIUM Confidence (Verified Patterns):**
- [Factory With Self-Registering Types](https://www.cppstories.com/2018/02/factory-selfregister/) - Self-registration pattern
- [Type Erasure Part III](https://akrzemi1.wordpress.com/2013/12/11/type-erasure-part-iii/) - Type erasure implementation
- [ESP32: The C++ map container](https://techtutorialsx.com/2021/11/02/esp32-the-cpp-map-container/) - STL usage on ESP32
- [Self-registration for global objects](http://p-nand-q.com/programming/cplusplus/registering_global_objects.html) - Initialization order solutions
- [ArduinoJson](https://arduinojson.org/) - Serialization patterns for embedded C++

**Architecture Pattern References:**
- C++ self-registering objects pattern via static initialization
- Meyer's Singleton for initialization-order-safe registry
- Type erasure via virtual base class for heterogeneous containers
- Visitor pattern for type-safe heterogeneous iteration
- Facade pattern over ESP32 NVS API
