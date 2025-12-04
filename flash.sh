#!/bin/bash
# Flash script for M5 Atom Echo Base

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PORT="${1:-/dev/cu.usbmodem101}"

echo "=========================================="
echo "M5 Atom Echo Base Firmware Flasher"
echo "=========================================="
echo ""
echo "Port: $PORT"
echo "Build Dir: $BUILD_DIR"
echo ""

# Verify files exist
if [ ! -f "$BUILD_DIR/bootloader/bootloader.bin" ]; then
    echo "❌ ERROR: Bootloader not found at $BUILD_DIR/bootloader/bootloader.bin"
    exit 1
fi

if [ ! -f "$BUILD_DIR/naphome-korvo1.bin" ]; then
    echo "❌ ERROR: Firmware not found at $BUILD_DIR/naphome-korvo1.bin"
    exit 1
fi

if [ ! -f "$BUILD_DIR/partition_table/partition-table.bin" ]; then
    echo "❌ ERROR: Partition table not found at $BUILD_DIR/partition_table/partition-table.bin"
    exit 1
fi

echo "✅ All files found"
echo ""
echo "Flashing firmware..."
echo "  - Bootloader: $(ls -lh $BUILD_DIR/bootloader/bootloader.bin | awk '{print $5}')"
echo "  - Firmware: $(ls -lh $BUILD_DIR/naphome-korvo1.bin | awk '{print $5}')"
echo "  - Partition: $(ls -lh $BUILD_DIR/partition_table/partition-table.bin | awk '{print $5}')"
echo ""

# Use idf.py if available, otherwise fall back to esptool
if command -v idf.py &> /dev/null; then
    echo "Using idf.py..."
    cd "$PROJECT_ROOT"
    idf.py -p "$PORT" flash
else
    echo "Using esptool.py directly..."
    python -m esptool \
        --chip esp32s3 \
        -p "$PORT" \
        -b 460800 \
        --before=default_reset \
        --after=hard_reset \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        --flash_size 16MB \
        0x0 "$BUILD_DIR/bootloader/bootloader.bin" \
        0x10000 "$BUILD_DIR/naphome-korvo1.bin" \
        0x8000 "$BUILD_DIR/partition_table/partition-table.bin"
fi

echo ""
echo "✅ Flashing complete!"
echo ""
echo "To monitor serial output, run:"
echo "  idf.py -p $PORT monitor"
echo ""
echo "Expected startup logs:"
echo "  I (XXX) m5_echo_base: ES8311 Chip ID: 0x18"
echo "  I (XXX) m5_echo_base: ES8311 codec initialized successfully"
echo ""
