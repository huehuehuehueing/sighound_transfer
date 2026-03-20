// src/core/bandwidth_table.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
//
// Maximum IQ bandwidth per decimation factor.
// Source: Signal Hound BB60 SDK documentation,
//   section "I/Q Filtering and Bandwidth Limitations"
//   (device_apis/bb_series/docs/html/index.html).
// Verified against SDK HTML docs for the 2026-02-19 release.
// If this table needs updating, re-check the HTML docs for your SDK version.
//
#include "core/bandwidth_table.hpp"
#include <array>

namespace sht {

struct BwEntry {
    int    decimation;
    double max_bw_hz;
};

// Table ordered by ascending decimation.
static constexpr std::array<BwEntry, 14> k_bw_table = {{
    {    1, 27'000'000.0},
    {    2, 17'800'000.0},
    {    4,  8'000'000.0},
    {    8,  3'750'000.0},
    {   16,  2'000'000.0},
    {   32,  1'000'000.0},
    {   64,    500'000.0},
    {  128,    250'000.0},
    {  256,    140'000.0},
    {  512,     65'000.0},
    { 1024,     30'000.0},
    { 2048,     15'000.0},
    { 4096,      8'000.0},
    { 8192,      4'000.0},
}};

double max_bandwidth_for_decimation(int decimation) {
    for (const auto& e : k_bw_table) {
        if (e.decimation == decimation) return e.max_bw_hz;
    }
    return -1.0;
}

BwStatus validate_bandwidth(int decimation, double bandwidth_hz) {
    double max_bw = max_bandwidth_for_decimation(decimation);
    if (max_bw < 0.0) return BwStatus::InvalidDecimation;
    if (bandwidth_hz <= 0.0) return BwStatus::TooWide;  // zero/negative BW is not valid
    if (bandwidth_hz > max_bw) return BwStatus::TooWide;
    return BwStatus::OK;
}

double default_bandwidth(int decimation) {
    double max_bw = max_bandwidth_for_decimation(decimation);
    if (max_bw < 0.0) return -1.0;
    return max_bw * 0.8;
}

} // namespace sht
