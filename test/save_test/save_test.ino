/**
 * @file save_test.ino
 * @brief Test sketch for QPreferences save() API.
 *
 * Tests:
 * 1. Batch save() - all dirty keys saved in single operation
 * 2. Per-key save(key) - single key with default removal
 * 3. Namespace batching - multiple keys in same namespace
 * 4. Reboot persistence - values survive power cycle
 *
 * Instructions:
 * 1. Upload and run - observe test output
 * 2. Press reset button or power cycle
 * 3. After reboot, test detects persisted value and reports SUCCESS
 */

#include <QPreferences.h>

// Define test keys in same namespace for batching test
PrefKey<int, "test", "count"> testCount{0};
PrefKey<bool, "test", "flag"> testFlag{false};
PrefKey<int, "test", "value"> testValue{100};
// Different namespace to verify namespace switching
PrefKey<String, "other", "name"> otherName{""};

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== Save Test ===\n");

    // Check if this is a reboot test (value should persist)
    int initialCount = QPrefs::get(testCount);
    Serial.printf("Initial count (from NVS or default): %d\n", initialCount);

    if (initialCount == 42) {
        Serial.println("\n*** SUCCESS: Value persisted across reboot! ***\n");
        Serial.println("Resetting to default for next test run...");
        QPrefs::set(testCount, 0);
        QPrefs::set(testFlag, false);
        QPrefs::set(testValue, 100);
        QPrefs::set(otherName, "");
        QPrefs::save();  // Batch save to reset
        Serial.println("Reset complete. Test passed.\n");
        Serial.println("=== REBOOT TEST PASSED ===");
        return;
    }

    // Test 1: Set multiple values and verify dirty
    Serial.println("--- Test 1: Set values ---");
    QPrefs::set(testCount, 42);
    QPrefs::set(testFlag, true);
    QPrefs::set(testValue, 200);
    QPrefs::set(otherName, "hello");
    Serial.printf("isDirty(count): %d (expect 1)\n", QPrefs::isDirty(testCount));
    Serial.printf("isDirty(flag): %d (expect 1)\n", QPrefs::isDirty(testFlag));
    Serial.printf("isDirty(value): %d (expect 1)\n", QPrefs::isDirty(testValue));
    Serial.printf("isDirty(name): %d (expect 1)\n", QPrefs::isDirty(otherName));
    Serial.println();

    // Test 2: Batch save() - all dirty keys saved in single operation
    Serial.println("--- Test 2: Batch save() ---");
    Serial.println("Calling QPrefs::save() to persist all dirty keys...");
    QPrefs::save();  // Saves all dirty keys with namespace batching
    Serial.printf("After save() - isDirty(count): %d (expect 0)\n", QPrefs::isDirty(testCount));
    Serial.printf("After save() - isDirty(flag): %d (expect 0)\n", QPrefs::isDirty(testFlag));
    Serial.printf("After save() - isDirty(value): %d (expect 0)\n", QPrefs::isDirty(testValue));
    Serial.printf("After save() - isDirty(name): %d (expect 0)\n", QPrefs::isDirty(otherName));
    Serial.println();

    // Test 3: Per-key save(key) with default removal
    Serial.println("--- Test 3: save(key) with default removal ---");
    QPrefs::set(testFlag, false);  // Back to default
    Serial.printf("Set flag to default - isDirty(flag): %d (expect 1)\n", QPrefs::isDirty(testFlag));
    QPrefs::save(testFlag);  // Should remove from NVS since equals default
    Serial.printf("After save(flag) - isDirty(flag): %d (expect 0)\n", QPrefs::isDirty(testFlag));
    Serial.println();

    // Test 4: Verify only dirty keys are saved
    Serial.println("--- Test 4: Only dirty keys saved ---");
    QPrefs::set(testCount, 99);  // Make dirty
    // testFlag is clean, testValue is clean
    Serial.printf("isDirty(count): %d (expect 1)\n", QPrefs::isDirty(testCount));
    Serial.printf("isDirty(flag): %d (expect 0)\n", QPrefs::isDirty(testFlag));
    QPrefs::set(testCount, 42);  // Change back for reboot test
    QPrefs::save();  // Only count should be written
    Serial.printf("After save() - isDirty(count): %d (expect 0)\n", QPrefs::isDirty(testCount));
    Serial.println();

    Serial.println("=== Reboot Test Instructions ===");
    Serial.println("1. Press reset button or power cycle the device");
    Serial.println("2. After reboot, count should be 42 (persisted from NVS)");
    Serial.println("3. The test will detect this and report SUCCESS");
    Serial.println("=== Test Complete - Please Reboot ===\n");
}

void loop() {
    // Nothing to do - waiting for reboot
    delay(10000);
}
