// src/bb/bb_error.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include "bb_api.h"
#include <stdexcept>
#include <string>

namespace sht {

class BbError : public std::runtime_error {
public:
    const bbStatus status;
    explicit BbError(bbStatus s, const std::string& context = "")
        : std::runtime_error(
              (context.empty() ? "" : context + ": ") +
              std::string(
                  [](const char* raw) { return raw ? raw : "(null error string)"; }(bbGetErrorString(s))))
        , status(s) {}
};

// Throw BbError if status is a real error (negative).
// Warnings (positive non-zero) are not thrown — caller can check return value.
[[nodiscard]] inline bbStatus bb_check(bbStatus s, const char* context = "") {
    if (s < bbNoError) throw BbError(s, context);
    return s;
}

// Convert BB device type integer to human-readable string.
inline const char* bb_device_type_name(int device_type) noexcept {
    switch (device_type) {
        case BB_DEVICE_BB60A: return "BB60A";
        case BB_DEVICE_BB60C: return "BB60C";
        case BB_DEVICE_BB60D: return "BB60D";
        default:              return "Unknown";
    }
}

} // namespace sht
