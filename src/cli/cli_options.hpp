// src/cli/cli_options.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include <string>
#include <stdexcept>
#include <cstdint>

namespace sht {

enum class OpMode  { None, Receive, Transmit };
enum class IqFormat { CF32, CS16, SigMF };

struct Config {
    // Mode
    OpMode      mode            = OpMode::None;
    bool        list_devices    = false;

    // HackRF-compat receive options
    std::string output_file;
    std::string transmit_file;
    double      center_freq_hz        = 0.0;
    double      requested_sample_rate = 40e6;
    int64_t     num_samples           = 0;     // 0 = unlimited
    double      bandwidth_hz          = 0.0;   // 0 = auto
    bool        amp_enable            = false; // -a (rejected on BB60)
    int         tx_vga_gain_db        = -1;    // -x (rejected on BB60)

    // Signal Hound native options
    uint32_t    serial_number         = 0;     // 0 = first device
    int         device_index          = 0;
    double      ref_level_dbm         = -20.0;
    int         gain                  = -1;    // BB_AUTO_GAIN
    int         atten                 = -1;    // BB_AUTO_ATTEN
    IqFormat    format                = IqFormat::CF32;
    int         explicit_decimation   = 0;     // 0 = derive from sample rate
    double      duration_seconds      = 0.0;   // 0 = unlimited
    bool        overwrite             = false;
    bool        quiet                 = false;
    bool        progress              = true;
    bool        show_diagnostics      = false;
    bool        stdout_mode           = false;
    std::string metadata_path;
    int         block_size            = 262144;
    std::string trigger_mode          = "none"; // none/rising/falling
    int         trigger_sentinel      = -1;
    std::string timestamp_mode        = "system"; // none/system/gps
};

class CliError : public std::runtime_error {
public:
    explicit CliError(const std::string& msg) : std::runtime_error(msg) {}
};

// Parse argc/argv into Config.
// Throws CliError on invalid input or missing required options.
// Does NOT open hardware or validate hardware-specific limits.
Config parse_cli(int argc, char* argv[]);

// Print usage to stdout.
void print_help(const char* progname);

} // namespace sht
