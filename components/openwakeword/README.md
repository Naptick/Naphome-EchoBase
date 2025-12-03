# OpenWakeWord Integration for ESP32

This component provides OpenWakeWord integration for ESP32-S3, enabling wake word detection on the Korvo1 development board.

## Overview

OpenWakeWord is an open-source wake word detection library that can run on embedded devices. This integration allows the Korvo1 to detect wake words like "Hey Jarvis" or custom wake words.

## Current Status

**⚠️ This is a placeholder/stub implementation.**

To use actual OpenWakeWord functionality, you need to:

1. **Add OpenWakeWord as a component:**
   ```bash
   cd components/openwakeword
   git submodule add https://github.com/dscripka/openWakeWord.git openwakeword
   ```

2. **Update the implementation:**
   - Include OpenWakeWord headers in `openwakeword_wrapper.cpp`
   - Implement actual model loading in `openwakeword_esp32.cpp`
   - Link against OpenWakeWord library in `CMakeLists.txt`

3. **Add a wake word model:**
   - Download or train an OpenWakeWord model
   - Embed it in the firmware or load from SPIFFS
   - Update model loading code

## Architecture

```
Microphone (Korvo1 PDM)
    ↓
I2S1 (16kHz mono)
    ↓
wake_word_manager.c
    ↓
openwakeword_esp32.cpp
    ↓
OpenWakeWord Model
    ↓
Wake Word Detected Callback
    ↓
Google Assistant / Action Handler
```

## Configuration

- **Sample Rate**: 16 kHz (required by OpenWakeWord)
- **Format**: 16-bit PCM, mono
- **I2S Port**: I2S1 (I2S0 is used for speaker output)
- **Buffer Size**: 512 samples (32ms chunks)

## GPIO Configuration

The microphone uses PDM mode on I2S1:
- **DIN**: GPIO 35 (PDM data input)
- **BCLK**: GPIO 36 (PDM bit clock)
- **WS**: GPIO 37 (PDM word select)
- **MCLK**: GPIO 0 (Master clock, if needed)

**Note**: Verify these GPIO pins match your Korvo1 board configuration.

## Usage

```c
// Initialize wake word manager
wake_word_manager_init();

// Start detection
wake_word_manager_start();

// Wake word callback is called automatically when detected
// See wake_word_manager.c for callback implementation
```

## Next Steps

1. Add OpenWakeWord repository as submodule
2. Implement model loading
3. Test with actual wake word model
4. Integrate with Google Assistant (when Google Cast is implemented)
5. Add LED feedback for wake word detection

## Resources

- [OpenWakeWord GitHub](https://github.com/dscripka/openWakeWord)
- [OpenWakeWord Documentation](https://github.com/dscripka/openWakeWord#readme)
- [ESP32 Audio Processing](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)
