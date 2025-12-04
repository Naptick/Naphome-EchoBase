# Quick Start: Building for M5 Atom Echo Base

## Prerequisites

1. **ESP-IDF v5.0+** installed and configured
2. **M5 Atom Echo Base** hardware
3. **M5 Atom S3R** (optional, for enhanced LED ring)
4. **Google Gemini API key** (for voice assistant features)
5. **USB-C cable** for programming and serial communication

## Setup Steps

### 1. Clone the Repository
```bash
git clone https://github.com/your-repo/Naphome-EchoBase.git
cd Naphome-EchoBase
```

### 2. Configure for M5 Board
```bash
# Set target to ESP32-S3
idf.py set-target esp32s3

# Open menuconfig
idf.py menuconfig
```

### 3. Select M5 Board in Menuconfig
Navigate to:
```
Voice Assistant Firmware Configuration
    → Board Selection
        → M5 Atom S3R + Atom Echo Base
```

Also configure:
```
Voice Assistant Firmware Configuration
    → Voice Assistant Configuration
        → Enter Gemini API Key
        → Enter WiFi SSID
        → Enter WiFi Password
```

Save and exit (Ctrl+S → Q).

### 4. Build the Firmware
```bash
idf.py build
```

**Expected output**:
```
[100%] Built target naphome-korvo1
Wrote 1234567 bytes (999999 compressed) to address 0x10000 in 12.34 seconds...
```

### 5. Flash to M5 Echo Base
```bash
# Connect M5 Echo Base via USB-C cable
idf.py flash monitor
```

**Expected startup log**:
```
I (0) cpu_start: Starting scheduler on APP CPU.
I (245) m5_echo_base: I2C initialized on SDA=19 SCL=33, freq=100000 Hz
I (255) m5_echo_base: Initializing ES8311 codec...
I (265) m5_echo_base: ES8311 Chip ID: 0x18
I (325) m5_echo_base: ES8311 codec initialized successfully
...
```

## Hardware Pinout Reference

### Audio Connections (Fixed on Echo Base)

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA | 19 | Fixed, cannot change |
| I2C SCL | 33 | Fixed, cannot change |
| Speaker MCLK | 0 | Master clock |
| Speaker BCLK | 23 | Bit clock |
| Speaker LRCLK | 33 | Frame sync (shared with SCL) |
| Speaker DOUT | 22 | Data out |
| Mic DIN | 23 | Shared with speaker BCLK |
| Mic WS | 33 | Shared with speaker LRCLK |
| LED (SK6812) | 46 | Single RGB LED |

### Microphone & Speaker Sharing

⚠️ **Important**: The Echo Base uses **full-duplex I2S** with shared pins!

- Speaker and microphone **cannot be used simultaneously** without additional configuration
- Current driver supports sequential operation (record then play, or vice versa)
- Full-duplex operation requires careful clock synchronization

## Testing the Build

### 1. Audio Playback Test
You should hear three log-sweep test tones after startup. This verifies:
- ES8311 codec is working
- I2S speaker output is functional
- Audio playback volume is appropriate

### 2. LED Test
The single SK6812 LED on Echo Base should:
- Light up during startup (blue)
- Show progress animation during audio playback
- Turn off when idle

### 3. Microphone Test
To verify microphone input:
1. Edit `main/app_main.c` to uncomment microphone test code
2. Rebuild and flash
3. Monitor serial output for audio samples

### 4. Full Voice Assistant Test
1. Ensure WiFi is connected (check logs)
2. Say the wake word (default: "hey jarvis")
3. Speak a command (e.g., "what's the weather")
4. Listen for the voice response

## Configuration Options

### Audio Sample Rate
Edit `main/Kconfig.projbuild`:
```
CONFIG_AUDIO_SAMPLE_RATE=16000  # Default for STT (optimize for voice)
CONFIG_AUDIO_SAMPLE_RATE=44100  # High quality audio
```

### LED Brightness
```
CONFIG_LED_AUDIO_BRIGHTNESS=128  # Range 0-255
```

### Log Level
```
CONFIG_LOG_DEFAULT_LEVEL=2  # 0=None, 1=Error, 2=Warning, 3=Info, 4=Debug
```

## Troubleshooting

### Build Fails with "Board Not Selected"
**Solution**: Run `idf.py menuconfig` and select M5 board under Board Selection

### "No device found" during flash
**Solution**:
1. Check USB cable connection
2. Press boot button while connecting device
3. Try different USB port

### Audio Not Working
**Checklist**:
1. [ ] I2C communication successful (check logs for "ES8311 Chip ID")
2. [ ] I2S pins correct (GPIO 0, 22, 23, 33)
3. [ ] Audio volume not muted (check `CONFIG_LED_AUDIO_BRIGHTNESS`)
4. [ ] Speaker is powered on

### Microphone Not Capturing
**Checklist**:
1. [ ] I2S1 initialized successfully (check logs)
2. [ ] GPIO 23, 33 not conflicting with speaker
3. [ ] Microphone enabled in codec (check ES8311 ADC_CTRL)
4. [ ] Full-duplex mode not conflicting with playback

### WiFi Disconnects
**Troubleshooting**:
1. Verify SSID and password in menuconfig
2. Check if AP is on 2.4 GHz (M5 Echo Base may not support 5 GHz)
3. Check signal strength (should be > -70 dBm)

### API Calls Fail
**Ensure**:
1. Gemini API key is valid and enabled
2. Required APIs enabled: Speech-to-Text, Generative Language, Text-to-Speech
3. Quota limits not exceeded
4. Network connectivity working

## Building for Korvo1 (Revert)

To switch back to original Korvo1 board:

```bash
idf.py menuconfig
# Select: Board Selection → Korvo1 Development Board
idf.py build
```

Or use the pre-configured default:
```bash
cp sdkconfig.defaults.korvo1 sdkconfig.defaults
idf.py build
```

## Next Steps

After confirming basic functionality:

1. **Add External LED Ring** (optional):
   - Connect 12x WS2812 ring to free GPIO on Atom S3R
   - Update `CONFIG_LED_AUDIO_STRIP_GPIO` in menuconfig

2. **Optimize Audio Quality**:
   - Adjust microphone gain
   - Configure audio EQ for better speech recognition
   - Test noise suppression

3. **Implement Custom Wake Word**:
   - Train custom wake word model
   - Integrate OpenWakeWord library

4. **Add OTA Updates**:
   - Implement firmware update over WiFi
   - Use ESP-IDF OTA partition scheme

## Additional Resources

- [M5 Atom Echo Base Documentation](https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base)
- [M5 Atom S3R Documentation](https://docs.m5stack.com/en/core/AtomS3)
- [ESP-IDF Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
- [Google Gemini API Documentation](https://ai.google.dev/docs)

## Support

For issues or questions:
1. Check serial logs: `idf.py monitor`
2. Review `IMPLEMENTATION_CHECKLIST.md`
3. Consult `PORTING_M5_GUIDE.md` for detailed architecture
4. Open an issue on GitHub

---

**Last Updated**: 2025-12-03
**Firmware Version**: M5 Atom Echo Base Support
