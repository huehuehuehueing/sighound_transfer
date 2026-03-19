# sighound_transfer

A `hackrf_transfer`-style command-line IQ capture utility for Signal Hound BB60 devices.

## What this is

`sighound_transfer` captures IQ data from Signal Hound BB60 spectrum analyzers
to file. It is inspired by `hackrf_transfer` and preserves familiar option names
where technically honest.

**Important:** BB60 devices are receive-only spectrum analyzers. This tool does
not support transmit. The `-t` flag is parsed and returns a clear error.

## Compared to hackrf_transfer

| Feature | hackrf_transfer | sighound_transfer |
|---|---|---|
| Receive IQ to file | ✓ | ✓ |
| Transmit from file | ✓ | ✗ (BB60 is RX-only) |
| Sample rates | 8/10/12.5/16/20 MHz | 40/20/10/5/2.5/... MHz (power-of-2 decimation) |
| SigMF output | ✗ | ✓ |
| Device listing | ✗ | ✓ |
| Trigger support | ✗ | ✓ (external trigger via port 2) |

## Hardware Support

- **Primary:** BB60C, BB60D
- **Secondary:** BB60A (no serial number enumeration)
- USB ID: `2817:0005 Signal Hound, Inc. FX3`

## Feature List

- Continuous IQ capture to CF32 or CS16 raw binary
- SigMF output (`.sigmf-data` + `.sigmf-meta`)
- Automatic sample-rate-to-decimation mapping with user feedback
- Bandwidth validation against decimation limits
- External trigger support (rising/falling edge, configurable sentinel)
- System clock timestamps per block
- Device listing and serial number targeting
- Progress display with sample-loss warnings
- Diagnostics (temperature, USB voltage)
- SIGINT/SIGTERM clean shutdown

## Linux Build

### Prerequisites

```bash
sudo apt-get install cmake libusb-1.0-0-dev unzip curl
```

### Get the SDK

```bash
export SIGNALHOUND_SDK_URL="https://signalhound.com/sigdownloads/SDK/signal_hound_sdk_02_19_26.zip"
bash scripts/bootstrap_sdk.sh
eval "$(bash scripts/bootstrap_sdk.sh 2>/dev/null | grep export)"
```

Or point to an existing local SDK:

```bash
export SIGNALHOUND_SDK_ROOT=~/software/signalhound/sdk/signal_hound_sdk
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIGNALHOUND_SDK_ROOT="$SIGNALHOUND_SDK_ROOT"
cmake --build build --parallel
```

### Install

```bash
sudo cmake --install build
```

### udev Rules (required for non-root access)

```bash
sudo cp "$SIGNALHOUND_SDK_ROOT/device_apis/bb_series/lib/linux_x64/sh_usb.rules" \
        /etc/udev/rules.d/
sudo udevadm control --reload-rules
# Unplug and replug the BB60
```

### Runtime Library Path

The BB API library must be findable at runtime:

```bash
# Option 1: install to system path
sudo cp "$SIGNALHOUND_SDK_ROOT/device_apis/bb_series/lib/linux_x64/Ubuntu 18.04/libbb_api.so.5.0.9" \
        /usr/local/lib/libbb_api.so
sudo ldconfig

# Option 2: set LD_LIBRARY_PATH per-session
export LD_LIBRARY_PATH="$SIGNALHOUND_SDK_ROOT/device_apis/bb_series/lib/linux_x64/Ubuntu 18.04:$LD_LIBRARY_PATH"
```

FTDI D2XX is also required:

```bash
sudo cp "$SIGNALHOUND_SDK_ROOT/device_apis/bb_series/lib/linux_x64/Ubuntu 18.04/libftd2xx.so" \
        /usr/local/lib/
sudo ldconfig
```

## CI Environment Variables

| Variable | Default | Purpose |
|---|---|---|
| `SIGNALHOUND_SDK_URL` | Official 2026 release URL | SDK download location |
| `SIGNALHOUND_SDK_ARCHIVE` | `.sdk_cache/sdk.zip` | Local cache path |
| `SIGNALHOUND_SDK_SHA256` | (none) | Optional integrity check |
| `SIGNALHOUND_SDK_DIR` | `.sdk_cache/signal_hound_sdk` | Extraction directory |
| `SIGNALHOUND_SDK_ROOT` | (auto-discovered) | CMake SDK root |

## Usage Examples

### List connected devices

```bash
sighound_transfer --list-devices
```

### Basic capture (1 second at 20 Msps)

```bash
sighound_transfer -r capture.bin -f 433920000 -s 20000000 --duration 1
```

### Fixed sample count

```bash
sighound_transfer -r capture.bin -f 1000000000 -n 2097152
```

### Specific sample rate (nearest supported chosen)

```bash
sighound_transfer -r capture.bin -f 2400000000 -s 10000000
# Prints: Sample rate: 10.000 Msps (decimation 4), bandwidth: 7.824 MHz
```

### SigMF output

```bash
sighound_transfer -r capture --format sigmf -f 433920000 -s 1000000 -n 1048576
# Writes capture.sigmf-data and capture.sigmf-meta
```

### Open specific device by serial

```bash
sighound_transfer --serial 12345678 -r capture.bin -f 915000000
```

### Trigger capture (rising edge on port 2)

```bash
sighound_transfer -r capture.bin -f 433920000 --trigger rising --trigger-sentinel -1
```

### Show device diagnostics

```bash
sighound_transfer --show-diagnostics -r capture.bin -f 1e9 --duration 5
```

### Pipe to stdout

```bash
sighound_transfer --stdout -f 433920000 -s 1000000 | sox -t raw -r 1000000 -e float -b 32 -c 2 - output.wav
```

## Local Smoke Test (with hardware)

```bash
# Verify: device opens, actual rate reported, file written, count correct, clean shutdown
sighound_transfer -r /tmp/smoke.bin -f 433920000 -s 1000000 -n 100000 --show-diagnostics
ls -la /tmp/smoke.bin
# Expected size: 100000 * 2 * 4 = 800000 bytes (CF32)
wc -c /tmp/smoke.bin
```

## File Formats

### CF32 (default)
Raw binary, interleaved complex float32 I/Q pairs. Little-endian.
Same layout as HackRF raw output and GNU Radio file source.

### CS16
Raw binary, interleaved complex int16_t I/Q pairs. Little-endian.
**Full scale (not amplitude corrected).** The amplitude correction factor
from `bbGetIQCorrection()` is not applied to the raw output; it is reported
in the SigMF metadata when used with `--format sigmf`.

### SigMF
Two files: `<name>.sigmf-data` (CF32 IQ) and `<name>.sigmf-meta` (JSON).
The metadata includes center frequency, sample rate, bandwidth, device type,
serial number, capture timestamp, and Signal Hound extension fields.

## Trigger and Timestamp Notes

- **Triggers** require external signal on port 2 of BB60C/D.
- Trigger indices are relative to the acquired block first sample.
- Default sentinel `-1` avoids ambiguity with a trigger at sample index 0.
- **Timestamps** are per-block first-sample timestamps from the system clock.
- GPS timestamping (`--timestamp-mode gps`) requires GPS receiver connected
  and is documented as Windows-oriented in the SDK examples.
- Use only the first block timestamp plus sample index math for accurate timing.

## Acknowledgments

Signal Hound hardware used during development and testing was graciously
provided by Cesar Arguello and Prof. Timothy J. Pierson (Dartmouth College).

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE).

## Limitations and Future Work

- Transmit is not supported (BB60 hardware is receive-only).
- GPS timestamp mode not validated on Linux.
- WAV output not implemented (use SigMF instead).
- Multi-device simultaneous capture not supported.
- Linux-only: code uses `getopt.h`, `sigaction`, and `gmtime_r`; Windows port not implemented.
- Future: SA series backend for sweep-to-file capture.
