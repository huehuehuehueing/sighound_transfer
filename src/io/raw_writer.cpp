// src/io/raw_writer.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include "io/raw_writer.hpp"
#include <stdexcept>
#include <sys/stat.h>

namespace sht {

static bool file_exists(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}

RawWriter::RawWriter(const std::string& path, IqFormat format, bool overwrite)
    : format_(format)
    , bytes_per_sample_(format == IqFormat::CS16 ? sizeof(int16_t) * 2 : sizeof(float) * 2)
{
    if (format == IqFormat::SigMF)
        throw std::runtime_error("RawWriter does not support SigMF format; use SigMfWriter");
    if (!overwrite && file_exists(path)) {
        throw std::runtime_error("Output file exists: " + path +
                                 ". Use --overwrite to replace.");
    }
    file_ = std::fopen(path.c_str(), "wb");
    if (!file_) throw std::runtime_error("Cannot open output file: " + path);
    owns_file_ = true;
}

RawWriter::RawWriter(IqFormat format)
    : format_(format)
    , bytes_per_sample_(format == IqFormat::CS16 ? sizeof(int16_t) * 2 : sizeof(float) * 2)
{
    if (format == IqFormat::SigMF)
        throw std::runtime_error("RawWriter does not support SigMF format; use SigMfWriter");
    file_ = stdout;
    owns_file_ = false;
}

RawWriter::~RawWriter() {
    finalize();
}

void RawWriter::write(const void* data, int64_t count) {
    if (count <= 0) return;
    if (!file_) throw std::runtime_error("Write to closed file");
    size_t n = static_cast<size_t>(count) * bytes_per_sample_;
    size_t written = std::fwrite(data, 1, n, file_);
    if (written != n) throw std::runtime_error("Write error: short write to output file");
    samples_written_ += count;
}

void RawWriter::finalize() {
    if (file_ && owns_file_) {
        std::fflush(file_);
        std::fclose(file_);
        file_ = nullptr;
    }
}

} // namespace sht
