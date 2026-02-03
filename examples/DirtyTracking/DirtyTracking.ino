/**
 * QPreferences Dirty Tracking Example
 *
 * Demonstrates:
 * - isDirty(key) - check if RAM differs from NVS
 * - isModified(key) - check if value differs from default
 * - save(key) - persist single key with default removal
 * - save() - persist all dirty keys in batch
 *
 * This example shows how changes are tracked in RAM
 * and only written to flash when you call save().
 */

#include <QPreferences/QPreferences.h>

// Define preference keys in different states
PrefKey<int, "dirty", "counter"> counter{0};
PrefKey<float, "dirty", "temperature"> temperature{25.0f};
PrefKey<bool, "dirty", "enabled"> enabled{false};

void printStatus(const char* label) {
    Serial.printf("\n--- %s ---\n", label);
    Serial.printf("counter:     value=%d, isDirty=%s, isModified=%s\n",
        QPrefs::get(counter),
        QPrefs::isDirty(counter) ? "YES" : "no",
        QPrefs::isModified(counter) ? "YES" : "no");
    Serial.printf("temperature: value=%.1f, isDirty=%s, isModified=%s\n",
        QPrefs::get(temperature),
        QPrefs::isDirty(temperature) ? "YES" : "no",
        QPrefs::isModified(temperature) ? "YES" : "no");
    Serial.printf("enabled:     value=%s, isDirty=%s, isModified=%s\n",
        QPrefs::get(enabled) ? "true" : "false",
        QPrefs::isDirty(enabled) ? "YES" : "no",
        QPrefs::isModified(enabled) ? "YES" : "no");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("QPreferences Dirty Tracking Example");
    Serial.println("====================================");

    // 1. Initial state - values loaded from NVS (or defaults)
    printStatus("After loading from NVS");

    // 2. Modify values in RAM (no flash write yet!)
    Serial.println("\nModifying values in RAM...");
    QPrefs::set(counter, QPrefs::get(counter) + 1);
    QPrefs::set(temperature, 30.5f);
    QPrefs::set(enabled, true);

    printStatus("After set() - changes in RAM only");

    // 3. Save individual key (with default removal)
    Serial.println("\nSaving counter with save(key)...");
    QPrefs::save(counter);
    printStatus("After save(counter)");

    // 4. Reset one value back to default
    Serial.println("\nSetting enabled back to default (false)...");
    QPrefs::set(enabled, false);
    printStatus("After reset to default");

    // 5. Save all remaining dirty values
    Serial.println("\nSaving all dirty values with save()...");
    QPrefs::save();
    printStatus("After save() - all clean");

    // 6. Demonstrate isModified vs isDirty
    Serial.println("\n=== isModified vs isDirty ===");
    Serial.println("isModified = value != default");
    Serial.println("isDirty = RAM != NVS (unsaved changes)");

    QPrefs::set(counter, 0);  // Back to default
    Serial.printf("\ncounter set to 0 (default):\n");
    Serial.printf("  isModified: %s (same as default)\n",
        QPrefs::isModified(counter) ? "YES" : "no");
    Serial.printf("  isDirty: %s (RAM differs from NVS)\n",
        QPrefs::isDirty(counter) ? "YES" : "no");

    Serial.println("\nReboot to see persistence in action!");
}

void loop() {
    delay(1000);
}
