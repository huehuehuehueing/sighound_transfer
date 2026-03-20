// tests/test_bandwidth.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/bandwidth_table.hpp"

using namespace sht;

TEST_CASE("max BW for D=1 is 27 MHz", "[bandwidth]") {
    REQUIRE_THAT(max_bandwidth_for_decimation(1),
                 Catch::Matchers::WithinRel(27e6, 0.01));
}

TEST_CASE("max BW for D=4 is 8 MHz", "[bandwidth]") {
    REQUIRE_THAT(max_bandwidth_for_decimation(4),
                 Catch::Matchers::WithinRel(8.0e6, 0.01));
}

TEST_CASE("max BW for D=8192 is 4 kHz", "[bandwidth]") {
    REQUIRE_THAT(max_bandwidth_for_decimation(8192),
                 Catch::Matchers::WithinRel(4000.0, 0.01));
}

TEST_CASE("invalid decimation returns -1", "[bandwidth]") {
    REQUIRE(max_bandwidth_for_decimation(3) < 0.0);
    REQUIRE(max_bandwidth_for_decimation(0) < 0.0);
    REQUIRE(max_bandwidth_for_decimation(9000) < 0.0);
}

TEST_CASE("validate_bandwidth accepts valid combination", "[bandwidth]") {
    // D=1, BW=20 MHz — within 27 MHz limit
    REQUIRE(validate_bandwidth(1, 20e6) == BwStatus::OK);
}

TEST_CASE("validate_bandwidth rejects BW > max for decimation", "[bandwidth]") {
    // D=4, BW=15 MHz — max is 8 MHz per SDK docs
    REQUIRE(validate_bandwidth(4, 15e6) == BwStatus::TooWide);
}

TEST_CASE("validate_bandwidth rejects invalid decimation", "[bandwidth]") {
    REQUIRE(validate_bandwidth(3, 1e6) == BwStatus::InvalidDecimation);
}

TEST_CASE("validate_bandwidth accepts BW exactly at max", "[bandwidth]") {
    // Boundary is inclusive: BW == max is OK, BW > max is TooWide
    REQUIRE(validate_bandwidth(1, 27e6) == BwStatus::OK);
    REQUIRE(validate_bandwidth(8192, 4000.0) == BwStatus::OK);
}

TEST_CASE("validate_bandwidth rejects zero or negative BW", "[bandwidth]") {
    REQUIRE(validate_bandwidth(1, 0.0) == BwStatus::TooWide);
    REQUIRE(validate_bandwidth(1, -1.0) == BwStatus::TooWide);
}

TEST_CASE("default_bandwidth returns 80% of max BW", "[bandwidth]") {
    double bw = default_bandwidth(1);
    REQUIRE_THAT(bw, Catch::Matchers::WithinRel(27e6 * 0.8, 0.01));
}

TEST_CASE("default_bandwidth returns -1 for invalid decimation", "[bandwidth]") {
    REQUIRE(default_bandwidth(3) < 0.0);
    REQUIRE(default_bandwidth(0) < 0.0);
}
