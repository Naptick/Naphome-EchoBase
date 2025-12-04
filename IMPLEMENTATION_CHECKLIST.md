# M5 Atom S3R + Echo Base Porting Implementation Checklist

## Overview

This document provides a step-by-step checklist for porting the Naphome-EchoBase firmware from Korvo1 to M5 Atom S3R + Atom Echo Base.

**Status**: Infrastructure complete, ready for implementation testing

## Phase 1: Build Configuration & Testing ✅ COMPLETE

### Build System Setup
- [x] Create board selection in `Kconfig.projbuild` (BOARD_KORVO1, BOARD_M5_ECHO_BASE)
- [x] Create `main/board_config.h` for board-specific includes
- [x] Create `boards/korvo1.h` with Korvo1 pin definitions
- [x] Create `boards/m5_echo_base.h` with M5 Echo Base pin definitions
- [x] Update root `CMakeLists.txt` to conditionally include board drivers
- [x] Create `sdkconfig.defaults.korvo1` for Korvo1 defaults
- [x] Create `sdkconfig.defaults.m5` for M5 defaults
- [x] Create `partitions_m5.csv` for 4MB flash partition table

### M5 Echo Base Driver Implementation
- [x] Create `drivers/audio/m5_echo_base/` directory structure
- [x] Create `include/m5_echo_base.h` with API definitions
- [x] Create `src/m5_echo_base.c` with ES8311 codec initialization
- [x] Implement I2C communication (register read/write)
- [x] Implement I2S speaker output configuration
- [x] Implement I2S microphone input configuration (full-duplex)
- [x] Create `CMakeLists.txt` for M5 driver component
- [x] Implement volume control functions (get/set)
- [x] Add LED color control placeholder

## Phase 2: Hardware Integration & Testing

### Pre-Build Testing
- [ ] Compile firmware for Korvo1 board (verify no regressions)
  ```bash
  idf.py set-target esp32s3
  cp sdkconfig.defaults.korvo1 sdkconfig.defaults
  idf.py build
  ```

- [ ] Compile firmware for M5 board
  ```bash
  idf.py menuconfig  # Select CONFIG_BOARD_M5_ECHO_BASE
  idf.py build
  ```

### Hardware Validation - M5 Atom Echo Base
- [ ] Verify ES8311 codec initialization
  - [ ] Check I2C communication (GPIO 19=SDA, GPIO 33=SCL)
  - [ ] Verify chip ID register reads correctly
  - [ ] Confirm codec responds to reset command
  - [ ] Test volume control commands

- [ ] Test Speaker Output (I2S0)
  - [ ] Verify I2S0 initialization on GPIO 0, 22, 23, 33
  - [ ] Generate test tone and listen to speaker
  - [ ] Measure audio level with external equipment
  - [ ] Verify no audio distortion

- [ ] Test Microphone Input (I2S1)
  - [ ] Verify I2S1 initialization (full-duplex mode)
  - [ ] Record audio and analyze PCM samples
  - [ ] Test noise floor and sensitivity
  - [ ] Verify 16 kHz mono recording

- [ ] Test Full-Duplex I2S
  - [ ] Simultaneous playback + recording
  - [ ] Verify no crosstalk between channels
  - [ ] Test with various sample rates

### LED Control
- [ ] Test SK6812 LED on GPIO 46 (Echo Base single LED)
  - [ ] Implement RMT driver for LED control
  - [ ] Test color transitions
  - [ ] Integrate with audio playback indicator

- [ ] Optional: Add external WS2812 ring on Atom S3R
  - [ ] Connect 12x WS2812 ring to available GPIO
  - [ ] Update LED configuration for dual-LED setup
  - [ ] Synchronize animations between Echo Base and S3R

## Phase 3: Application Layer Integration

### Audio Player Updates
- [ ] Review `main/audio_player.c` for Korvo1-specific code
- [ ] Create board-abstraction layer for audio operations
- [ ] Update GPIO pin initialization to use `board_config.h`
- [ ] Test audio playback of embedded test tone
- [ ] Test WAV file decoding and playback

### Microphone & Wake Word
- [ ] Test microphone input via `m5_echo_base_read_microphone()`
- [ ] Validate 16 kHz PCM capture
- [ ] Integrate with wake word detection
- [ ] Test wake word "hey jarvis" or similar

### Voice Assistant Pipeline
- [ ] Test STT (Speech-to-Text) via Gemini API
- [ ] Test LLM query (Gemini model)
- [ ] Test TTS (Text-to-Speech) playback
- [ ] Validate end-to-end voice command processing

### WiFi & Network
- [ ] Verify WiFi connectivity on M5 Atom S3R
- [ ] Test API calls to Google Gemini
- [ ] Measure network latency and performance

## Phase 4: Optimization & Refinement

### Memory Optimization
- [ ] Verify 4MB flash partition usage
- [ ] Analyze binary size and optimize if needed
- [ ] Test firmware update mechanism with reduced flash
- [ ] Evaluate PSRAM usage (if available on Echo Base)

### Performance Testing
- [ ] Measure audio latency (microphone to output)
- [ ] Test CPU load during simultaneous operations
- [ ] Optimize DMA buffer sizes for M5 hardware
- [ ] Profile power consumption

### Audio Quality
- [ ] Measure frequency response (20 Hz - 20 kHz)
- [ ] Test THD (Total Harmonic Distortion)
- [ ] Evaluate microphone SNR (Signal-to-Noise Ratio)
- [ ] Validate echo cancellation (if available)

## Phase 5: Documentation & Deployment

### Documentation Updates
- [ ] Update README.md with M5 board instructions
- [ ] Document GPIO pin assignments
- [ ] Create troubleshooting guide for M5 hardware
- [ ] Add M5 board setup instructions

### Build Instructions for M5
- [ ] Document how to select M5 board in `idf.py menuconfig`
- [ ] Provide pre-built `sdkconfig` files for quick setup
- [ ] Create build scripts for automated compilation

### Testing & Validation
- [ ] Create test suite for both boards
- [ ] Verify Korvo1 still works (no regressions)
- [ ] Full integration testing on M5 hardware
- [ ] Create validation checklist for deployment

## Known Issues & Workarounds

### Issue 1: Fixed I2C Pins (RESOLVED)
**Status**: Handled in `m5_echo_base.h`
- I2C is locked to GPIO 19 (SDA) and GPIO 33 (SCL)
- Cannot be remapped without hardware modification
- **Workaround**: Use fixed pins in driver

### Issue 2: Shared I2S Pins (REQUIRES TESTING)
**Status**: Implemented but untested
- Microphone and speaker share GPIO 23, 33
- Full-duplex I2S configured in driver
- **Workaround**: Test actual hardware behavior, may need clock synchronization adjustments

### Issue 3: Reduced Flash (4MB vs 16MB) (ADDRESSED)
**Status**: Partition table created
- Embedded WAV test tone may need compression
- OpenWakeWord models require external storage
- **Workaround**: Use SPIFFS or SD card for large binary data

### Issue 4: Multi-Board Architecture (DESIGN DECISION)
**Status**: Both boards programmed independently
- Atom S3R and Echo Base are separate devices
- Echo Base can operate standalone or controlled by S3R
- **Current Design**: Both receive same firmware, Echo Base handles audio

## Testing Procedures

### Basic Audio Test
```c
// Pseudocode for audio test
1. Initialize m5_echo_base (I2C + I2S)
2. Generate 1 kHz test tone (1 second)
3. Write tone to speaker via I2S
4. Verify audio output
5. Record audio via microphone I2S
6. Verify recorded PCM data
```

### Voice Assistant Test
```
1. Ensure WiFi connected
2. Set Google Gemini API key in menuconfig
3. Record 5-second voice command: "What's the weather?"
4. Process through STT → LLM → TTS
5. Playback response through speaker
6. Verify LED indicates playback
```

### Full Integration Test
```
1. Boot firmware on M5 hardware
2. Observe startup animation on LED
3. Hear test tone playback
4. Trigger wake word detection
5. Issue voice command
6. Receive response and LED feedback
7. Verify logs in serial output
```

## Comparison: Before & After

### Korvo1 (Original)
- 16 MB flash, 8 MB PSRAM
- 12x WS2812 LED ring
- Separate I2S ports (I2S0 speaker, I2S1 PDM mic)
- GPIO 19 for LED, GPIO 1/2 for I2C
- 3.5" display (optional)

### M5 Atom S3R + Echo Base (New)
- 4 MB flash (on-chip), optional PSRAM
- Single SK6812 LED (1 on Echo Base + optional external ring)
- Full-duplex I2S (shared GPIO 23, 33)
- GPIO 19/33 for I2C (fixed), GPIO 46 for LED
- No display (but can add M5Stack display module)

## Files Modified/Created

### New Files
```
✅ PORTING_M5_GUIDE.md                          (This guide)
✅ IMPLEMENTATION_CHECKLIST.md                  (This file)
✅ boards/m5_echo_base.h                        (M5 pin definitions)
✅ boards/korvo1.h                              (Korvo1 pin definitions)
✅ main/board_config.h                          (Board selection)
✅ drivers/audio/m5_echo_base/CMakeLists.txt
✅ drivers/audio/m5_echo_base/include/m5_echo_base.h
✅ drivers/audio/m5_echo_base/src/m5_echo_base.c
✅ sdkconfig.defaults.korvo1
✅ sdkconfig.defaults.m5
✅ partitions_m5.csv
```

### Modified Files
```
✅ CMakeLists.txt                               (Board conditional includes)
✅ main/Kconfig.projbuild                       (Board selection menu)
```

### Files Requiring Updates (Phase 3+)
```
⏳ main/audio_player.c                          (Board abstraction)
⏳ main/audio_player.h                          (Pin constants)
⏳ main/app_main.c                              (Board-specific init)
⏳ components/led_strip/led_strip.c             (Optional: dual LED support)
⏳ README.md                                    (Build instructions)
```

## Next Steps

1. **Immediate**: Test compilation for both boards
   ```bash
   # Test Korvo1 build
   idf.py set-target esp32s3
   idf.py build

   # Test M5 build
   idf.py menuconfig  # Select BOARD_M5_ECHO_BASE
   idf.py build
   ```

2. **Short-term**: Flash and test hardware
   - Flash M5 firmware to Atom Echo Base
   - Verify ES8311 codec initialization
   - Test audio I/O

3. **Medium-term**: Integrate application layer
   - Update audio_player.c for board abstraction
   - Test microphone and speaker
   - Integrate wake word detection

4. **Long-term**: Full validation
   - End-to-end voice assistant testing
   - Performance optimization
   - Production deployment

## Support & Debugging

### Serial Logs
Monitor serial output during development:
```bash
idf.py monitor
```

### Common Issues
1. **I2C communication fails**: Verify GPIO 19, 33 have pull-up resistors
2. **No audio output**: Check I2S pin configuration and codec initialization
3. **Microphone silent**: Verify I2S1 full-duplex mode and pin sharing
4. **LED not responding**: Check GPIO 46 RMT driver initialization

### Additional Resources
- [ESP-IDF I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)
- [ES8311 Datasheet](https://www.espressif.com/)
- [M5 Atom Echo Base Documentation](https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base)

---

**Last Updated**: 2025-12-03
**Status**: Porting infrastructure complete, Phase 2 ready to begin
