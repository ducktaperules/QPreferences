/**
 * QPreferences Namespace Groups Example
 *
 * Demonstrates:
 * - Organizing preferences into namespaces
 * - forEach() - iterate all registered preferences
 * - forEachInNamespace() - iterate keys in specific namespace
 * - factoryReset() - clear all NVS and restore defaults
 *
 * Namespaces help organize preferences by component/feature.
 * This example shows how to inspect and reset grouped preferences.
 */

#include <QPreferences/QPreferences.h>

// WiFi settings namespace
PrefKey<String, "wifi", "ssid"> wifiSSID{String("MyNetwork")};
PrefKey<String, "wifi", "password"> wifiPassword{String("")};
PrefKey<bool, "wifi", "autoConnect"> autoConnect{true};

// Display settings namespace
PrefKey<int, "display", "brightness"> brightness{100};
PrefKey<bool, "display", "nightMode"> nightMode{false};

// Sensor settings namespace
PrefKey<float, "sensor", "threshold"> threshold{1.5f};
PrefKey<int, "sensor", "interval"> interval{1000};

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("QPreferences Namespace Groups Example");
    Serial.println("======================================\n");

    // Initialize all preferences (triggers lazy load from NVS)
    QPrefs::get(wifiSSID);
    QPrefs::get(wifiPassword);
    QPrefs::get(autoConnect);
    QPrefs::get(brightness);
    QPrefs::get(nightMode);
    QPrefs::get(threshold);
    QPrefs::get(interval);

    // 1. List ALL registered preferences
    Serial.println("=== All Registered Preferences ===");
    QPrefs::forEach([](const QPreferences::PrefInfo& info) {
        Serial.printf("  %s/%s", info.namespace_name, info.key_name);
        if (info.is_dirty) Serial.print(" [dirty]");
        if (!info.is_initialized) Serial.print(" [not loaded]");
        Serial.println();
    });

    // 2. List preferences in a specific namespace
    Serial.println("\n=== WiFi Namespace Only ===");
    QPrefs::forEachInNamespace("wifi", [](const QPreferences::PrefInfo& info) {
        Serial.printf("  %s\n", info.key_name);
    });

    Serial.println("\n=== Display Namespace Only ===");
    QPrefs::forEachInNamespace("display", [](const QPreferences::PrefInfo& info) {
        Serial.printf("  %s\n", info.key_name);
    });

    // 3. Count keys per namespace
    Serial.println("\n=== Keys Per Namespace ===");
    int wifiCount = 0, displayCount = 0, sensorCount = 0;
    QPrefs::forEach([&](const QPreferences::PrefInfo& info) {
        if (strcmp(info.namespace_name, "wifi") == 0) wifiCount++;
        else if (strcmp(info.namespace_name, "display") == 0) displayCount++;
        else if (strcmp(info.namespace_name, "sensor") == 0) sensorCount++;
    });
    Serial.printf("  wifi: %d keys\n", wifiCount);
    Serial.printf("  display: %d keys\n", displayCount);
    Serial.printf("  sensor: %d keys\n", sensorCount);

    // 4. Modify some values
    Serial.println("\n=== Modifying Values ===");
    QPrefs::set(brightness, 75);
    QPrefs::set(threshold, 2.0f);
    QPrefs::save();  // Persist changes
    Serial.println("Modified brightness=75, threshold=2.0 and saved");

    // 5. Show current values
    Serial.println("\n=== Current Values ===");
    Serial.printf("  brightness: %d\n", QPrefs::get(brightness));
    Serial.printf("  threshold: %.1f\n", QPrefs::get(threshold));

    // 6. Factory reset demonstration
    Serial.println("\n=== Factory Reset ===");
    Serial.println("Calling factoryReset() - this clears ALL NVS data!");
    QPrefs::factoryReset();

    Serial.println("\nAfter factory reset:");
    Serial.printf("  brightness: %d (default: 100)\n", QPrefs::get(brightness));
    Serial.printf("  threshold: %.1f (default: 1.5)\n", QPrefs::get(threshold));

    Serial.println("\nFactory reset complete - all values restored to defaults.");
    Serial.println("Reboot to verify NVS is cleared.");
}

void loop() {
    delay(1000);
}
