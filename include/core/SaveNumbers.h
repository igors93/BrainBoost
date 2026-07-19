#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

// Shared numeric parsing rules for the save format. Structural validation
// (SaveManager) and deserialization (UserProfile, Statistics) must use these
// same helpers so a file that validates can never lose data silently while
// being loaded.
//
// Policy for integer fields:
// - the whole string must be a base-10 integer — anything else is rejected;
// - required fields use the non-negative variants: a leading '-' indicates
//   corruption and marks the file as corrupted;
// - values outside a field's range saturate to the nearest bound, so a save
//   written with wider limits keeps as much progress as possible instead of
//   being zeroed. save.version is the exception: it uses the exact variant
//   because a version that overflows is garbage, not a future version;
// - optional record fields (history entries) use parseSaturating, which
//   accepts negatives and normalizes them to the field minimum.
//
// Policy for floating-point fields: must parse fully and be finite (NaN and
// infinity are rejected); range clamping is done by the caller.
namespace savenum {

// Parses a base-10 integer with an optional leading '-', saturating values
// outside [minimum, maximum] to the nearest bound. Returns false when the
// text is not a pure integer.
inline bool parseSaturating(const std::string& text, std::int64_t minimum,
                            std::int64_t maximum, std::int64_t& out) {
    if (text.empty() || minimum > maximum) return false;

    std::size_t index = 0;
    const bool negative = text[0] == '-';
    if (negative) index = 1;
    if (index >= text.size()) return false;

    bool overflowed = false;
    std::int64_t magnitude = 0;
    for (; index < text.size(); ++index) {
        const char character = text[index];
        if (character < '0' || character > '9') return false;
        const int digit = character - '0';
        if (!overflowed) {
            if (magnitude > (INT64_MAX - digit) / 10) {
                overflowed = true;
            } else {
                magnitude = magnitude * 10 + digit;
            }
        }
    }

    std::int64_t value;
    if (negative) {
        value = overflowed ? INT64_MIN : -magnitude;
    } else {
        value = overflowed ? INT64_MAX : magnitude;
    }
    out = std::clamp(value, minimum, maximum);
    return true;
}

// As parseSaturating, but for fields stored as int.
inline bool parseSaturatingInt(const std::string& text, int minimum,
                               int maximum, int& out) {
    std::int64_t wide = 0;
    if (!parseSaturating(text, minimum, maximum, wide)) return false;
    out = static_cast<int>(wide);
    return true;
}

// Parses a non-negative base-10 integer, saturating values above `maximum`.
// Returns false when `text` is empty, has non-digit characters or a sign.
inline bool parseNonNegative(const std::string& text, std::int64_t maximum,
                             std::int64_t& out) {
    if (!text.empty() && text[0] == '-') return false;
    return parseSaturating(text, 0, maximum, out);
}

// As parseNonNegative, but for fields stored as int.
inline bool parseNonNegativeInt(const std::string& text, int maximum, int& out) {
    std::int64_t wide = 0;
    if (!parseNonNegative(text, maximum, wide)) return false;
    out = static_cast<int>(wide);
    return true;
}

// Strict variant: the value must fit in [0, maximum] exactly; overflow is an
// error instead of saturating. Used for save.version.
inline bool parseNonNegativeExact(const std::string& text, std::int64_t maximum,
                                  std::int64_t& out) {
    std::int64_t value = 0;
    if (!parseNonNegative(text, INT64_MAX, value) || value > maximum) return false;
    out = value;
    return true;
}

// Parses a float that consumes the whole string and is finite.
inline bool parseFiniteFloat(const std::string& text, float& out) {
    try {
        std::size_t parsed = 0;
        const float value = std::stof(text, &parsed);
        if (parsed != text.size() || !std::isfinite(value)) return false;
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace savenum
