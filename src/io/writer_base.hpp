// src/io/writer_base.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include <cstdint>
#include <cstddef>

namespace sht {

// Abstract interface for IQ data sinks.
class Writer {
public:
    virtual ~Writer() = default;

    // Write `count` complex samples from `data`.
    // For CF32: data is float* (interleaved I,Q pairs, count*2 floats).
    // For CS16: data is int16_t* (interleaved I,Q pairs, count*2 shorts).
    virtual void write(const void* data, int64_t count) = 0;

    // Finalize (flush, write metadata). Called on clean shutdown.
    virtual void finalize() = 0;

    // Total complex samples written so far.
    virtual int64_t samples_written() const = 0;
};

} // namespace sht
