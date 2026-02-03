/**
 * QPreferences Basic Usage Example
 *
 * Demonstrates:
 * - Defining type-safe preference keys with PrefKey<T>
 * - Compile-time validation of namespace/key length (<=15 chars)
 * - Unified get/set API with automatic type deduction
 *
 * This example shows the complete Phase 1 API for QPreferences.
 * Preferences are stored in ESP32 NVS (Non-Volatile Storage).
 */

#include <QPreferences/QPreferences.h>

// Define preference keys - these are compile-time validated
// Namespace and key names must be <=15 characters (NVS limit)
// Format: PrefKey<Type, "namespace", "key">{default_value}

PrefKey<int, "myapp", "bootCount"> bootCount{0};
PrefKey<float, "myapp", "threshold"> threshold{1.5f};
PrefKey<bool, "myapp", "ledEnabled"> ledEnabled{true};
PrefKey<String, "myapp", "deviceName"> deviceName{String("ESP32")};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("QPreferences Basic Usage Example");
  Serial.println("================================");
  Serial.println();

  // Get current values (returns default if not stored)
  // Note: get() returns the correct type automatically!
  int count = QPrefs::get(bootCount);
  Serial.printf("Boot count: %d\n", count);

  // Increment and save the boot count
  QPrefs::set(bootCount, count + 1);

  // Read other preferences - each returns its declared type
  float thresh = QPrefs::get(threshold);
  bool led = QPrefs::get(ledEnabled);
  String name = QPrefs::get(deviceName);

  Serial.printf("Threshold: %.2f\n", thresh);
  Serial.printf("LED enabled: %s\n", led ? "true" : "false");
  Serial.printf("Device name: %s\n", name.c_str());

  // Demonstrate setting each type
  Serial.println("\nUpdating preferences...");

  QPrefs::set(threshold, 2.0f);
  QPrefs::set(ledEnabled, false);
  QPrefs::set(deviceName, String("MyESP32"));

  // Read back to verify
  Serial.printf("New threshold: %.2f\n", QPrefs::get(threshold));
  Serial.printf("New LED enabled: %s\n", QPrefs::get(ledEnabled) ? "true" : "false");
  Serial.printf("New device name: %s\n", QPrefs::get(deviceName).c_str());

  // ============================================
  // TYPE SAFETY DEMONSTRATION
  // ============================================
  // The following lines would NOT compile if uncommented.
  // This is the key benefit of QPreferences - type mismatches
  // are caught at compile time, not runtime!

  // QPrefs::set(bootCount, 3.14f);     // Error: cannot convert float to int
  // QPrefs::set(threshold, "hello");   // Error: cannot convert const char* to float
  // QPrefs::set(ledEnabled, 42);       // Error: cannot convert int to bool
  // int x = QPrefs::get(deviceName);   // Error: cannot convert String to int

  // ============================================
  // COMPILE-TIME LENGTH VALIDATION
  // ============================================
  // The following would fail at compile time because
  // namespace/key names exceed 15 characters:

  // PrefKey<int, "thisNamespaceIsTooLong", "key"> invalid1{0};  // Error!
  // PrefKey<int, "ns", "thisKeyNameIsTooLong"> invalid2{0};     // Error!

  Serial.println("\nReboot to see boot count increase!");
}

void loop() {
  delay(1000);
}
