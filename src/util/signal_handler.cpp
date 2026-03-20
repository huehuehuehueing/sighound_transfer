// src/util/signal_handler.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include "util/signal_handler.hpp"
#include <csignal>
#include <atomic>

namespace sht {

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/) {
    // relaxed: flag is poll-only; the capture loop needs no ordering
    // guarantee relative to other memory ops — just eventual visibility.
    g_stop.store(true, std::memory_order_relaxed);
}

void install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // restart syscalls interrupted by signal
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

bool stop_requested() {
    return g_stop.load(std::memory_order_relaxed);  // poll-only; see store comment
}

} // namespace sht
