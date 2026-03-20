// src/core/rate_planner.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#pragma once

namespace sht {

struct RatePlan {
    int     decimation;         // Power of 2 in [1, 8192]
    double  actual_sample_rate; // 40e6 / decimation
    bool    clamped;            // true if requested rate was out of supported range
};

// Find the nearest supported BB60 decimation for requested_hz.
// If the request is outside [4882.8125, 40e6], clamps and sets plan.clamped=true.
// On ties (equal distance to two supported rates), prefers the faster one (smaller D).
RatePlan plan_rate(double requested_hz);

} // namespace sht
