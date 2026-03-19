// src/core/capture_engine.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#pragma once
#include "cli/cli_options.hpp"
#include "bb/bb_device.hpp"
#include "io/writer_base.hpp"
#include "io/sigmf_writer.hpp"
#include <memory>

namespace sht {

// Run a full IQ capture based on Config.
// Returns 0 on success, non-zero on error.
// device must already be opened.
// writer must be constructed before calling.
int run_capture(BbDevice& device, const StreamParams& params,
                const Config& cfg, Writer& writer, SigMfMeta& meta);

// Factory: create the right Writer for the config.
// base_path is derived from cfg.output_file.
// meta is pre-populated from device info and params.
std::unique_ptr<Writer> make_writer(const Config& cfg, const SigMfMeta& meta);

} // namespace sht
