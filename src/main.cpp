// src/main.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include "version.hpp"
#include "cli/cli_options.hpp"
#include "bb/bb_device.hpp"
#include "bb/bb_error.hpp"
#include "core/rate_planner.hpp"
#include "core/bandwidth_table.hpp"
#include "core/capture_engine.hpp"
#include "util/signal_handler.hpp"
#include "io/sigmf_writer.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, char* argv[]) {
    using namespace sht;

    Config cfg;
    try {
        cfg = parse_cli(argc, argv);
    } catch (const CliError& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        std::fprintf(stderr, "Use -h for help.\n");
        return 1;
    }

    // --list-devices
    if (cfg.list_devices) {
        int n = list_devices();
        return (n >= 0) ? 0 : 1;
    }

    // -t (transmit): not supported on BB60
    if (cfg.mode == OpMode::Transmit) {
        std::fprintf(stderr,
            "Error: Transmit mode (-t) is not supported on Signal Hound BB60 devices.\n"
            "BB60 is a receive-only spectrum analyzer.\n"
            "Signal Hound does not produce a transmit-capable device with this API.\n");
        return 1;
    }

    // Reject HackRF-specific options that don't map to BB60
    if (cfg.amp_enable) {
        std::fprintf(stderr,
            "Error: -a (RF amp enable) is not available on BB60. "
            "Use --ref-level to adjust expected input amplitude.\n");
        return 1;
    }
    if (cfg.tx_vga_gain_db >= 0) {
        std::fprintf(stderr,
            "Error: -x (TX VGA gain) is not applicable on receive-only BB60.\n");
        return 1;
    }
    // Open device
    std::unique_ptr<BbDevice> device;
    try {
        uint32_t serial = cfg.serial_number;
        if (serial == 0 && cfg.device_index > 0) {
            int serials[BB_MAX_DEVICES] = {};
            int types[BB_MAX_DEVICES]   = {};
            int count = 0;
            bbStatus st = bbGetSerialNumberList2(serials, types, &count);
            if (st < bbNoError) {
                std::fprintf(stderr, "Error enumerating devices: %s\n", bbGetErrorString(st));
                return 1;
            }
            if (cfg.device_index >= count) {
                std::fprintf(stderr, "Error: device index %d not found (%d device(s) connected)\n",
                             cfg.device_index, count);
                return 1;
            }
            serial = static_cast<uint32_t>(serials[cfg.device_index]);
        }
        device = std::make_unique<BbDevice>(serial);
    } catch (const BbError& e) {
        std::fprintf(stderr, "Error opening device: %s\n", e.what());
        return 1;
    }

    if (!cfg.quiet) {
        const auto& info = device->info();
        std::printf("Opened %s serial=%u firmware=%d\n",
                    bb_device_type_name(info.device_type),
                    info.serial_number,
                    info.firmware_version);
    }

    // Show diagnostics if requested (before streaming)
    if (cfg.show_diagnostics) {
        float temp, volts, amps;
        try {
            device->get_diagnostics(temp, volts, amps);
            std::printf("Device temp: %.1f°C, USB: %.2fV %.3fA\n", temp, volts, amps);
        } catch (const BbError& e) {
            std::fprintf(stderr, "Warning: could not get diagnostics: %s\n", e.what());
        }
    }

    // Configure
    StreamParams params;
    try {
        params = device->configure(cfg);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Configuration error: %s\n", e.what());
        return 1;
    }

    if (!cfg.quiet) {
        std::printf("Sample rate: %.3f Msps (decimation %d), bandwidth: %.3f MHz\n",
                    params.actual_sample_rate / 1e6,
                    params.decimation,
                    params.actual_bandwidth / 1e6);
        std::printf("Output: %s, format: %s\n",
                    cfg.stdout_mode ? "stdout" : cfg.output_file.c_str(),
                    cfg.format == IqFormat::CF32 ? "cf32" :
                    cfg.format == IqFormat::CS16 ? "cs16" : "sigmf");
    }

    // Start streaming
    bool with_ts     = (cfg.timestamp_mode != "none");
    bool with_trig   = (cfg.trigger_mode   != "none");
    int  port2_flags = 0;
    if (with_trig) {
        const auto& info = device->info();
        if (info.device_type == BB_DEVICE_BB60D) {
            port2_flags = (cfg.trigger_mode == "rising")
                          ? BB60D_PORT2_IN_TRIG_RISING_EDGE
                          : BB60D_PORT2_IN_TRIG_FALLING_EDGE;
        } else {
            port2_flags = (cfg.trigger_mode == "rising")
                          ? BB60C_PORT2_IN_TRIG_RISING_EDGE
                          : BB60C_PORT2_IN_TRIG_FALLING_EDGE;
        }
    }
    try {
        device->start_streaming(with_ts, with_trig, port2_flags);
    } catch (const BbError& e) {
        std::fprintf(stderr, "Error starting streaming: %s\n", e.what());
        return 1;
    }

    // Retrieve iq_correction for CS16 after initiating (must be after bbInitiate)
    if (cfg.format == IqFormat::CS16) {
        float corr = 1.0f;
        static_cast<void>(bb_check(bbGetIQCorrection(device->handle(), &corr), "bbGetIQCorrection"));
        params.iq_correction = corr;
    }

    // Build SigMF metadata struct (used whether or not format is SigMF)
    SigMfMeta meta;
    meta.center_freq_hz = cfg.center_freq_hz;
    meta.sample_rate_hz = params.actual_sample_rate;
    meta.bandwidth_hz   = params.actual_bandwidth;
    meta.decimation     = params.decimation;
    const auto& info    = device->info();
    meta.device_type    = bb_device_type_name(info.device_type);
    meta.serial_number  = static_cast<int>(info.serial_number);
    meta.format         = (cfg.format == IqFormat::CS16) ? "ci16_le" : "cf32_le";
    meta.iq_correction  = params.iq_correction;
    meta.tool_version   = SIGHOUND_TRANSFER_VERSION;
    meta.sdk_version    = "";  // not exposed in API

    // Create writer
    std::unique_ptr<Writer> writer;
    try {
        writer = make_writer(cfg, meta);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error creating output: %s\n", e.what());
        return 1;
    }

    // Install signal handlers and run capture
    install_signal_handlers();

    int result = 0;
    try {
        result = run_capture(*device, params, cfg, *writer, meta);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Capture error: %s\n", e.what());
        result = 1;
    }

    // Write metadata sidecar if requested
    if (!cfg.metadata_path.empty() && result == 0) {
        meta.sample_count = writer->samples_written();
        nlohmann::json j = build_sigmf_meta(meta);
        FILE* mf = std::fopen(cfg.metadata_path.c_str(), "w");
        if (!mf) {
            std::fprintf(stderr, "Warning: could not write metadata to %s\n",
                         cfg.metadata_path.c_str());
        } else {
            std::string s = j.dump(2);
            std::fwrite(s.data(), 1, s.size(), mf);
            std::fclose(mf);
        }
    }

    return result;
}
