#include <QPreferences.h>

PrefKey<int, "test", "counter"> counterKey{0};
PrefKey<bool, "test", "enabled"> enabledKey{false};

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== QPreferences Cache Test ===\n");

    // Test 1: isModified false for default value
    Serial.println("Test 1: Initial get() should return default");
    int val = QPrefs::get(counterKey);
    Serial.printf("  Initial: %d, isModified: %d, isDirty: %d\n",
                  val, QPrefs::isModified(counterKey), QPrefs::isDirty(counterKey));
    Serial.println("  Expected: 0, false (0), false (0)");
    Serial.println();

    // Test 2: set() changes value in RAM
    Serial.println("Test 2: set(42) should update RAM and mark dirty");
    QPrefs::set(counterKey, 42);
    val = QPrefs::get(counterKey);
    Serial.printf("  After set(42): %d, isModified: %d, isDirty: %d\n",
                  val, QPrefs::isModified(counterKey), QPrefs::isDirty(counterKey));
    Serial.println("  Expected: 42, true (1), true (1)");
    Serial.println();

    // Test 3: Multiple sets update RAM only
    Serial.println("Test 3: 100 sets should all update RAM without NVS writes");
    for (int i = 0; i < 100; i++) {
        QPrefs::set(counterKey, i);
    }
    Serial.printf("  After 100 sets: %d\n", QPrefs::get(counterKey));
    Serial.println("  Expected: 99");
    Serial.println();

    // Test 4: isModified vs isDirty distinction
    Serial.println("Test 4: set() to default value");
    QPrefs::set(counterKey, 0);
    val = QPrefs::get(counterKey);
    Serial.printf("  After set(0): %d, isModified: %d, isDirty: %d\n",
                  val, QPrefs::isModified(counterKey), QPrefs::isDirty(counterKey));
    Serial.println("  Expected: 0, false (0), true (1)");
    Serial.println("  (value equals default but differs from NVS)");
    Serial.println();

    // Test 5: Boolean test
    Serial.println("Test 5: Boolean preference");
    bool enabled = QPrefs::get(enabledKey);
    Serial.printf("  Initial enabled: %d, isModified: %d, isDirty: %d\n",
                  enabled, QPrefs::isModified(enabledKey), QPrefs::isDirty(enabledKey));
    Serial.println("  Expected: false (0), false (0), false (0)");

    QPrefs::set(enabledKey, true);
    enabled = QPrefs::get(enabledKey);
    Serial.printf("  After set(true): %d, isModified: %d, isDirty: %d\n",
                  enabled, QPrefs::isModified(enabledKey), QPrefs::isDirty(enabledKey));
    Serial.println("  Expected: true (1), true (1), true (1)");
    Serial.println();

    Serial.println("=== Tests Complete ===");
    Serial.println("NOTE: set() does NOT write to NVS - all changes are in RAM only");
}

void loop() {
    // Nothing to do
}
