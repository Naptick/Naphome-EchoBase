# Hardware Reference Card

Quick reference for hardware specifications and pin configurations.

## M5 Atom S3R + Atom Echo Base vs Korvo1

### Processor & Memory Comparison

| Feature | Korvo1 | M5 Atom S3R | M5 Atom Echo Base |
|---------|--------|------------|-------------------|
| **Processor** | ESP32-S3 | ESP32-S3-PICO-1-N8R8 | ESP32-PICO-D4 |
| **CPU Cores** | 2x 240 MHz | 2x 240 MHz | 1x 160 MHz |
| **Flash** | 16 MB | 8 MB | 4 MB |
| **PSRAM** | 8 MB | 8 MB (on-chip) | None |
| **Total Memory** | 24 MB | 16 MB | 4 MB |
| **Role** | All-in-one | Main Controller | Audio Module |

### Form Factor

| Aspect | Korvo1 | M5 Atom S3R | Echo Base |
|--------|--------|------------|-----------|
| **Size** | ~100×100×80 mm | 24×24×13.5 mm | Small module |
| **Weight** | ~250g | ~25g | ~50g |
| **Connectors** | USB, Grove | USB-C, Grove | I2C connector |
| **Enclosure** | Plastic | Tiny metal cube | Plastic module |

## Complete Pin Mapping

### M5 Atom Echo Base - Fixed GPIO (Cannot be changed)

```
┌─────────────────────────────────────────┐
│      Atom Echo Base GPIO Map            │
│                                         │
│  I2C (ES8311 Codec Control):           │
│    GPIO 19 → SDA (FIXED)                │
│    GPIO 33 → SCL (FIXED)                │
│                                         │
│  Speaker Output (I2S0):                │
│    GPIO 0  → MCLK (Master Clock)       │
│    GPIO 22 → DOUT (Data Out)           │
│    GPIO 23 → BCLK (Bit Clock)          │
│    GPIO 33 → LRCLK (Frame Sync)        │
│                                         │
│  Microphone Input (I2S1):              │
│    GPIO 0  → MCLK (shared with I2S0)   │
│    GPIO 23 → DIN (Data In) **SHARED    │
│    GPIO 33 → WS (Word Select) **SHARED │
│                                         │
│  Status LED:                           │
│    GPIO 46 → SK6812 RGB LED (1x)       │
│                                         │
│  ⚠️  WARNING: All pins marked FIXED    │
│      cannot be reassigned!             │
└─────────────────────────────────────────┘
```

### M5 Atom S3R - User GPIO Available

```
┌─────────────────────────────────────────┐
│      Atom S3R GPIO Map                  │
│                                         │
│  Typically available for user code:    │
│    GPIO 5, 6, 7, 8, 9                  │
│    (Others used for internal functions) │
│                                         │
│  Grove I2C (if available):              │
│    GPIO 21/22 (flexible)                │
│                                         │
│  Recommended:                          │
│    - Add external WS2812 LED ring      │
│    - Connect user buttons              │
│    - Expand with Grove sensors         │
└─────────────────────────────────────────┘
```

### Korvo1 (Reference)

```
┌─────────────────────────────────────────┐
│      Korvo1 GPIO Map                    │
│                                         │
│  Speaker (I2S0):                       │
│    GPIO 39 → DOUT                      │
│    GPIO 40 → BCLK                      │
│    GPIO 41 → LRCLK                     │
│    GPIO 42 → MCLK                      │
│                                         │
│  Microphone (I2S1 PDM):                │
│    GPIO 35 → DIN                       │
│    GPIO 36 → BCLK                      │
│    GPIO 37 → WS                        │
│    GPIO 0  → MCLK                      │
│                                         │
│  I2C (Codec):                          │
│    GPIO 1  → SDA                       │
│    GPIO 2  → SCL                       │
│                                         │
│  LED Ring:                             │
│    GPIO 19 → 12x WS2812 (RMT)          │
└─────────────────────────────────────────┘
```

## I2S Interface Details

### Full-Duplex Operation (M5 Echo Base)

The Echo Base uses **full-duplex I2S** with shared pins:

```
Microphone (I2S1)        Speaker (I2S0)
     DIN ─────┬──────────┬─── BCLK (GPIO 23)
     WS  ─────┼──────────┼─── LRCLK (GPIO 33)
     MCLK ────┴──────────┴─── MCLK (GPIO 0)
```

**Configuration**:
- Both use GPIO 0 as master clock
- Both use GPIO 23 as bit clock (shared)
- Both use GPIO 33 as frame sync (shared)
- **Microphone data**: GPIO 23 (shared with speaker BCLK)
- **Speaker data**: GPIO 22 (separate)

**Implication**: Cannot simultaneously record and playback without clock conflict handling.

### Separate Operation (Korvo1)

Korvo1 uses **independent I2S ports** (I2S0 and I2S1):

```
Microphone (I2S1)        Speaker (I2S0)
     DIN ──→ GPIO 35     DOUT ──→ GPIO 39
     BCLK → GPIO 36      BCLK → GPIO 40
     WS   → GPIO 37      LRCLK → GPIO 41
     MCLK → GPIO 0       MCLK → GPIO 42
```

**Implication**: Can simultaneously record and playback independently.

## Audio Codec Details

### ES8311 Specification

| Parameter | Value |
|-----------|-------|
| **I2C Address** | 0x18 (7-bit) |
| **Sample Rates** | 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k, 44.1k, 48k Hz |
| **Bit Depths** | 16-bit, 18-bit, 20-bit, 24-bit, 32-bit |
| **Channels** | Mono, Stereo |
| **Communication** | I2S (I2C for control) |
| **Supply** | 3.3V |
| **Current Draw** | ~50 mA (typical) |

### Configuration on Both Boards

```
I2C Control:
  Bus Speed: 100 kHz
  Slave Address: 0x18
  Protocol: Standard I2C

Audio:
  Sample Rate: 16 kHz (optimized for STT)
  Bit Width: 16-bit
  Channels: Mono (1 channel)
  Format: I2S, Left-justified
```

## LED Configuration

### M5 Echo Base LED

- **Type**: SK6812 (NeoPixel-compatible)
- **GPIO**: 46 (fixed)
- **Count**: 1x LED (single RGB)
- **Protocol**: RMT (Remote Control Module)
- **Voltage**: 3.3V
- **Current**: ~50 mA at full brightness

### Korvo1 LED

- **Type**: WS2812 (NeoPixel)
- **GPIO**: 19 (RMT)
- **Count**: 12x RGB LEDs
- **Protocol**: RMT
- **Voltage**: 3.3V to 5V
- **Current**: Up to 150 mA

### Adding External LED Ring to M5 Atom S3R

Option: Connect 12x WS2812 ring to available GPIO:

```
Recommended Pin: GPIO 5 or 6
Connection:
  GND → GND
  +5V → +5V (via 330Ω resistor)
  DIN → GPIO 5 (via 1kΩ resistor)
```

## Power Specifications

| Board | Input Voltage | Current Draw | Notes |
|-------|---------------|-------------|-------|
| **M5 Atom S3R** | 5V (USB-C) | 200-300 mA (typical) | Active WiFi |
| **Atom Echo Base** | 3.3V (via S3R) | 100-150 mA | Audio module |
| **Korvo1** | 5V (USB) | 300-500 mA | All-in-one |

## Temperature Ratings

| Component | Operating Range | Storage Range |
|-----------|-----------------|---------------|
| **M5 Atom S3R** | 0°C to 40°C | -20°C to 60°C |
| **Atom Echo Base** | 0°C to 40°C | -20°C to 60°C |
| **Korvo1** | 0°C to 40°C | -20°C to 60°C |

## Microphone Specifications

### M5 Echo Base (Built-in MEMS)

- **Type**: Digital MEMS microphone
- **Frequency Range**: 100 Hz - 8 kHz (typical)
- **SNR**: ~90 dB (Signal-to-Noise Ratio)
- **Sensitivity**: -37 dBFS @ 1 kHz
- **Channels**: 1 (mono)
- **Interface**: I2S (via ES8311)
- **Sample Rate**: 16 kHz (optimized)

### Korvo1 (PDM Microphone)

- **Type**: PDM microphone
- **Frequency Range**: 100 Hz - 8 kHz
- **SNR**: ~90 dB
- **Interface**: I2S1 (PDM mode)
- **Sample Rate**: 16 kHz (configurable)

## Speaker Specifications

### M5 Echo Base

- **Type**: Mono speaker with NS4150B amplifier
- **Output Power**: ~2W @ 3Ω
- **Frequency Response**: 200 Hz - 10 kHz
- **Volume Range**: Software controlled via ES8311
- **Interface**: I2S (via ES8311)

### Korvo1

- **Type**: Mono speaker
- **Output Power**: Depends on power supply
- **Frequency Response**: Similar range
- **Interface**: I2S0 (via ES8311)

## Connectivity

### WiFi

| Board | Standard | Frequency | Range |
|-------|----------|-----------|-------|
| **M5 Atom S3R** | 802.11 b/g/n | 2.4 GHz | ~100m (open space) |
| **Atom Echo Base** | None | - | Via S3R |
| **Korvo1** | 802.11 b/g/n | 2.4 GHz | ~100m (open space) |

### Bluetooth (if available)

- M5 Atom S3R: BLE (Bluetooth Low Energy)
- Korvo1: BLE available

## Partition Table Differences

### Korvo1 (16 MB Flash)

```
┌──────────────────────────────┐
│ Bootloader + OTA (2 MB)      │
├──────────────────────────────┤
│ NVS (24 KB)                  │
├──────────────────────────────┤
│ PHY Init (4 KB)              │
├──────────────────────────────┤
│ App Partition (8-10 MB)      │ ← Large space
├──────────────────────────────┤
│ Free Space (~4 MB)           │
└──────────────────────────────┘
```

### M5 Atom Echo Base (4 MB Flash)

```
┌──────────────────────────────┐
│ Bootloader (1 MB)            │
├──────────────────────────────┤
│ NVS (24 KB)                  │
├──────────────────────────────┤
│ PHY Init (4 KB)              │
├──────────────────────────────┤
│ App Partition (3.5 MB)       │ ← Limited space
├──────────────────────────────┤
│ Free Space (~4 KB)           │
└──────────────────────────────┘
```

## I2C Bus Details

### M5 Echo Base I2C

```
Bus Speed:     100 kHz (standard mode)
SDA (GPIO 19): Pull-up required
SCL (GPIO 33): Pull-up required
Devices:       ES8311 @ 0x18
Pull-ups:      Usually built-in on module
```

### Korvo1 I2C

```
Bus Speed:     100 kHz
SDA (GPIO 1):  Pull-up resistor needed
SCL (GPIO 2):  Pull-up resistor needed
Devices:       ES8311 @ 0x18
Pull-ups:      Usually on board
```

## Quick Comparison Table

| Feature | Korvo1 | M5 S3R | Echo Base |
|---------|--------|--------|-----------|
| **Form Factor** | Large | Tiny | Module |
| **All-in-one?** | Yes | No | No |
| **Flash** | 16 MB | 8 MB | 4 MB |
| **PSRAM** | 8 MB | 8 MB | None |
| **Mic Mode** | PDM (I2S1) | - | I2S1 Full-duplex |
| **Speaker Mode** | I2S0 | - | I2S0 Full-duplex |
| **LED Count** | 12x | User GPIO | 1x |
| **Display** | Optional | None | None |
| **Cost** | Higher | Low | Low |
| **Expandability** | Limited | High | Limited |
| **Best For** | Full featured | Space-constrained | Audio-focused |

## Checklist for Hardware Setup

### M5 Atom S3R + Echo Base

- [ ] USB-C cable for S3R
- [ ] Jumper wires (optional, if not using connectors)
- [ ] Pull-up resistors for I2C (if not on module)
- [ ] 5V power supply
- [ ] WiFi network available
- [ ] Google Gemini API key

### Korvo1

- [ ] USB cable (data + power)
- [ ] Micro SD card (if using external storage)
- [ ] WiFi network available
- [ ] Google Gemini API key

---

**Last Updated**: 2025-12-03
**Hardware Versions**: M5 Atom S3R (ESP32-S3-PICO-1-N8R8) + Atom Echo Base (ESP32-PICO-D4)
