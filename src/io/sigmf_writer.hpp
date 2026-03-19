// src/io/sigmf_writer.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include "io/writer_base.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

namespace sht {

struct SigMfMeta {
    double      center_freq_hz    = 0.0;
    double      sample_rate_hz    = 0.0;
    double      bandwidth_hz      = 0.0;
    std::string device_type;        // "BB60A", "BB60C", "BB60D"
    int         serial_number     = 0;
    std::string format            = "cf32_le"; // "cf32_le" or "ci16_le"
    std::string capture_timestamp;  // ISO 8601
    std::string tool_version;
    std::string sdk_version;
    int         decimation        = 1;
    float       iq_correction     = 1.0f;  // CS16 amplitude correction factor from bbGetIQCorrection
    int64_t     sample_count      = 0;     // populated after capture
};

// Build a SigMF metadata JSON object.
nlohmann::json build_sigmf_meta(const SigMfMeta& meta);

// Writes SigMF output: <path>.sigmf-data (raw IQ) + <path>.sigmf-meta (JSON).
// If path ends in ".sigmf-data" or ".sigmf-meta", strips that suffix.
// Always writes CF32 little-endian to the data file.
class SigMfWriter : public Writer {
public:
    SigMfWriter(const std::string& base_path, const SigMfMeta& meta, bool overwrite);
    ~SigMfWriter() override;

    void write(const void* data, int64_t count) override;
    void finalize() override;
    int64_t samples_written() const override { return samples_written_; }

    // Update metadata capture timestamp (call before finalize).
    void set_timestamp(const std::string& iso8601);

    std::string data_path() const { return data_path_; }
    std::string meta_path() const { return meta_path_; }

private:
    std::string   data_path_;
    std::string   meta_path_;
    FILE*         file_       = nullptr;
    SigMfMeta     meta_;
    int64_t       samples_written_ = 0;

    void write_meta_file();
};

} // namespace sht
