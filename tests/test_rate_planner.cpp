// tests/test_rate_planner.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/rate_planner.hpp"

using namespace sht;

TEST_CASE("plan_rate exact 40 MHz", "[rate_planner]") {
    auto p = plan_rate(40e6);
    REQUIRE(p.decimation == 1);
    REQUIRE_THAT(p.actual_sample_rate, Catch::Matchers::WithinRel(40e6, 1e-9));
}

TEST_CASE("plan_rate exact 20 MHz", "[rate_planner]") {
    auto p = plan_rate(20e6);
    REQUIRE(p.decimation == 2);
}

TEST_CASE("plan_rate exact 10 MHz", "[rate_planner]") {
    auto p = plan_rate(10e6);
    REQUIRE(p.decimation == 4);
}

TEST_CASE("plan_rate nearest: 15 MHz snaps to 20 MHz (D=2)", "[rate_planner]") {
    // 15 MHz is between D=2 (20 MHz) and D=4 (10 MHz)
    // |40/2 - 15| = 5, |40/4 - 15| = 5 -- tie goes to faster (smaller D)
    auto p = plan_rate(15e6);
    REQUIRE(p.decimation == 2);
    REQUIRE_THAT(p.actual_sample_rate, Catch::Matchers::WithinRel(20e6, 1e-9));
    REQUIRE(p.clamped == false);
}

TEST_CASE("plan_rate nearest: 6 MHz snaps to 5 MHz (D=8)", "[rate_planner]") {
    auto p = plan_rate(6e6);
    REQUIRE(p.decimation == 8);
    REQUIRE_THAT(p.actual_sample_rate, Catch::Matchers::WithinRel(5e6, 1e-9));
    REQUIRE(p.clamped == false);
}

TEST_CASE("plan_rate clamps at minimum (40 MHz)", "[rate_planner]") {
    auto p = plan_rate(100e6);
    REQUIRE(p.decimation == 1);
    REQUIRE(p.clamped == true);
}

TEST_CASE("plan_rate clamps at maximum decimation", "[rate_planner]") {
    auto p = plan_rate(1.0);  // way too slow
    REQUIRE(p.decimation == 8192);
    REQUIRE(p.clamped == true);
}

TEST_CASE("plan_rate actual_sample_rate is 40e6/decimation", "[rate_planner]") {
    for (int d : {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192}) {
        auto p = plan_rate(40e6 / d);
        REQUIRE(p.decimation == d);
        REQUIRE_THAT(p.actual_sample_rate, Catch::Matchers::WithinRel(40e6 / d, 1e-9));
        REQUIRE(p.clamped == false);
    }
}
