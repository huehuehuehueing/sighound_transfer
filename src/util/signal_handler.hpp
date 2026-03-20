// src/util/signal_handler.hpp
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

// Install SIGINT and SIGTERM handlers.
// After a signal is received, stop_requested() returns true.
void install_signal_handlers();

// Returns true if SIGINT or SIGTERM has been received.
bool stop_requested();

} // namespace sht
