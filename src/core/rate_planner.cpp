// src/core/rate_planner.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include "core/rate_planner.hpp"
#include <cmath>
#include <array>

namespace sht {

// All valid decimation factors for BB60 IQ streaming, sorted ascending.
static constexpr std::array<int, 14> k_decimations = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
};

static constexpr double k_base_rate = 40.0e6;

RatePlan plan_rate(double requested_hz) {
    RatePlan best{};
    best.clamped = false;

    double best_diff = 1e99;
    for (int d : k_decimations) {
        double rate = k_base_rate / d;
        double diff = std::abs(rate - requested_hz);
        // On tie, prefer faster rate (smaller d) — k_decimations is sorted ascending
        // so the first match at a given distance is the fastest.
        if (diff < best_diff) {
            best_diff = diff;
            best.decimation = d;
            best.actual_sample_rate = rate;
        }
    }

    // Check clamping
    double min_rate = k_base_rate / k_decimations.back();
    double max_rate = k_base_rate / k_decimations.front();
    if (requested_hz > max_rate || requested_hz < min_rate) {
        best.clamped = true;
    }

    return best;
}

} // namespace sht
