# Integration Summary: Google Cast & OpenWakeWord

## Overview

This document summarizes the integration of Google Cast Audio and OpenWakeWord wake word detection into the Naphome Korvo1 project.

## Components Added

### 1. Google Cast Audio (Placeholder)

**Files Created:**
- `main/google_cast.h` - Google Cast API definitions
- `main/wifi_manager.h` - WiFi management API

**Status:** ⚠️ **Stub/Placeholder Implementation**

The Google Cast integration is currently a placeholder. To complete the implementation:

1. **Add Google Cast SDK:**
   - Integrate Google Cast Audio SDK for embedded devices
   - Or use ESP-ADF (Espressif Audio Development Framework) which includes Cast support

2. **Implement WiFi Manager:**
   - Complete `wifi_manager.c` implementation
   - Add WiFi provisioning (smart config or WPS)
   - Handle connection/disconnection events

3. **Implement Cast Receiver:**
   - Set up mDNS/DNS-SD for device discovery
   - Implement Cast protocol (HTTP server, WebSocket)
   - Integrate with audio player for streaming

**Resources:**
- [Google Cast SDK](https://developers.google.com/cast)
- [ESP-ADF](https://github.com/espressif/esp-adf)

### 2. OpenWakeWord (Placeholder)

**Files Created:**
- `components/openwakeword/` - OpenWakeWord component
  - `include/openwakeword_esp32.h` - ESP32 wrapper API
  - `openwakeword_esp32.cpp` - Implementation (stub)
  - `openwakeword_wrapper.cpp` - Library wrapper
  - `README.md` - Integration guide
- `main/wake_word_manager.h` - Wake word manager API
- `main/wake_word_manager.c` - Wake word manager implementation

**Status:** ⚠️ **Stub/Placeholder Implementation**

The OpenWakeWord integration is currently a placeholder with simulated detection. To complete:

1. **Add OpenWakeWord Library:**
   ```bash
   cd components/openwakeword
   git submodule add https://github.com/dscripka/openWakeWord.git openwakeword
   ```

2. **Update Implementation:**
   - Include OpenWakeWord headers
   - Load wake word model (embed or load from SPIFFS)
   - Implement actual inference in `openwakeword_esp32.cpp`
   - Link against OpenWakeWord library

3. **Add Wake Word Model:**
   - Download pre-trained model or train custom model
   - Embed in firmware or store in SPIFFS partition
   - Update model loading code

**Current Behavior:**
- Simulates wake word detection every 3 seconds (for testing)
- Ready for actual OpenWakeWord integration

**Resources:**
- [OpenWakeWord GitHub](https://github.com/dscripka/openWakeWord)
- [OpenWakeWord Documentation](https://github.com/dscripka/openWakeWord#readme)

## Architecture

```
┌─────────────────┐
│   Microphone    │ (Korvo1 PDM, I2S1)
│   (16kHz mono)  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Wake Word Mgr   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ OpenWakeWord    │ (Wake word detection)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Callback       │ (Wake word detected)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Google Cast     │ (Audio streaming)
│   or            │
│ Google Assistant│ (Voice commands)
└─────────────────┘
```

## Integration Points

### Wake Word Detection Flow

1. **Microphone Input:**
   - Korvo1 PDM microphone on I2S1
   - 16 kHz, 16-bit PCM, mono
   - Continuous capture in background task

2. **Wake Word Processing:**
   - Audio chunks processed through OpenWakeWord
   - Detection callback triggered on wake word

3. **Action on Detection:**
   - Currently logs detection
   - Ready for Google Assistant integration
   - Can trigger LED feedback, audio acknowledgment, etc.

### Google Cast Flow (Future)

1. **WiFi Connection:**
   - Connect to local network
   - Get IP address

2. **Cast Discovery:**
   - Advertise device via mDNS
   - Respond to Cast discovery requests

3. **Audio Streaming:**
   - Receive audio stream from Cast sender
   - Decode and play through ES8311 codec
   - Apply EQ processing

## Configuration

### GPIO Pins (Korvo1)

**Microphone (I2S1):**
- DIN: GPIO 35 (PDM data)
- BCLK: GPIO 36 (PDM bit clock)
- WS: GPIO 37 (PDM word select)
- MCLK: GPIO 0 (if needed)

**Speaker (I2S0):**
- Already configured in `audio_player.c`

**Note:** Verify GPIO pins match your Korvo1 board configuration.

### Sample Rates

- **Wake Word Detection:** 16 kHz (required by OpenWakeWord)
- **Audio Playback:** 48 kHz (current WAV files)
- **Google Cast:** Typically 44.1 kHz or 48 kHz

## Next Steps

### Immediate (To Make Functional)

1. **OpenWakeWord:**
   - Add OpenWakeWord as submodule
   - Implement model loading
   - Test with actual wake word model

2. **WiFi Manager:**
   - Complete WiFi connection implementation
   - Add WiFi provisioning

### Future Enhancements

1. **Google Cast:**
   - Integrate Cast SDK
   - Implement receiver protocol
   - Test with Google Home app

2. **Google Assistant:**
   - Add Assistant SDK
   - Integrate voice commands
   - Handle Assistant responses

3. **LED Feedback:**
   - Visual feedback for wake word detection
   - Status indicators for Cast connection
   - Audio level visualization

4. **Configuration:**
   - Add menuconfig options for WiFi/Cast
   - Store credentials in NVS
   - OTA updates for models

## Testing

### Current State

- ✅ Build compiles successfully
- ✅ Wake word manager initializes
- ⚠️ Simulated wake word detection (every 3 seconds)
- ⚠️ Google Cast not yet implemented

### Testing Checklist

- [ ] Verify microphone GPIO pins
- [ ] Test microphone capture
- [ ] Integrate actual OpenWakeWord model
- [ ] Test wake word detection accuracy
- [ ] Implement WiFi connection
- [ ] Test Google Cast discovery
- [ ] Test audio streaming via Cast
- [ ] Integrate Google Assistant (if needed)

## Files Modified

- `main/app_main.c` - Added wake word manager initialization
- `main/CMakeLists.txt` - Added wake_word_manager.c and openwakeword component

## Files Created

### Google Cast (Placeholder)
- `main/google_cast.h`
- `main/wifi_manager.h`

### OpenWakeWord
- `components/openwakeword/CMakeLists.txt`
- `components/openwakeword/include/openwakeword_esp32.h`
- `components/openwakeword/openwakeword_esp32.cpp`
- `components/openwakeword/openwakeword_wrapper.cpp`
- `components/openwakeword/README.md`

### Wake Word Manager
- `main/wake_word_manager.h`
- `main/wake_word_manager.c`

## Notes

- Both integrations are currently **placeholder/stub implementations**
- The architecture is designed to be easily extended with actual libraries
- All components compile and integrate correctly
- Ready for actual library integration when needed
