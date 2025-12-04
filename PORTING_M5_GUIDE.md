# Porting Guide: Korvo1 to M5 Atom S3R + Atom Echo Base

## Overview

This guide documents the porting process from the ESP32-S3 Korvo1 development board to the M5Stack combination of **Atom S3R** (main controller) and **Atom Echo Base** (audio module).

## Hardware Comparison

| Component | Korvo1 | M5 Atom S3R | Atom Echo Base |
|-----------|--------|------------|-----------------|
| **Processor** | ESP32-S3 | ESP32-S3-PICO-1-N8R8 | ESP32-PICO-D4 (slave) |
| **Flash** | 16 MB | 8 MB | - |
| **PSRAM** | 8 MB | 8 MB (on-chip) | - |
| **Audio Codec** | ES8311 | - | ES8311 |
| **Microphone** | PDM (I2S1) | - | Built-in MEMS |
| **Speaker** | Mono (I2S0) | - | Mono (full-duplex) |
| **LED** | 12x WS2812 RGB Ring | User GPIO | SK6812 (RGB) |
| **I2C** | Codec (GPIO1/2) | Configurable | Audio (GPIO19/33 fixed) |
| **Button** | Power + Boot | Reset + Boot | Programmable key |

## Architecture Changes

### Single Device vs. Multi-Device

**Korvo1 (Original)**: Single ESP32-S3 with integrated codec
```
ESP32-S3
├─ Audio Input (PDM I2S1)
├─ Audio Codec (I2C + I2S0)
├─ LED Control (GPIO19)
└─ WiFi
```

**M5 Atom S3R + Echo Base (New)**: Distributed architecture
```
Atom S3R (Master)          Atom Echo Base (Slave)
├─ WiFi                    ├─ ES8311 Codec
├─ Processing              ├─ Microphone (I2S1)
├─ Voice Assistant Logic   ├─ Speaker (I2S0)
└─ GPIO Control            └─ SK6812 LED

Connected via GPIO (I2C/UART) or Serial/USB
```

## Critical Hardware Differences

### 1. **Memory Configuration**: Different Layout
**Korvo1**: 16 MB flash + 8 MB PSRAM (separate)
**M5 Atom S3R**: 8 MB flash + 8 MB PSRAM (on-chip)
**Atom Echo Base**: 4 MB flash (on-chip), no PSRAM

- **Impact**: M5 has same total memory as Korvo1 but distributed differently
- **Advantage**: PSRAM on-chip = faster access on Atom S3R
- **Challenge**: Echo Base has only 4 MB (no PSRAM) - requires optimization
- **Solution**:
  - Keep WAV test tone but consider compressing
  - Use external storage (SPIFFS on Echo Base) for large files
  - Optimize partition table for 4 MB Echo Base (3.5 MB available for app)

### 2. **Codec I2C Connection**
| Signal | Korvo1 | Echo Base | Notes |
|--------|--------|-----------|-------|
| I2C SDA | GPIO 1 | GPIO 19 | **Fixed - cannot change** |
| I2C SCL | GPIO 2 | GPIO 33 | **Fixed - cannot change** |

**Key**: Echo Base has **fixed I2C pins**. Do not attempt to remap.

### 3. **Audio I2S Configuration**

#### Speaker Output (I2S0)
| Signal | Korvo1 | Echo Base | Notes |
|--------|--------|-----------|-------|
| BCLK | GPIO 40 | GPIO 23 | Bit clock |
| LRCLK | GPIO 41 | GPIO 33 | Frame sync |
| DOUT | GPIO 39 | GPIO 22 | Data out |
| MCLK | GPIO 42 | GPIO 0 | Master clock |

**Korvo1 pins**: 39, 40, 41, 42
**Echo Base pins**: 0, 22, 23, 33

#### Microphone Input (I2S1)
| Signal | Korvo1 | Echo Base |
|--------|--------|-----------|
| DIN | GPIO 35 | GPIO 23 (shared) |
| BCLK | GPIO 36 | GPIO 33 (shared) |
| WS | GPIO 37 | GPIO 19 (shared) |
| MCLK | GPIO 0 | GPIO 0 |

**Critical Issue**: Echo Base uses **full-duplex I2S** with shared pins between microphone and speaker.

### 4. **LED Configuration**
| Aspect | Korvo1 | Atom S3R | Echo Base |
|--------|--------|----------|-----------|
| Type | WS2812 (NeoPixel) | GPIO-configurable | SK6812 (RGB) |
| Count | 12 LEDs | User-defined | 1 LED (fixed) |
| GPIO | GPIO 19 | User choice | GPIO 46 (fixed) |
| Protocol | RMT | RMT (configurable) | Same as WS2812 |

**Strategy**:
- Option A: Use Atom S3R GPIO for external WS2812 ring
- Option B: Use Echo Base SK6812 for status only (simpler)
- Option C: Add external I2S/SPI LED controller

### 5. **Multi-Device Communication**

Echo Base can communicate with Atom S3R via:
1. **UART Serial** (built-in protocol)
2. **I2C** (if additional board connector available)
3. **SPI** (if interface exposed)

Currently, **no direct connector exists** - they appear to be independent boards for now.

## Porting Steps

### Phase 1: Infrastructure Setup

#### 1.1 Create M5-Specific Board Definitions
```
drivers/audio/m5_echo_base/
├── include/
│   ├── m5_echo_base.h
│   └── m5_echo_base_pins.h
└── src/
    └── m5_echo_base.c
```

#### 1.2 Update CMakeLists.txt
- Add M5 board in EXTRA_COMPONENT_DIRS
- Update flash size to 8 MB
- Conditional compilation for board type

#### 1.3 Create sdkconfig.m5
- Reduce partition sizes for 8 MB flash
- Set correct I2C/I2S pins
- Disable unused components (Korvo1-specific drivers)

### Phase 2: Audio Driver Adaptation

#### 2.1 Abstract Hardware Pins
Current: Hard-coded GPIO pins in app_main.c
```c
static const audio_player_config_t s_audio_config = {
    .i2s_port = I2S_NUM_0,
    .bclk_gpio = GPIO_NUM_40,  // Korvo1
    ...
};
```

New: Board-selectable at build time
```c
#if defined(CONFIG_BOARD_KORVO1)
    // Korvo1 pins
#elif defined(CONFIG_BOARD_M5_ECHO_BASE)
    // Echo Base pins
#endif
```

#### 2.2 Update I2S Pin Mappings
- Replace hardcoded GPIO numbers with board-specific constants
- Create `m5_echo_base_pins.h` with:
  - Speaker I2S pins (GPIO 0, 22, 23, 33)
  - Microphone I2S pins (shared with speaker)
  - I2C pins for ES8311 (GPIO 19, 33)
  - LED GPIO (Echo Base: GPIO 46)

#### 2.3 Handle Full-Duplex I2S
Echo Base uses shared I2S pins for simultaneous capture/playback:
- May require I2S mode adjustments
- Test with simultaneous record+playback
- Consider DMA buffer sizing

### Phase 3: LED Control

#### Option A: Keep 12-LED Ring on Atom S3R (Recommended for feature parity)
- Use free GPIO on S3R for RMT-driven WS2812
- Synchronize with Echo Base audio
- Same animation logic

#### Option B: Echo Base Single LED (Simplest)
- Use GPIO 46 (SK6812)
- Reduce animation to single-pixel status indicator
- Simpler implementation

#### Option C: Daisy-chain both LEDs
- Control both via single RMT channel
- Requires different protocol management

**Recommendation**: Start with Option B (single LED on Echo Base), upgrade to Option A later.

### Phase 4: Memory Optimization

#### 4.1 Flash Size Reduction (16 MB → 8 MB)
- Current test tone: ~256 KB
- Embedded models (if used): Remove or store in SPIFFS
- Partition table adjustment

#### 4.2 Partition Table Changes
**Current (16 MB)**:
```
nvs:        0x6000  (NVS)
phy_init:   0x2000  (WiFi calibration)
factory:    Remaining space
app:        Large
```

**New (8 MB)**:
```
nvs:        0x6000  (same)
phy_init:   0x2000  (same)
app:        ~6.5 MB (reduced)
```

**Action**: Update `partitions.csv` or use ESP-IDF default for 8 MB

### Phase 5: Configuration System

#### 5.1 Menuconfig Updates
Add new options in `Kconfig.projbuild`:
```
choice BOARD_SELECTION
    prompt "Target Board"
    config BOARD_KORVO1
        bool "Korvo1 (Original)"
    config BOARD_M5_ECHO_BASE
        bool "M5 Atom S3R + Echo Base"
endchoice
```

#### 5.2 Create Board-Specific Defaults
- `sdkconfig.defaults.korvo1`
- `sdkconfig.defaults.m5`

### Phase 6: Build System Changes

#### 6.1 Update Root CMakeLists.txt
```cmake
if(CONFIG_BOARD_M5_ECHO_BASE)
    list(APPEND EXTRA_COMPONENT_DIRS
        "${PROJECT_ROOT}/drivers/audio/m5_echo_base")
elseif(CONFIG_BOARD_KORVO1)
    list(APPEND EXTRA_COMPONENT_DIRS
        "${PROJECT_ROOT}/drivers/audio/korvo1")
endif()
```

#### 6.2 Update main/CMakeLists.txt
```cmake
set(COMPONENT_SRCS
    app_main.c
    audio_player.c
    audio_eq.c
    voice_assistant.c
    wake_word_manager.c
    wifi_manager.c
)

if(CONFIG_BOARD_KORVO1)
    # Korvo1-specific sources
elseif(CONFIG_BOARD_M5_ECHO_BASE)
    # M5-specific sources
endif()
```

## Detailed Pin Mapping

### Atom Echo Base I2S/I2C Pinout

**Important Note**: The Atom Echo Base pins are **FIXED** and cannot be reassigned.

#### Speaker Output (I2S0)
```
GPIO 0  → I2S0_MCLK    (Master Clock)
GPIO 22 → I2S0_DOUT    (Data Out, Speaker)
GPIO 23 → I2S0_BCLK    (Bit Clock)
GPIO 33 → I2S0_LRCLK   (L/R Clock, Frame Sync)
```

#### Microphone Input (I2S1, Full-Duplex)
```
GPIO 0  → I2S1_MCLK    (Master Clock, shared with I2S0)
GPIO 23 → I2S1_DIN     (Data In, Mic - shared with BCLK)
GPIO 33 → I2S1_WS      (Word Select - shared with LRCLK)
```

#### ES8311 Codec Control (I2C)
```
GPIO 19 → I2C_SDA      (Data, FIXED)
GPIO 33 → I2C_SCL      (Clock, FIXED)
```

#### Status LED
```
GPIO 46 → SK6812 RGB LED (NeoPixel-compatible)
```

### Translation Table

| Function | Korvo1 GPIO | Echo Base GPIO |
|----------|-------------|-----------------|
| Speaker BCLK | 40 | 23 |
| Speaker LRCLK | 41 | 33 |
| Speaker DOUT | 39 | 22 |
| Speaker MCLK | 42 | 0 |
| Mic DIN | 35 | 23* |
| Mic BCLK | 36 | 33* |
| Mic WS | 37 | 19* |
| I2C SDA | 1 | 19 |
| I2C SCL | 2 | 33 |
| LED | 19 | 46 |

*Shared pins - full-duplex I2S

## Implementation Priority

### Phase 1: Minimal Working Version (HIGH PRIORITY)
- [ ] Create M5 board definitions
- [ ] Update pin mappings in audio_player
- [ ] Test audio playback with test tone
- [ ] Single LED status indicator

### Phase 2: Core Features (MEDIUM PRIORITY)
- [ ] Microphone input (test recording)
- [ ] Wake word detection
- [ ] STT integration test
- [ ] Multi-LED animation (if adding external ring)

### Phase 3: Full Integration (LOW PRIORITY)
- [ ] Full voice assistant pipeline
- [ ] OTA updates for 8 MB flash
- [ ] Advanced audio EQ
- [ ] Performance optimization

## Testing Checklist

- [ ] Compile without errors on 8 MB flash
- [ ] Audio codec initializes on Echo Base
- [ ] Test tone plays through speaker
- [ ] Microphone captures audio
- [ ] Wake word detection works
- [ ] LED indicates audio playback
- [ ] WiFi connects
- [ ] Voice assistant responds end-to-end

## Known Issues & Workarounds

### Issue 1: Shared I2S Pins
**Problem**: Microphone and speaker share I2S pins (GPIO 23, 33)
**Impact**: Cannot do simultaneous record+playback cleanly
**Workaround**:
- Use separate I2S channels if ESP32-S3 supports
- Time-multiplex microphone/speaker
- Check Atom Echo Base firmware for default handling

### Issue 2: Fixed I2C Pins
**Problem**: Echo Base locks I2C to GPIO 19/33
**Impact**: Cannot use standard I2C pins
**Solution**: Already handled in new driver

### Issue 3: Flash Size Reduction
**Problem**: 8 MB vs 16 MB
**Impact**: Limited space for embedded audio/models
**Solution**: Use SPIFFS or external storage

### Issue 4: Multi-Board Communication
**Problem**: Atom S3R and Echo Base are separate devices
**Impact**: Requires separate firmware or connector
**Solution**:
- Option A: Flash both boards independently (current plan)
- Option B: Use serial protocol if physical connection exists
- Option C: Control Echo Base from S3R via protocol

## File Changes Summary

### New Files
```
drivers/audio/m5_echo_base/
├── CMakeLists.txt
├── include/
│   ├── m5_echo_base.h
│   └── m5_echo_base_pins.h
└── src/
    └── m5_echo_base.c

sdkconfig.defaults.m5
boards/
├── m5_echo_base.h
└── korvo1.h

main/board_config.h (board selection macros)
```

### Modified Files
```
CMakeLists.txt (add M5 board to EXTRA_COMPONENT_DIRS)
main/CMakeLists.txt (conditional compilation)
main/app_main.c (board-specific configuration)
main/audio_player.c (pin abstraction)
main/audio_player.h (pin constants)
Kconfig.projbuild (add board selection menu)
```

## References

- [M5 Atom Echo Base Docs](https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base)
- [M5 Atom S3R Docs](https://docs.m5stack.com/en/core/AtomS3)
- [ESP32-S3 I2S Driver Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2s.html)
- [ES8311 Audio Codec](https://www.espressif.com/)

## Next Steps

1. Create M5 Echo Base board definition files
2. Implement pin mapping abstraction
3. Test audio codec initialization
4. Debug I2S full-duplex operation
5. Validate microphone input
6. Integrate wake word detection
7. End-to-end testing

---

**Last Updated**: 2025-12-03
**Target**: M5 Atom S3R + Atom Echo Base
**Status**: Planning phase
