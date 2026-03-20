// tests/test_cli_options.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "cli/cli_options.hpp"
#include <vector>
#include <string>

using namespace sht;

// Helper: build argv from initializer list
struct FakeArgv {
    std::vector<std::string> storage;
    std::vector<char*> argv;
    FakeArgv(std::initializer_list<const char*> args) {
        storage.assign(args.begin(), args.end());
        for (auto& s : storage) argv.push_back(s.data());
    }
    FakeArgv(const FakeArgv&) = delete;
    FakeArgv& operator=(const FakeArgv&) = delete;
    int argc() { return static_cast<int>(argv.size()); }
    char** data() { return argv.data(); }
};

TEST_CASE("receive file flag sets output_file", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.output_file == "out.bin");
    REQUIRE(cfg.mode == OpMode::Receive);
}

TEST_CASE("freq flag sets center_freq_hz", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "2400000000"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.center_freq_hz == 2400000000.0);
}

TEST_CASE("sample rate flag sets requested_sample_rate", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "-s", "10000000"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.requested_sample_rate == 10e6);
}

TEST_CASE("num samples flag sets num_samples", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "-n", "1048576"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.num_samples == 1048576LL);
}

TEST_CASE("transmit flag sets transmit mode", "[cli]") {
    FakeArgv a{"sighound_transfer", "-t", "tx.bin"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.mode == OpMode::Transmit);
    REQUIRE(cfg.transmit_file == "tx.bin");
}

TEST_CASE("list-devices option sets list_devices", "[cli]") {
    FakeArgv a{"sighound_transfer", "--list-devices"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.list_devices == true);
}

TEST_CASE("serial option sets serial_number", "[cli]") {
    FakeArgv a{"sighound_transfer", "--serial", "123456", "-r", "out.bin", "-f", "1e9"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.serial_number == 123456);
}

TEST_CASE("ref-level option sets ref_level_dbm", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--ref-level", "-20"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.ref_level_dbm == -20.0);
}

TEST_CASE("format option cf32 sets format", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--format", "cf32"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.format == IqFormat::CF32);
}

TEST_CASE("format option cs16 sets format", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--format", "cs16"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.format == IqFormat::CS16);
}

TEST_CASE("format option sigmf sets format", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--format", "sigmf"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.format == IqFormat::SigMF);
}

TEST_CASE("decimation option sets explicit_decimation", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--decimation", "4"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.explicit_decimation == 4);
}

TEST_CASE("quiet option sets quiet mode", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--quiet"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.quiet == true);
    REQUIRE(cfg.progress == false);
}

TEST_CASE("duration option sets duration_seconds", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--duration", "5.5"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.duration_seconds == Catch::Approx(5.5));
}

TEST_CASE("overwrite option sets overwrite flag", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--overwrite"};
    auto cfg = parse_cli(a.argc(), a.data());
    REQUIRE(cfg.overwrite == true);
}

TEST_CASE("no receive and no list-devices triggers parse error", "[cli]") {
    FakeArgv a{"sighound_transfer", "-f", "1e9"};
    REQUIRE_THROWS_AS(parse_cli(a.argc(), a.data()), CliError);
}

TEST_CASE("freq flag required for receive mode", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin"};
    REQUIRE_THROWS_AS(parse_cli(a.argc(), a.data()), CliError);
}

TEST_CASE("invalid format value throws CliError", "[cli]") {
    FakeArgv a{"sighound_transfer", "-r", "out.bin", "-f", "1e9", "--format", "wav"};
    REQUIRE_THROWS_AS(parse_cli(a.argc(), a.data()), CliError);
}
