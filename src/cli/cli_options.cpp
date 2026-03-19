// src/cli/cli_options.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026  Larry H  <l.gr [at] dartmouth [dot] edu>
// See LICENSE for the full terms.
#include "cli/cli_options.hpp"
#include "version.hpp"
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <cstdio>

namespace sht {

// Long option IDs (>= 256 to avoid clashing with short options)
enum LongOptId {
    OPT_LIST_DEVICES = 256,
    OPT_SERIAL,
    OPT_DEVICE_INDEX,
    OPT_REF_LEVEL,
    OPT_GAIN,
    OPT_ATTEN,
    OPT_FORMAT,
    OPT_DECIMATION,
    OPT_BANDWIDTH,
    OPT_DURATION,
    OPT_STDOUT,
    OPT_OVERWRITE,
    OPT_METADATA,
    OPT_SIGMF,
    OPT_TIMESTAMP_MODE,
    OPT_TRIGGER,
    OPT_TRIGGER_SENTINEL,
    OPT_BLOCK_SIZE,
    OPT_SHOW_DIAGNOSTICS,
    OPT_PROGRESS,
    OPT_QUIET,
    OPT_VERSION,
};

static const struct option k_long_opts[] = {
    {"list-devices",      no_argument,       nullptr, OPT_LIST_DEVICES},
    {"serial",            required_argument,  nullptr, OPT_SERIAL},
    {"device-index",      required_argument,  nullptr, OPT_DEVICE_INDEX},
    {"ref-level",         required_argument,  nullptr, OPT_REF_LEVEL},
    {"gain",              required_argument,  nullptr, OPT_GAIN},
    {"atten",             required_argument,  nullptr, OPT_ATTEN},
    {"format",            required_argument,  nullptr, OPT_FORMAT},
    {"decimation",        required_argument,  nullptr, OPT_DECIMATION},
    {"bandwidth",         required_argument,  nullptr, OPT_BANDWIDTH},
    {"duration",          required_argument,  nullptr, OPT_DURATION},
    {"stdout",            no_argument,        nullptr, OPT_STDOUT},
    {"overwrite",         no_argument,        nullptr, OPT_OVERWRITE},
    {"metadata",          required_argument,  nullptr, OPT_METADATA},
    {"sigmf",             no_argument,        nullptr, OPT_SIGMF},
    {"timestamp-mode",    required_argument,  nullptr, OPT_TIMESTAMP_MODE},
    {"trigger",           required_argument,  nullptr, OPT_TRIGGER},
    {"trigger-sentinel",  required_argument,  nullptr, OPT_TRIGGER_SENTINEL},
    {"block-size",        required_argument,  nullptr, OPT_BLOCK_SIZE},
    {"show-diagnostics",  no_argument,        nullptr, OPT_SHOW_DIAGNOSTICS},
    {"progress",          no_argument,        nullptr, OPT_PROGRESS},
    {"quiet",             no_argument,        nullptr, OPT_QUIET},
    {"version",           no_argument,        nullptr, OPT_VERSION},
    {nullptr, 0, nullptr, 0}
};

void print_help(const char* progname) {
    std::printf(
        "sighound_transfer v" SIGHOUND_TRANSFER_VERSION " -- Signal Hound IQ capture utility\n"
        "Inspired by hackrf_transfer; BB60 is receive-only.\n\n"
        "USAGE:\n"
        "  %s -r <file> -f <freq_hz> [options]\n"
        "  %s --list-devices\n\n"
        "HACKRF-COMPATIBLE OPTIONS:\n"
        "  -r <file>          Receive IQ into file\n"
        "  -f <freq_hz>       Center frequency in Hz\n"
        "  -s <rate_hz>       Requested sample rate in Hz (nearest supported chosen)\n"
        "  -n <num_samples>   Number of complex samples (0 = unlimited)\n"
        "  -b <bw_hz>         IQ bandwidth in Hz (0 = auto)\n"
        "\n"
        "SIGNAL HOUND OPTIONS:\n"
        "  --list-devices           List connected BB60 devices and exit\n"
        "  --serial <n>             Open device with serial number n\n"
        "  --device-index <n>       Open nth detected device (default 0)\n"
        "  --ref-level <dBm>        Reference level, max input amplitude (default -20)\n"
        "  --gain <0..3|auto>       Gain setting (default auto)\n"
        "  --atten <0..3|auto>      Attenuation setting (default auto)\n"
        "  --format <cf32|cs16|sigmf>  Output format (default cf32)\n"
        "  --decimation <n>         Explicit power-of-2 decimation factor\n"
        "  --bandwidth <hz>         IQ filter bandwidth (overrides -b)\n"
        "  --duration <seconds>     Capture duration in seconds (0 = unlimited)\n"
        "  --stdout                 Write IQ data to stdout\n"
        "  --overwrite              Overwrite existing output file\n"
        "  --metadata <path>        Write JSON metadata sidecar to this path\n"
        "  --sigmf                  Alias for --format sigmf\n"
        "  --timestamp-mode <mode>  none|system|gps (default system)\n"
        "  --trigger <mode>         none|rising|falling (default none)\n"
        "  --trigger-sentinel <n>   Trigger sentinel value (default -1)\n"
        "  --block-size <n>         IQ samples per API call (default 262144)\n"
        "  --show-diagnostics       Print device temperature and USB voltage\n"
        "  --progress               Show progress (default when not --quiet)\n"
        "  --quiet                  Suppress all non-error output\n"
        "  --version                Print version and exit\n\n"
        "EXAMPLES:\n"
        "  %s -r capture.bin -f 433920000 -s 1000000\n"
        "  %s -r capture.sigmf -f 2.4e9 --format sigmf -n 1048576\n"
        "  %s --list-devices\n",
        progname, progname, progname, progname, progname
    );
}

Config parse_cli(int argc, char* argv[]) {
    Config cfg;
    int opt;
    int longidx = 0;

    // Reset getopt state (important for unit tests calling this multiple times)
    optind = 1;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "r:t:f:s:n:b:ax:wh",
                              k_long_opts, &longidx)) != -1) {
        switch (opt) {
        case 'r':
            cfg.output_file = optarg;
            cfg.mode = OpMode::Receive;
            break;
        case 't':
            cfg.transmit_file = optarg;
            cfg.mode = OpMode::Transmit;
            break;
        case 'w':
            cfg.format = IqFormat::SigMF;
            break;
        case 'f':
            try {
                cfg.center_freq_hz = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for -f: ") + optarg);
            }
            break;
        case 's':
            try {
                cfg.requested_sample_rate = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for -s: ") + optarg);
            }
            break;
        case 'n':
            try {
                cfg.num_samples = std::stoll(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for -n: ") + optarg);
            }
            break;
        case 'b':
            try {
                cfg.bandwidth_hz = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for -b: ") + optarg);
            }
            break;
        case 'a':
            cfg.amp_enable = true;
            break;
        case 'x':
            try {
                cfg.tx_vga_gain_db = std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for -x: ") + optarg);
            }
            break;
        case 'h':
            print_help(argv[0]);
            std::exit(0);
            break;

        case OPT_LIST_DEVICES:
            cfg.list_devices = true;
            break;
        case OPT_SERIAL:
            try {
                cfg.serial_number = static_cast<uint32_t>(std::stoul(optarg));
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --serial: ") + optarg);
            }
            break;
        case OPT_DEVICE_INDEX:
            try {
                cfg.device_index = std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --device-index: ") + optarg);
            }
            if (cfg.device_index < 0)
                throw CliError("--device-index must be non-negative");
            break;
        case OPT_REF_LEVEL:
            try {
                cfg.ref_level_dbm = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --ref-level: ") + optarg);
            }
            break;
        case OPT_GAIN:
            try {
                cfg.gain = (std::strcmp(optarg, "auto") == 0) ? -1 : std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --gain: ") + optarg);
            }
            break;
        case OPT_ATTEN:
            try {
                cfg.atten = (std::strcmp(optarg, "auto") == 0) ? -1 : std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --atten: ") + optarg);
            }
            break;
        case OPT_FORMAT:
            if (std::strcmp(optarg, "cf32") == 0)      cfg.format = IqFormat::CF32;
            else if (std::strcmp(optarg, "cs16") == 0) cfg.format = IqFormat::CS16;
            else if (std::strcmp(optarg, "sigmf") == 0) cfg.format = IqFormat::SigMF;
            else throw CliError(std::string("Unknown format: ") + optarg);
            break;
        case OPT_DECIMATION:
            try {
                cfg.explicit_decimation = std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --decimation: ") + optarg);
            }
            break;
        case OPT_BANDWIDTH:
            try {
                cfg.bandwidth_hz = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --bandwidth: ") + optarg);
            }
            break;
        case OPT_DURATION:
            try {
                cfg.duration_seconds = std::stod(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --duration: ") + optarg);
            }
            break;
        case OPT_STDOUT:
            cfg.stdout_mode = true;
            break;
        case OPT_OVERWRITE:
            cfg.overwrite = true;
            break;
        case OPT_METADATA:
            cfg.metadata_path = optarg;
            break;
        case OPT_SIGMF:
            cfg.format = IqFormat::SigMF;
            break;
        case OPT_TIMESTAMP_MODE:
            cfg.timestamp_mode = optarg;
            break;
        case OPT_TRIGGER:
            cfg.trigger_mode = optarg;
            break;
        case OPT_TRIGGER_SENTINEL:
            try {
                cfg.trigger_sentinel = std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --trigger-sentinel: ") + optarg);
            }
            break;
        case OPT_BLOCK_SIZE:
            try {
                cfg.block_size = std::stoi(optarg);
            } catch (const std::exception&) {
                throw CliError(std::string("Invalid value for --block-size: ") + optarg);
            }
            break;
        case OPT_SHOW_DIAGNOSTICS:
            cfg.show_diagnostics = true;
            break;
        case OPT_PROGRESS:
            cfg.progress = true;
            break;
        case OPT_QUIET:
            cfg.quiet = true;
            cfg.progress = false;
            break;
        case OPT_VERSION:
            std::printf("sighound_transfer v" SIGHOUND_TRANSFER_VERSION "\n");
            std::exit(0);
            break;
        case '?':
        default:
            throw CliError("Unknown option. Use -h for help.");
        }
    }

    // Validation
    if (cfg.list_devices) return cfg;  // --list-devices needs no other args

    if (cfg.mode == OpMode::Transmit) return cfg;  // validated later

    if (cfg.mode == OpMode::None && !cfg.stdout_mode) {
        throw CliError("No operation specified. Use -r <file> to receive, or --list-devices.");
    }

    if (cfg.mode == OpMode::Receive && cfg.center_freq_hz == 0.0) {
        throw CliError("Center frequency (-f) is required for receive mode.");
    }

    return cfg;
}

} // namespace sht
