#!/bin/bash

# REW Measurement Automation Script
# This script helps automate audio measurements with REW or alternative tools

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configuration
REW_APP="/Applications/REW.app"
SWEEP_DURATION=5  # seconds (should match your embedded WAV file)
OUTPUT_DIR="$PROJECT_ROOT/measurements"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "=== REW Measurement Helper ==="
echo ""
echo "This script helps automate audio measurements."
echo ""

# Check if REW is installed
if [ ! -d "$REW_APP" ]; then
    echo "⚠️  REW not found at $REW_APP"
    echo ""
    echo "Options:"
    echo "1. Use SoX for command-line recording (recommended for automation)"
    echo "2. Use Python with sounddevice/pyaudio"
    echo "3. Manual REW measurement"
    echo ""
    read -p "Choose option (1-3): " choice
    
    case $choice in
        1)
            echo ""
            echo "Using SoX for recording..."
            if ! command -v sox &> /dev/null; then
                echo "Installing SoX..."
                if command -v brew &> /dev/null; then
                    brew install sox
                else
                    echo "Please install SoX: brew install sox"
                    exit 1
                fi
            fi
            
            echo ""
            echo "Recording audio from default input..."
            echo "Make sure your audio interface is set as the default input."
            echo "Press Enter when ready to start recording, then trigger playback on the device..."
            read
            
            OUTPUT_FILE="$OUTPUT_DIR/measurement_${TIMESTAMP}.wav"
            sox -d -r 48000 -c 2 -b 16 "$OUTPUT_FILE" trim 0 $SWEEP_DURATION
            
            echo ""
            echo "✅ Recording saved to: $OUTPUT_FILE"
            echo ""
            echo "To analyze in REW:"
            echo "1. Open REW"
            echo "2. File → Import → Import measurement"
            echo "3. Select: $OUTPUT_FILE"
            ;;
        2)
            echo ""
            echo "Using Python for recording..."
            python3 "$SCRIPT_DIR/record_measurement.py" --duration $SWEEP_DURATION --output "$OUTPUT_DIR/measurement_${TIMESTAMP}.wav"
            ;;
        3)
            echo ""
            echo "Manual REW measurement:"
            echo "1. Open REW"
            echo "2. Set measurement input to your audio interface"
            echo "3. Click 'Measure' → 'Sweep'"
            echo "4. Configure sweep parameters to match your device"
            echo "5. Trigger playback on your ESP32 device"
            echo "6. Click 'Start Measurement' in REW"
            ;;
    esac
else
    echo "✅ REW found at $REW_APP"
    echo ""
    echo "REW doesn't have a native command-line interface, but you can:"
    echo ""
    echo "Option 1: Use AppleScript to automate REW (macOS only)"
    echo "Option 2: Record with SoX, then import to REW"
    echo "Option 3: Use Python script for automated recording"
    echo ""
    read -p "Choose option (1-3): " choice
    
    case $choice in
        1)
            echo ""
            echo "Opening REW with AppleScript automation..."
            osascript <<EOF
tell application "REW"
    activate
    delay 2
    -- Note: REW automation is limited - you'll need to manually configure
    -- the measurement settings and start the measurement
end tell
EOF
            echo ""
            echo "REW opened. Please:"
            echo "1. Configure measurement input/output"
            echo "2. Set sweep parameters"
            echo "3. Trigger playback on your ESP32 device"
            echo "4. Click 'Start Measurement' in REW"
            ;;
        2)
            echo ""
            echo "Recording with SoX, then importing to REW..."
            if ! command -v sox &> /dev/null; then
                echo "Installing SoX..."
                if command -v brew &> /dev/null; then
                    brew install sox
                else
                    echo "Please install SoX: brew install sox"
                    exit 1
                fi
            fi
            
            OUTPUT_FILE="$OUTPUT_DIR/measurement_${TIMESTAMP}.wav"
            echo ""
            echo "Recording audio from default input..."
            echo "Press Enter when ready, then trigger playback on the device..."
            read
            
            sox -d -r 48000 -c 2 -b 16 "$OUTPUT_FILE" trim 0 $SWEEP_DURATION
            
            echo ""
            echo "✅ Recording saved to: $OUTPUT_FILE"
            echo ""
            echo "Opening REW to import measurement..."
            open -a REW "$OUTPUT_FILE"
            ;;
        3)
            echo ""
            echo "Using Python script for automated recording..."
            python3 "$SCRIPT_DIR/record_measurement.py" --duration $SWEEP_DURATION --output "$OUTPUT_DIR/measurement_${TIMESTAMP}.wav"
            echo ""
            echo "Opening REW to import measurement..."
            open -a REW "$OUTPUT_DIR/measurement_${TIMESTAMP}.wav"
            ;;
    esac
fi

echo ""
echo "=== Done ==="
