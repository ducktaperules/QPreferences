#ifndef QPREFERENCES_PREFKEY_H
#define QPREFERENCES_PREFKEY_H

#include "StringLiteral.h"

namespace QPreferences {

/**
 * @brief Type-safe preference key definition with compile-time validation.
 *
 * PrefKey captures a preference's type, namespace, key name, and default value
 * at compile time. The namespace and key name lengths are validated via
 * static_assert to ensure they fit within ESP32 Preferences limits.
 *
 * @tparam T The value type (int, float, bool, String, etc.)
 * @tparam Namespace The namespace name (max 15 characters)
 * @tparam Key The key name (max 15 characters)
 *
 * ESP32 Preferences limits:
 *   - Namespace: 15 characters max
 *   - Key name: 15 characters max
 *
 * Usage:
 *   PrefKey<int, "myapp", "counter"> counterKey{0};
 *   PrefKey<float, "myapp", "threshold"> thresholdKey{1.5f};
 *   PrefKey<bool, "myapp", "enabled"> enabledKey{true};
 */
template<typename T, StringLiteral Namespace, StringLiteral Key>
struct PrefKey {
    // Compile-time validation of namespace and key lengths
    static_assert(Namespace.size() <= 15, "Namespace must be 15 characters or less");
    static_assert(Key.size() <= 15, "Key name must be 15 characters or less");

    /// The value type for this preference
    using value_type = T;

    /// The namespace name as a C-string
    static constexpr const char* namespace_name = Namespace.value;

    /// The key name as a C-string
    static constexpr const char* key_name = Key.value;

    /// The default value for this preference
    T default_value;

    /**
     * @brief Construct a PrefKey with the given default value.
     * @param default_val The default value to use when preference is not set
     */
    constexpr explicit PrefKey(T default_val) : default_value(default_val) {}
};

} // namespace QPreferences

#endif // QPREFERENCES_PREFKEY_H
