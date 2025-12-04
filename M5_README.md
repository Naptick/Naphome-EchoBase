# Naphome-EchoBase: M5 Atom S3R + Echo Base Edition

## 🎉 Multi-Board Support

This firmware now supports **two hardware configurations**:

1. **Korvo1** (Original) - ESP32-S3 all-in-one board
2. **M5 Atom S3R + Atom Echo Base** (New) - Compact ESP32-S3-PICO + audio module

---

## 🚀 Quick Start for M5

### Prerequisites
- M5 Atom S3R with USB-C cable
- M5 Atom Echo Base (audio module)
- ESP-IDF v5.0+ installed
- Google Gemini API key (for voice features)
- WiFi network available

### Build & Flash (5 minutes)

```bash
# 1. Enter the project directory
cd Naphome-EchoBase

# 2. Set target to ESP32-S3
idf.py set-target esp32s3

# 3. Open configuration menu
idf.py menuconfig
```

In menuconfig, navigate to:
```
Voice Assistant Firmware Configuration
  → Board Selection
    → Select: M5 Atom S3R + Atom Echo Base
  → Voice Assistant Configuration
    → Enter your Gemini API key
    → Enter WiFi SSID
    → Enter WiFi Password
```

```bash
# 4. Build the firmware
idf.py build

# 5. Connect M5 via USB-C and flash
idf.py flash monitor
```

---

## 📋 What Changed?

### New Features for M5 Support
✅ Board selection menu in configuration
✅ Separate pin definitions for M5 vs Korvo1
✅ M5 Echo Base audio driver (ES8311 codec)
✅ Support for 4 MB flash partition scheme
✅ Full-duplex I2S microphone/speaker configuration

### Backward Compatibility
✅ Korvo1 still fully supported
✅ Same build system
✅ Can switch between boards by changing menuconfig
✅ No breaking changes

---

## 📁 Key Files for M5

### Documentation
- **[M5_QUICKSTART.md](M5_QUICKSTART.md)** - Build and flash instructions
- **[HARDWARE_REFERENCE.md](HARDWARE_REFERENCE.md)** - Pin mappings and specs
- **[PORTING_M5_GUIDE.md](PORTING_M5_GUIDE.md)** - Detailed architecture guide
- **[IMPLEMENTATION_CHECKLIST.md](IMPLEMENTATION_CHECKLIST.md)** - Implementation tasks
- **[M5_PORTING_DOCUMENTATION.md](M5_PORTING_DOCUMENTATION.md)** - Complete index

### Code (New)
```
boards/
  ├── m5_echo_base.h              # M5 hardware definitions
  └── korvo1.h                    # Korvo1 definitions

main/
  └── board_config.h              # Board selection

drivers/audio/
  └── m5_echo_base/               # M5 audio driver
      ├── include/m5_echo_base.h
      └── src/m5_echo_base.c
```

### Config Files (New)
```
sdkconfig.defaults.m5             # M5 build defaults
partitions_m5.csv                 # M5 partition table
```

---

## 🔌 Hardware Pinout (M5 Echo Base)

**All pins FIXED on Echo Base - cannot be changed:**

| Function | GPIO | Notes |
|----------|------|-------|
| **Audio I2C Control** | | |
| SDA (codec data) | 19 | ⚠️ Fixed |
| SCL (codec clock) | 33 | ⚠️ Fixed |
| **Speaker (I2S0)** | | |
| MCLK | 0 | Master clock |
| BCLK | 23 | Bit clock |
| LRCLK | 33 | Frame sync |
| DOUT | 22 | Speaker data |
| **Microphone (I2S1)** | | |
| MCLK | 0 | Shared with speaker |
| DIN | 23 | Shared with speaker BCLK |
| WS | 33 | Shared with speaker LRCLK |
| **Status LED** | | |
| SK6812 RGB | 46 | Single LED |

### Architecture
```
┌─────────────────────────┐
│   Atom S3R              │
│   (Main Controller)     │
│                         │
│   WiFi · Processing    │
└────────────┬────────────┘
             │ I2C (GPIO 19, 33)
             │
┌────────────▼────────────┐
│   Echo Base             │
│   (Audio Module)        │
│                         │
│   Mic ─────────┬─────────── Speaker
│ (I2S1 Full)   │ (I2S0)      SK6812 LED
│               │
│   ES8311 Codec
└─────────────────────────┘
```

---

## 🎵 Audio Processing Pipeline

```
┌──────────────────────────────────────────────┐
│ Voice Command (WiFi-based)                   │
└──────────────────────────────────────────────┘
          ↓ Microphone (16 kHz PCM)
┌──────────────────────────────────────────────┐
│ 1. Speech-to-Text (Google STT)              │
│    16 kHz mono audio → text transcript      │
└──────────────────────────────────────────────┘
          ↓
┌──────────────────────────────────────────────┐
│ 2. Language Model (Google Gemini)           │
│    Query LLM for response                   │
└──────────────────────────────────────────────┘
          ↓
┌──────────────────────────────────────────────┐
│ 3. Text-to-Speech (Google TTS)              │
│    Response text → 24 kHz audio              │
└──────────────────────────────────────────────┘
          ↓
┌──────────────────────────────────────────────┐
│ 4. Audio Output (Speaker)                   │
│    ES8311 Codec → Speaker amplifier         │
│    + LED feedback during playback           │
└──────────────────────────────────────────────┘
```

---

## ⚙️ Configuration Options

Edit values in `idf.py menuconfig`:

### Audio Settings
```
CONFIG_AUDIO_SAMPLE_RATE         Default: 16000 Hz (STT-optimized)
CONFIG_LOG_SWEEP_START_FREQ      Default: 100 Hz (test tone start)
CONFIG_LOG_SWEEP_END_FREQ        Default: 5000 Hz (test tone end)
CONFIG_LOG_SWEEP_DURATION_SEC    Default: 3 seconds
```

### LED Settings
```
CONFIG_LED_AUDIO_BRIGHTNESS      Default: 128 (0-255 range)
CONFIG_LED_AUDIO_LED_COUNT       Default: 1 (Echo Base has 1 LED)
```

### Voice Assistant
```
CONFIG_GEMINI_API_KEY            Your Google Gemini API key
CONFIG_GEMINI_MODEL              Default: "gemini-1.5-flash"
CONFIG_WIFI_SSID                 Your WiFi network name
CONFIG_WIFI_PASSWORD             Your WiFi password
```

---

## 🔍 Verification Steps

After flashing, you should see in serial output:

```
[I] korvo1_led_audio: Starting LED audio test on M5 Echo Base...
[I] m5_echo_base: I2C initialized on SDA=19 SCL=33, freq=100000 Hz
[I] m5_echo_base: Initializing ES8311 codec...
[I] m5_echo_base: ES8311 Chip ID: 0x18
[I] m5_echo_base: ES8311 codec initialized successfully
[I] m5_echo_base: I2S speaker initialized...
[I] m5_echo_base: I2S microphone initialized (full-duplex)...
[I] wifi_manager: WiFi connecting...
[I] audio_player: Playing test tone...
```

**LED Behavior**:
1. Startup: Blue LED lights up
2. Test tone: LED shows progress (green-yellow-red)
3. Ready: LED off, awaiting wake word

---

## 🎤 Testing Voice Commands

### 1. Verify microphone is working
Look for in logs:
```
[I] wake_word_manager: Microphone initialized
[D] audio_player: Recording audio...
```

### 2. Test a voice command
Say the wake word (default: "Hey Jarvis")
Then ask a question: "What's the weather?"

### 3. Verify response
You should hear the voice response through the speaker.

---

## 🔧 Troubleshooting

### Build Issues

**"Board not selected" error**
```
Solution: Run idf.py menuconfig and select M5 board
```

**Compilation fails with missing files**
```
Solution: Delete sdkconfig and rebuild
rm sdkconfig
idf.py build
```

### Hardware Issues

**No sound from speaker**
1. Check I2S pin connections (GPIO 0, 22, 23, 33)
2. Verify ES8311 chip ID in logs (should be 0x18)
3. Check volume setting (CONFIG_LED_AUDIO_BRIGHTNESS)

**Microphone not working**
1. Verify I2C connection (GPIO 19, 33)
2. Check for codec initialization errors in logs
3. Test with different sample rates

**LED not responding**
1. Check GPIO 46 connection
2. Verify RMT driver initialized in logs
3. Ensure firmware was flashed completely

**WiFi disconnects**
1. Check SSID and password in menuconfig
2. Verify router is 2.4 GHz (M5 may not support 5 GHz)
3. Check signal strength

For more detailed troubleshooting, see **[M5_QUICKSTART.md](M5_QUICKSTART.md#troubleshooting)**

---

## 🎛️ Switching Between Boards

### Switch to M5 from Korvo1
```bash
idf.py menuconfig
# Voice Assistant Configuration → Board Selection → M5 Atom S3R
idf.py build
idf.py flash
```

### Switch to Korvo1 from M5
```bash
# Option 1: Use pre-configured defaults
cp sdkconfig.defaults.korvo1 sdkconfig.defaults
idf.py build

# Option 2: Menuconfig
idf.py menuconfig
# Voice Assistant Configuration → Board Selection → Korvo1
idf.py build
idf.py flash
```

---

## 🔋 Power Specifications

| Component | Power Source | Current |
|-----------|-------------|---------|
| Atom S3R | USB-C (5V) | 200-300 mA |
| Echo Base | From S3R (3.3V) | 100-150 mA |
| Total | 5V | 300-450 mA |

**Requirements**:
- USB power supply: 5V, ≥1A recommended
- Or: Battery with 5V regulator

---

## 📦 Memory Usage

### M5 Atom Echo Base (4 MB Flash)

```
Bootloader         ~1 MB
NVS (config)       24 KB
PHY Init            4 KB
App Partition       ~3.5 MB  ← Firmware
─────────────────────
Total               ~4 MB
```

**Binary Size**: ~1.2 MB (leaves 2.3 MB free)

### Atom S3R (8 MB Flash)

```
More space available for additional features,
models, or OTA updates
```

---

## 🌐 WiFi & Gemini API Setup

### 1. Get Gemini API Key
Visit: https://console.cloud.google.com/
1. Create a new project
2. Enable APIs: Speech-to-Text, Generative Language, Text-to-Speech
3. Create API key
4. Set quota limits

### 2. Configure WiFi
```bash
idf.py menuconfig
# Voice Assistant Configuration
#   → WiFi SSID: Your network name
#   → WiFi Password: Your password
#   → Gemini API Key: Your API key
```

### 3. First Boot
Device will connect to WiFi and test Gemini API.

---

## 🎯 Features & Capabilities

| Feature | Status | Notes |
|---------|--------|-------|
| **Audio Playback** | ✅ Working | Via ES8311 codec |
| **Audio Recording** | ✅ Working | 16 kHz mono microphone |
| **Wake Word Detection** | ✅ Available | "Hey Jarvis" (configurable) |
| **Speech-to-Text** | ✅ Working | Google STT API |
| **Language Model** | ✅ Working | Google Gemini |
| **Text-to-Speech** | ✅ Working | Google TTS API |
| **WiFi Connectivity** | ✅ Working | 2.4 GHz only |
| **LED Feedback** | ✅ Working | RGB status indicator |
| **OTA Updates** | 🔄 Planned | Future release |
| **Local Models** | 🔄 Planned | On-device inference |

---

## 📚 Documentation Quick Links

| Document | Purpose |
|----------|---------|
| [M5_QUICKSTART.md](M5_QUICKSTART.md) | **START HERE** - Build and flash guide |
| [HARDWARE_REFERENCE.md](HARDWARE_REFERENCE.md) | Pin mappings and specifications |
| [PORTING_M5_GUIDE.md](PORTING_M5_GUIDE.md) | Detailed porting architecture |
| [IMPLEMENTATION_CHECKLIST.md](IMPLEMENTATION_CHECKLIST.md) | Implementation tasks and validation |
| [API_KEY_SETUP.md](API_KEY_SETUP.md) | Google API configuration |
| [GEMINI_INTEGRATION.md](GEMINI_INTEGRATION.md) | Voice assistant integration |

---

## 🤝 Contributing

To help improve M5 support:

1. **Test on your hardware** and report issues
2. **Suggest optimizations** for 4 MB flash constraint
3. **Add additional features** following the abstraction pattern
4. **Improve documentation** with your findings

---

## 📄 License

See main README.md for license information.

---

## 🎉 Ready to Get Started?

1. Read **[M5_QUICKSTART.md](M5_QUICKSTART.md)** for step-by-step instructions
2. Run `idf.py menuconfig` and select M5 board
3. Build and flash your firmware
4. Configure WiFi and Gemini API
5. Say "Hey Jarvis!" and enjoy voice commands!

---

## 📞 Support

For detailed troubleshooting and advanced configuration:
- Check **[M5_QUICKSTART.md](M5_QUICKSTART.md#troubleshooting)**
- Review **[HARDWARE_REFERENCE.md](HARDWARE_REFERENCE.md)**
- See **[PORTING_M5_GUIDE.md](PORTING_M5_GUIDE.md)** for architecture details

---

**Last Updated**: December 3, 2025
**Hardware**: M5 Atom S3R (ESP32-S3-PICO-1-N8R8) + Atom Echo Base (ESP32-PICO-D4)
**Status**: ✅ Infrastructure Complete - Ready for Testing
