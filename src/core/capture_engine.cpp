// src/core/capture_engine.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include "core/capture_engine.hpp"
#include "io/raw_writer.hpp"
#include "io/sigmf_writer.hpp"
#include "util/signal_handler.hpp"
#include "util/progress.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <algorithm>

namespace sht {

static std::string iso8601_now() {
    std::time_t t = std::time(nullptr);
    char buf[64];
    std::tm tm_utc{};
    gmtime_r(&t, &tm_utc);
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return buf;
}

std::unique_ptr<Writer> make_writer(const Config& cfg, const SigMfMeta& meta) {
    if (cfg.stdout_mode) {
        if (cfg.format == IqFormat::SigMF) {
            std::fprintf(stderr, "Warning: --stdout with --format sigmf produces raw CF32 output; "
                         "SigMF metadata cannot be written to stdout.\n");
        }
        IqFormat stdout_fmt = (cfg.format == IqFormat::SigMF) ? IqFormat::CF32 : cfg.format;
        return std::make_unique<RawWriter>(stdout_fmt);
    }

    switch (cfg.format) {
    case IqFormat::SigMF:
        return std::make_unique<SigMfWriter>(cfg.output_file, meta, cfg.overwrite);
    case IqFormat::CF32:
        return std::make_unique<RawWriter>(cfg.output_file, IqFormat::CF32, cfg.overwrite);
    case IqFormat::CS16:
        return std::make_unique<RawWriter>(cfg.output_file, IqFormat::CS16, cfg.overwrite);
    }
    throw std::logic_error("Unknown format");
}

int run_capture(BbDevice& device, const StreamParams& params,
                const Config& cfg, Writer& writer, SigMfMeta& meta) {
    bool trigger_enabled = (cfg.trigger_mode != "none");
    int trig_buf_size = trigger_enabled ? 256 : 0;
    std::vector<int> triggers(static_cast<size_t>(trig_buf_size), cfg.trigger_sentinel);

    int block_size = cfg.block_size;
    size_t bytes_per_sample = (cfg.format == IqFormat::CS16)
                              ? sizeof(int16_t) * 2
                              : sizeof(float) * 2;
    std::vector<uint8_t> buf(static_cast<size_t>(block_size) * bytes_per_sample);

    ProgressReporter progress(cfg.progress && !cfg.quiet, params.actual_sample_rate);

    int64_t total_requested = cfg.num_samples;
    if (cfg.duration_seconds > 0.0 && total_requested == 0) {
        total_requested = static_cast<int64_t>(
            cfg.duration_seconds * params.actual_sample_rate);
    }

    int64_t captured = 0;
    bool first_block = true;

    auto t_start = std::chrono::steady_clock::now();
    std::exception_ptr loop_exception;
    try {
        while (!stop_requested()) {
            int64_t remaining = (total_requested > 0) ? (total_requested - captured) : block_size;
            if (remaining <= 0) break;
            int this_block = static_cast<int>(std::min(static_cast<int64_t>(block_size), remaining));

            int sec = 0, nano = 0;
            bool loss = device.get_iq_block(buf.data(), this_block,
                                             &sec, &nano,
                                             trigger_enabled ? triggers.data() : nullptr,
                                             trig_buf_size);

            if (first_block && cfg.timestamp_mode != "none") {
                first_block = false;
                std::time_t ts = static_cast<std::time_t>(sec);
                char buf_ts[64];
                std::tm tm_utc{};
                gmtime_r(&ts, &tm_utc);
                std::strftime(buf_ts, sizeof(buf_ts), "%Y-%m-%dT%H:%M:%S", &tm_utc);
                char full[80];
                std::snprintf(full, sizeof(full), "%s.%09dZ", buf_ts, nano);
                meta.capture_timestamp = full;
                if (auto* sw = dynamic_cast<SigMfWriter*>(&writer))
                    sw->set_timestamp(full);
            } else {
                first_block = false;
            }

            writer.write(buf.data(), this_block);
            captured += this_block;

            progress.update(captured, total_requested, loss);
        }
    } catch (...) {
        loop_exception = std::current_exception();
    }

    auto t_end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    device.stop_streaming();
    try { writer.finalize(); } catch (const std::exception& e) {
        if (!loop_exception)  // don't shadow the original exception
            std::fprintf(stderr, "Warning: error finalizing output: %s\n", e.what());
    }

    if (loop_exception) {
        std::rethrow_exception(loop_exception);
    }

    progress.finish(captured, elapsed);

    if (!cfg.quiet) {
        std::printf("Actual sample rate: %.3f Msps, decimation: %d, bandwidth: %.3f MHz\n",
                    params.actual_sample_rate / 1e6,
                    params.decimation,
                    params.actual_bandwidth / 1e6);
    }

    return 0;
}

} // namespace sht
