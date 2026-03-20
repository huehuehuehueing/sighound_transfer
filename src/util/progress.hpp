// src/util/progress.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#pragma once
#include <cstdint>

namespace sht {

class ProgressReporter {
public:
    explicit ProgressReporter(bool enabled, double sample_rate);

    // Call after each block; samples_total=0 means unlimited.
    void update(int64_t samples_captured, int64_t samples_total,
                bool sample_loss_occurred);

    // Print final summary line.
    void finish(int64_t samples_captured, double elapsed_seconds);

private:
    bool   enabled_;
    double sample_rate_;
    int    last_pct_ = -1;
};

} // namespace sht
