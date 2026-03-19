// src/util/progress.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include "util/progress.hpp"
#include <cstdio>

namespace sht {

ProgressReporter::ProgressReporter(bool enabled, double sample_rate)
    : enabled_(enabled), sample_rate_(sample_rate) {}

void ProgressReporter::update(int64_t samples_captured, int64_t samples_total,
                               bool sample_loss_occurred) {
    if (!enabled_) return;

    if (sample_loss_occurred) {
        std::fprintf(stderr, "\nWarning: sample loss detected\n");
    }

    if (samples_total > 0) {
        int pct = static_cast<int>(100.0 * samples_captured / samples_total);
        if (pct != last_pct_) {
            last_pct_ = pct;
            std::fprintf(stderr, "\rCapturing: %d%% (%lld / %lld samples)",
                         pct,
                         static_cast<long long>(samples_captured),
                         static_cast<long long>(samples_total));
            std::fflush(stderr);
        }
    } else {
        // Unlimited: show MB captured and sample rate
        double mb = static_cast<double>(samples_captured) * 8.0 / (1024.0 * 1024.0);
        std::fprintf(stderr, "\rCapturing: %.1f MB (%.0f ksps)",
                     mb, sample_rate_ / 1000.0);
        std::fflush(stderr);
    }
}

void ProgressReporter::finish(int64_t samples_captured, double elapsed_seconds) {
    if (!enabled_) return;
    std::fprintf(stderr, "\n");  // End the \r line
    double actual_rate = (elapsed_seconds > 0) ? samples_captured / elapsed_seconds : 0;
    std::printf("Captured %lld samples in %.2f s (%.3f Msps)\n",
                static_cast<long long>(samples_captured),
                elapsed_seconds,
                actual_rate / 1e6);
}

} // namespace sht
