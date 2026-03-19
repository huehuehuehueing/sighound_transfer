// tests/test_sigmf_metadata.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "io/sigmf_writer.hpp"
#include <nlohmann/json.hpp>

using namespace sht;
using json = nlohmann::json;

TEST_CASE("build_sigmf_meta produces required fields", "[sigmf]") {
    SigMfMeta meta;
    meta.center_freq_hz     = 433.92e6;
    meta.sample_rate_hz     = 1e6;
    meta.bandwidth_hz       = 800e3;
    meta.device_type        = "BB60C";
    meta.serial_number      = 123456;
    meta.format             = "cf32_le";
    meta.capture_timestamp  = "2026-03-19T12:00:00Z";
    meta.tool_version       = "0.1.0";
    meta.sdk_version        = "5.0.9";

    auto j = build_sigmf_meta(meta);

    REQUIRE(j.contains("global"));
    REQUIRE(j["global"].contains("core:sample_rate"));
    REQUIRE(j["global"]["core:sample_rate"].get<double>() == Catch::Approx(1e6));
    REQUIRE(j["global"].contains("core:datatype"));
    REQUIRE(j["global"]["core:datatype"].get<std::string>() == "cf32_le");

    REQUIRE(j.contains("captures"));
    REQUIRE(j["captures"].is_array());
    REQUIRE(!j["captures"].empty());
    REQUIRE(j["captures"][0].contains("core:frequency"));
    REQUIRE(j["captures"][0]["core:frequency"].get<double>() == Catch::Approx(433.92e6));

    REQUIRE(j.contains("annotations"));
}

TEST_CASE("build_sigmf_meta includes Signal Hound extension fields", "[sigmf]") {
    SigMfMeta meta;
    meta.center_freq_hz = 1e9;
    meta.sample_rate_hz = 20e6;
    meta.device_type    = "BB60D";
    meta.serial_number  = 7654321;
    meta.format         = "cf32_le";

    auto j = build_sigmf_meta(meta);

    // Signal Hound extension namespace
    REQUIRE(j["global"].contains("signalhound:device_type"));
    REQUIRE(j["global"]["signalhound:device_type"].get<std::string>() == "BB60D");
    REQUIRE(j["global"]["signalhound:serial_number"].get<int>() == 7654321);
}
