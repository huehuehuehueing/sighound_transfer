// src/io/raw_writer.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include "io/writer_base.hpp"
#include "cli/cli_options.hpp"
#include <cstdio>
#include <string>

namespace sht {

// Writes raw binary IQ data.
// CF32: interleaved float32 pairs (same as hackrf_transfer raw output).
// CS16: interleaved int16_t pairs (raw SDK output, NOT amplitude corrected).
//       If you want amplitude-corrected output, use a higher-level caller
//       that applies bbGetIQCorrection before calling write().
class RawWriter : public Writer {
public:
    // Opens file for writing. Throws std::runtime_error on failure.
    // If overwrite is false and the file exists, throws.
    RawWriter(const std::string& path, IqFormat format, bool overwrite);

    // Writes to stdout (for --stdout mode).
    explicit RawWriter(IqFormat format);

    ~RawWriter() override;

    void write(const void* data, int64_t count) override;
    void finalize() override;
    int64_t samples_written() const override { return samples_written_; }

private:
    FILE*     file_         = nullptr;
    bool      owns_file_    = false;
    IqFormat  format_;
    int64_t   samples_written_ = 0;
    size_t    bytes_per_sample_; // sizeof(float)*2 or sizeof(int16_t)*2
};

} // namespace sht
