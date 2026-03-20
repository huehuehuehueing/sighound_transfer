// tests/test_raw_writer.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include <catch2/catch_test_macros.hpp>
#include "io/raw_writer.hpp"
#include <cstdio>
#include <vector>
#include <cstring>

using namespace sht;

TEST_CASE("RawWriter rejects SigMF format", "[raw_writer]") {
    REQUIRE_THROWS_AS(RawWriter("/tmp/test_sigmf.bin", IqFormat::SigMF, true),
                      std::runtime_error);
}

TEST_CASE("RawWriter write then finalize tracks samples", "[raw_writer]") {
    const char* path = "/tmp/test_raw_writer_cf32.bin";
    {
        RawWriter w(path, IqFormat::CF32, true);
        float data[4] = {1.0f, 2.0f, 3.0f, 4.0f};  // 2 complex samples
        w.write(data, 2);
        REQUIRE(w.samples_written() == 2);
        w.finalize();
        REQUIRE(w.samples_written() == 2);
    }
    std::remove(path);
}

TEST_CASE("RawWriter write to closed file throws", "[raw_writer]") {
    const char* path = "/tmp/test_raw_writer_closed.bin";
    RawWriter w(path, IqFormat::CF32, true);
    w.finalize();
    float data[2] = {1.0f, 2.0f};
    REQUIRE_THROWS_AS(w.write(data, 1), std::runtime_error);
    std::remove(path);
}

TEST_CASE("RawWriter skips write when count is zero", "[raw_writer]") {
    const char* path = "/tmp/test_raw_writer_zero.bin";
    RawWriter w(path, IqFormat::CF32, true);
    float data[2] = {1.0f, 2.0f};
    w.write(data, 0);
    REQUIRE(w.samples_written() == 0);
    std::remove(path);
}

TEST_CASE("RawWriter rejects existing file without overwrite", "[raw_writer]") {
    const char* path = "/tmp/test_raw_writer_nooverwrite.bin";
    // Create the file first
    RawWriter w1(path, IqFormat::CF32, true);
    w1.finalize();
    // Now try without overwrite
    REQUIRE_THROWS_AS(RawWriter(path, IqFormat::CF32, false), std::runtime_error);
    std::remove(path);
}
