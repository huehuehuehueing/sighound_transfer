// src/core/bandwidth_table.hpp
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

enum class BwStatus {
    OK,
    TooWide,
    InvalidDecimation,
};

// Return max allowed bandwidth (Hz) for a given BB60 decimation factor.
// Returns -1.0 if decimation is not a valid BB60 value.
// Source: Signal Hound BB60 SDK documentation,
//   section "I/Q Filtering and Bandwidth Limitations"
//   (device_apis/bb_series/docs/html/index.html).
double max_bandwidth_for_decimation(int decimation);

// Validate a user-requested bandwidth against the decimation constraint.
BwStatus validate_bandwidth(int decimation, double bandwidth_hz);

// Return a sensible default bandwidth for a given decimation (80% of max).
double default_bandwidth(int decimation);

} // namespace sht
