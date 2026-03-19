// src/bb/bb_device.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include "bb/bb_device.hpp"
#include "core/rate_planner.hpp"
#include "core/bandwidth_table.hpp"
#include "bb/bb_error.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <limits>

namespace sht {

// Local void-returning wrapper: bb_check() is [[nodiscard]] to guard against
// accidentally discarding its return value, but throughout this file we use it
// solely for its throw-on-error side effect.  bb_chk() lets us call it
// without triggering -Wunused-result at every call site.
static void bb_chk(bbStatus s, const char* ctx) { static_cast<void>(bb_check(s, ctx)); }

BbDevice::BbDevice(uint32_t serial_number) {
    if (serial_number == 0) {
        bb_chk(bbOpenDevice(&info_.handle), "bbOpenDevice");
    } else {
        if (serial_number > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
            throw std::out_of_range("Serial number exceeds SDK int range");
        }
        bb_chk(bbOpenDeviceBySerialNumber(&info_.handle, static_cast<int>(serial_number)),
               "bbOpenDeviceBySerialNumber");
    }
    try {
        bb_chk(bbGetDeviceType(info_.handle, &info_.device_type), "bbGetDeviceType");
        bb_chk(bbGetSerialNumber(info_.handle, &info_.serial_number), "bbGetSerialNumber");
        bb_chk(bbGetFirmwareVersion(info_.handle, &info_.firmware_version), "bbGetFirmwareVersion");
    } catch (...) {
        bbCloseDevice(info_.handle);
        throw;
    }
}

BbDevice::~BbDevice() {
    if (info_.handle != 0) {
        if (streaming_) {
            bbAbort(info_.handle);
        }
        bbCloseDevice(info_.handle);
    }
}

StreamParams BbDevice::configure(const Config& cfg) {
    if (streaming_) {
        stop_streaming();  // bbAbort required before re-configuring
    }
    StreamParams params{};

    // 1. Resolve decimation
    int decimation;
    if (cfg.explicit_decimation > 0) {
        bool valid = false;
        for (int d = 1; d <= 8192; d <<= 1) {
            if (cfg.explicit_decimation == d) { valid = true; break; }
        }
        if (!valid) throw std::invalid_argument("--decimation must be a power of 2 in [1, 8192]");
        decimation = cfg.explicit_decimation;
        params.actual_sample_rate = 40e6 / decimation;
    } else {
        auto plan = plan_rate(cfg.requested_sample_rate);
        decimation = plan.decimation;
        params.actual_sample_rate = plan.actual_sample_rate;
        if (plan.clamped && !cfg.quiet) {
            std::fprintf(stderr, "Warning: requested sample rate %.0f Hz not achievable; "
                         "using %.0f Hz (decimation %d)\n",
                         cfg.requested_sample_rate, plan.actual_sample_rate, decimation);
        }
    }
    params.decimation = decimation;

    // 2. Resolve bandwidth
    double bw_hz = cfg.bandwidth_hz;
    if (bw_hz <= 0.0) {
        bw_hz = default_bandwidth(decimation);
    }
    double max_bw = max_bandwidth_for_decimation(decimation);
    if (bw_hz > max_bw) {
        if (!cfg.quiet)
            std::fprintf(stderr, "Warning: requested bandwidth %.0f Hz exceeds max %.0f Hz "
                         "for decimation %d; clamping.\n", bw_hz, max_bw, decimation);
        bw_hz = max_bw;
    }
    params.actual_bandwidth = bw_hz;

    // 3. Data type
    bbDataType data_type = (cfg.format == IqFormat::CS16) ? bbDataType16sc : bbDataType32fc;
    bb_chk(bbConfigureIQDataType(info_.handle, data_type), "bbConfigureIQDataType");

    // 4. Center frequency
    bb_chk(bbConfigureIQCenter(info_.handle, cfg.center_freq_hz), "bbConfigureIQCenter");

    // 5. Reference level
    bb_chk(bbConfigureRefLevel(info_.handle, cfg.ref_level_dbm), "bbConfigureRefLevel");

    // 6. Gain / attenuation
    int gain  = (cfg.gain  == -1) ? BB_AUTO_GAIN  : cfg.gain;
    int atten = (cfg.atten == -1) ? BB_AUTO_ATTEN : cfg.atten;
    bb_chk(bbConfigureGainAtten(info_.handle, gain, atten), "bbConfigureGainAtten");

    // 7. IQ decimation + bandwidth
    bb_chk(bbConfigureIQ(info_.handle, decimation, bw_hz), "bbConfigureIQ");

    // 8. Trigger sentinel (global call)
    if (cfg.trigger_mode != "none") {
        bbConfigureIQTriggerSentinel(cfg.trigger_sentinel);
    }

    // 9. iq_correction is only valid after bbInitiate; set placeholder for now.
    // Caller must call bbGetIQCorrection(handle(), &params.iq_correction) after start_streaming().
    params.iq_correction = 1.0f;

    return params;
}

void BbDevice::start_streaming(bool with_timestamp, bool with_trigger,
                               int port2_flags) {
    if (with_trigger && port2_flags != 0) {
        bb_chk(bbConfigureIO(info_.handle, 0, port2_flags), "bbConfigureIO");
    }

    uint32_t flag = BB_STREAM_IQ;
    if (with_timestamp) flag |= BB_TIME_STAMP;

    bb_chk(bbInitiate(info_.handle, BB_STREAMING, flag), "bbInitiate");
    streaming_ = true;
}

void BbDevice::stop_streaming() {
    if (streaming_) {
        bbAbort(info_.handle);
        streaming_ = false;
    }
}

bool BbDevice::get_iq_block(void* buf, int block_size,
                             int* out_sec, int* out_nano,
                             int* triggers, int trigger_count) {
    int data_remaining = 0, sample_loss = 0, sec = 0, nano = 0;
    int trig_cnt = triggers ? trigger_count : 0;

    bb_chk(bbGetIQUnpacked(info_.handle, buf, block_size,
                            triggers, trig_cnt, BB_FALSE,
                            &data_remaining, &sample_loss,
                            &sec, &nano),
           "bbGetIQUnpacked");

    if (out_sec)  *out_sec  = sec;
    if (out_nano) *out_nano = nano;

    return sample_loss != BB_FALSE;
}

void BbDevice::get_diagnostics(float& temp, float& usb_voltage, float& usb_current) {
    bb_chk(bbGetDeviceDiagnostics(info_.handle, &temp, &usb_voltage, &usb_current),
           "bbGetDeviceDiagnostics");
}

int list_devices() {
    int serials[BB_MAX_DEVICES] = {};
    int types[BB_MAX_DEVICES]   = {};
    int count                   = 0;

    bbStatus st = bbGetSerialNumberList2(serials, types, &count);
    if (st < bbNoError) {
        std::fprintf(stderr, "Error enumerating devices: %s\n", bbGetErrorString(st));
        return -1;
    }

    if (count == 0) {
        std::puts("No Signal Hound BB60 devices found.");
        return 0;
    }

    std::printf("Found %d device(s):\n", count);
    for (int i = 0; i < count; i++) {
        std::printf("  [%d] Serial: %d  Type: %s\n",
                    i, serials[i], bb_device_type_name(types[i]));
    }
    return count;
}

} // namespace sht
