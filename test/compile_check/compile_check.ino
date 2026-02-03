/**
 * @file compile_check.ino
 * @brief Compile-time validation test for PrefKey templates.
 *
 * This sketch verifies that the PrefKey and StringLiteral templates
 * compile correctly. If this file compiles without errors, the
 * template infrastructure is working correctly.
 *
 * This is NOT a runtime test - it's a compilation verification.
 */

#include "../../src/QPreferences/PrefKey.h"
#include <Arduino.h>  // For String type

using namespace QPreferences;

// =============================================================================
// Valid PrefKey definitions - these MUST compile
// =============================================================================

// Integer preference with default value 0
PrefKey<int, "myapp", "counter"> counterKey{0};

// Float preference with default value 1.5
PrefKey<float, "myapp", "threshold"> thresholdKey{1.5f};

// Boolean preference with default value true
PrefKey<bool, "myapp", "enabled"> enabledKey{true};

// String preference with default value
PrefKey<String, "myapp", "username"> usernameKey{String("default")};

// Edge case: exactly 15 characters (should compile)
PrefKey<int, "exactly15chars_", "key"> maxNsKey{0};
PrefKey<int, "ns", "exactly15chars_"> maxKeyKey{0};

// =============================================================================
// Compile-time validation tests
// Uncomment any of these lines to verify they produce compile errors:
// =============================================================================

// ERROR: Namespace exceeds 15 characters
// PrefKey<int, "this_namespace_is_way_too_long", "x"> badNs{0};

// ERROR: Key name exceeds 15 characters
// PrefKey<int, "x", "this_key_name_is_way_too_long"> badKey{0};

// ERROR: Both namespace and key exceed 15 characters
// PrefKey<int, "namespace_too_long", "key_name_too_long"> badBoth{0};

// =============================================================================
// Setup and loop - verify member access compiles
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== PrefKey Compile Check ===");
    Serial.println();

    // Verify namespace_name access
    Serial.print("counterKey.namespace_name: ");
    Serial.println(counterKey.namespace_name);

    // Verify key_name access
    Serial.print("counterKey.key_name: ");
    Serial.println(counterKey.key_name);

    // Verify default_value access
    Serial.print("counterKey.default_value: ");
    Serial.println(counterKey.default_value);

    Serial.println();

    // Verify value_type is accessible (compile-time check)
    // This line compiles only if value_type is properly defined
    using CounterType = decltype(counterKey)::value_type;
    CounterType testValue = 42;
    Serial.print("value_type test: ");
    Serial.println(testValue);

    Serial.println();

    // Display all preference definitions
    Serial.println("All PrefKey definitions:");

    Serial.print("  int    ");
    Serial.print(counterKey.namespace_name);
    Serial.print("/");
    Serial.print(counterKey.key_name);
    Serial.print(" = ");
    Serial.println(counterKey.default_value);

    Serial.print("  float  ");
    Serial.print(thresholdKey.namespace_name);
    Serial.print("/");
    Serial.print(thresholdKey.key_name);
    Serial.print(" = ");
    Serial.println(thresholdKey.default_value);

    Serial.print("  bool   ");
    Serial.print(enabledKey.namespace_name);
    Serial.print("/");
    Serial.print(enabledKey.key_name);
    Serial.print(" = ");
    Serial.println(enabledKey.default_value ? "true" : "false");

    Serial.print("  String ");
    Serial.print(usernameKey.namespace_name);
    Serial.print("/");
    Serial.print(usernameKey.key_name);
    Serial.print(" = ");
    Serial.println(usernameKey.default_value);

    Serial.println();
    Serial.println("=== Compile Check PASSED ===");
}

void loop() {
    // Nothing to do - this is a compile-time test
    delay(10000);
}
