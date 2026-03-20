// src/bb/bb_device.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#pragma once
#include "bb_api.h"
#include "bb_error.hpp"
#include "cli/cli_options.hpp"
#include "core/rate_planner.hpp"
#include <string>
#include <cstdint>

namespace sht {

struct DeviceInfo {
    int      handle;
    int      device_type;   // BB_DEVICE_BB60A/C/D
    uint32_t serial_number;
    int      firmware_version;
};

struct StreamParams {
    double actual_sample_rate;
    double actual_bandwidth;
    int    decimation;
    // For CS16 amplitude correction. Always 1.0f after configure();
    // caller must call bbGetIQCorrection(handle(), &params.iq_correction)
    // after start_streaming() to get the real correction factor.
    float  iq_correction;
};

// RAII wrapper around a BB60 device handle.
// Destructor always calls bbAbort + bbCloseDevice.
class BbDevice {
public:
    // Open first available device or device with specific serial number.
    explicit BbDevice(uint32_t serial_number = 0);
    ~BbDevice();

    // No copy
    BbDevice(const BbDevice&) = delete;
    BbDevice& operator=(const BbDevice&) = delete;

    // Configure for IQ streaming based on Config.
    // Resolves decimation, bandwidth, data type, gain, ref level.
    // Returns StreamParams describing actual chosen parameters.
    StreamParams configure(const Config& cfg);

    // Initiate IQ streaming. Call after configure().
    // bbConfigureIO is only called when with_trigger is true and port2_flags is non-zero.
    // Non-trigger IO setup (e.g. 10 MHz reference) must be done via bbConfigureIO on handle().
    void start_streaming(bool with_timestamp = false, bool with_trigger = false,
                         int port2_flags = 0);

    // Stop streaming (bbAbort).
    void stop_streaming();

    // Get one block of IQ data. buf must be pre-allocated to
    // block_size * (sizeof(float)*2 for cf32 or sizeof(int16_t)*2 for cs16).
    // Returns sampleLoss flag.
    bool get_iq_block(void* buf, int block_size,
                      int* out_sec = nullptr, int* out_nano = nullptr,
                      int* triggers = nullptr, int trigger_count = 0);

    // Get device diagnostics (temperature, USB voltage, current).
    void get_diagnostics(float& temp, float& usb_voltage, float& usb_current);

    const DeviceInfo& info() const { return info_; }

    int handle() const { return info_.handle; }

private:
    DeviceInfo  info_{};
    bool        streaming_ = false;
};

// List all connected BB60 devices to stdout.
// Returns number of devices found.
int list_devices();

} // namespace sht
