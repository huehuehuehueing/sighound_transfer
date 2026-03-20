// src/io/sigmf_writer.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
//
// Linking exception: you may combine and distribute this program with the
// proprietary Signal Hound BB API (libbb_api) and FTDI D2XX (libftd2xx)
// libraries under their respective license terms. The GPL requirements of
// this file do not extend to those libraries.
#include "io/sigmf_writer.hpp"
#include <nlohmann/json.hpp>
#include <ctime>
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>

namespace sht {

using json = nlohmann::json;

static bool file_exists(const std::string& p) {
    struct stat st{};
    return stat(p.c_str(), &st) == 0;
}

static std::string strip_sigmf_suffix(const std::string& path) {
    for (const char* suf : {".sigmf-data", ".sigmf-meta", ".sigmf"}) {
        size_t pos = path.rfind(suf);
        if (pos != std::string::npos && pos + std::strlen(suf) == path.size())
            return path.substr(0, pos);
    }
    return path;
}

nlohmann::json build_sigmf_meta(const SigMfMeta& meta) {
    json j;

    // Global object
    j["global"]["core:datatype"]    = meta.format;
    j["global"]["core:sample_rate"] = meta.sample_rate_hz;
    j["global"]["core:version"]     = "1.0.0";
    j["global"]["core:extensions"]  = json::array({
        json::object({{"name", "signalhound"}, {"version", "1.0.0"}, {"optional", true}})
    });

    if (!meta.tool_version.empty())
        j["global"]["core:recorder"]    = "sighound_transfer v" + meta.tool_version;
    if (!meta.sdk_version.empty())
        j["global"]["signalhound:sdk_version"] = meta.sdk_version;
    if (!meta.capture_timestamp.empty())
        j["global"]["signalhound:capture_datetime"] = meta.capture_timestamp;

    j["global"]["signalhound:device_type"]   = meta.device_type;
    j["global"]["signalhound:serial_number"] = meta.serial_number;
    j["global"]["signalhound:decimation"]    = meta.decimation;
    j["global"]["signalhound:iq_correction"] = meta.iq_correction;

    if (meta.bandwidth_hz > 0.0)
        j["global"]["signalhound:bandwidth_hz"] = meta.bandwidth_hz;
    if (meta.sample_count > 0)
        j["global"]["signalhound:sample_count"] = meta.sample_count;

    // Captures array (one entry — single contiguous capture)
    json capture;
    capture["core:sample_start"] = 0;
    capture["core:frequency"]    = meta.center_freq_hz;
    if (!meta.capture_timestamp.empty())
        capture["core:datetime"] = meta.capture_timestamp;

    j["captures"] = json::array({capture});

    // Annotations (empty for now)
    j["annotations"] = json::array();

    return j;
}

SigMfWriter::SigMfWriter(const std::string& base_path, const SigMfMeta& meta, bool overwrite)
    : meta_(meta)
{
    std::string base = strip_sigmf_suffix(base_path);
    data_path_ = base + ".sigmf-data";
    meta_path_ = base + ".sigmf-meta";

    if (!overwrite && (file_exists(data_path_) || file_exists(meta_path_))) {
        throw std::runtime_error("SigMF output files exist: " + base +
                                 ". Use --overwrite.");
    }

    file_ = std::fopen(data_path_.c_str(), "wb");
    if (!file_) throw std::runtime_error("Cannot open SigMF data file: " + data_path_);
}

SigMfWriter::~SigMfWriter() {
    try {
        finalize();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "SigMfWriter: finalize failed in destructor: %s\n", e.what());
    } catch (...) {
        std::fprintf(stderr, "SigMfWriter: unknown error in destructor finalize\n");
    }
}

void SigMfWriter::write(const void* data, int64_t count) {
    if (count <= 0) return;
    if (!file_) throw std::runtime_error("SigMfWriter: write to closed file");
    // SigMfWriter always writes CF32 (complex float32) IQ data.
    static_assert(sizeof(float) == 4, "float must be 32-bit");
    size_t n = static_cast<size_t>(count) * sizeof(float) * 2;
    if (std::fwrite(data, 1, n, file_) != n)
        throw std::runtime_error("SigMF write error");
    samples_written_ += count;
}

void SigMfWriter::set_timestamp(const std::string& iso8601) {
    meta_.capture_timestamp = iso8601;
}

void SigMfWriter::finalize() {
    if (file_) {
        std::fflush(file_);
        std::fclose(file_);
        file_ = nullptr;
        meta_.sample_count = samples_written_;
        write_meta_file();
    }
}

void SigMfWriter::write_meta_file() {
    auto j = build_sigmf_meta(meta_);

    FILE* mf = std::fopen(meta_path_.c_str(), "w");
    if (!mf) throw std::runtime_error("Cannot write SigMF meta: " + meta_path_);
    std::string s = j.dump(2);
    size_t written = std::fwrite(s.data(), 1, s.size(), mf);
    std::fclose(mf);
    if (written != s.size())
        throw std::runtime_error("SigMF meta write error: " + meta_path_);
}

} // namespace sht
