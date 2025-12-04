# Phase 2: Hardware Integration & Testing Implementation Guide

## 🎯 Objective

Complete hardware testing and integration on M5 Atom S3R + Echo Base. Verify all components work correctly before moving to application layer (Phase 3).

**Estimated Duration**: 2-4 hours (depending on hardware issues)
**Success Criteria**: All tests pass, audio I/O verified, logs show correct operation

---

## ✅ Pre-Implementation Checklist

Before starting, ensure you have:

- [ ] M5 Atom S3R with USB-C cable
- [ ] M5 Atom Echo Base (audio module)
- [ ] Working ESP-IDF environment (v5.0+)
- [ ] Google Gemini API key (with quotas enabled)
- [ ] WiFi network credentials
- [ ] Serial terminal tool (built into `idf.py monitor`)
- [ ] Time to run tests and troubleshoot

---

## 🚀 Phase 2: Step-by-Step Implementation

### Step 1: Verify Build System (15 minutes)

#### 1.1 Test Korvo1 Build (No M5 Hardware Needed)

```bash
cd Naphome-EchoBase

# Clear old configuration
rm sdkconfig sdkconfig.old 2>/dev/null

# Set target
idf.py set-target esp32s3

# Use Korvo1 defaults
cp sdkconfig.defaults.korvo1 sdkconfig.defaults

# Build
idf.py build
```

**Expected Result**: Build completes successfully with no errors
**Log Output Should Show**:
```
[100%] Built target naphome-korvo1
Wrote XXXX bytes to file naphome-korvo1.bin
```

**If it fails**:
- Check ESP-IDF installation: `idf.py --version`
- Verify components: `ls components/`
- Check CMakeLists.txt syntax

#### 1.2 Test M5 Build Configuration

```bash
# Clear configuration
rm sdkconfig sdkconfig.old 2>/dev/null

# Open menuconfig
idf.py menuconfig
```

**Navigation in menuconfig**:
```
1. Arrow down to "Voice Assistant Firmware Configuration"
2. Press Enter to expand
3. Arrow down to "Board Selection"
4. Press Enter to expand
5. Select "M5 Atom S3R + Atom Echo Base"
6. Press S to save
7. Press Q to exit
```

**Verify**:
```bash
# Check that M5 board is selected
grep "CONFIG_BOARD_M5_ECHO_BASE=y" sdkconfig
# Should output: CONFIG_BOARD_M5_ECHO_BASE=y
```

#### 1.3 Build for M5

```bash
idf.py build
```

**Expected Result**: Build completes for M5 configuration

---

### Step 2: Hardware Connectivity (10 minutes)

#### 2.1 Connect M5 Hardware

1. Attach Atom Echo Base to Atom S3R (using connectors)
   - If no connectors available, use GPIO jumpers (see manual)
2. Connect USB-C cable to Atom S3R
3. Connect to computer

#### 2.2 Identify Serial Port

```bash
# macOS/Linux
ls -la /dev/tty* | grep -E "usbserial|usbmodem|ACM"

# Windows (PowerShell)
Get-WmiObject WIN32_SerialPort
```

**Expected**: `/dev/tty.usbserial-XXXXXX` or similar

---

### Step 3: Initial Flash & Serial Output (15 minutes)

#### 3.1 Flash Firmware to M5

```bash
# Make sure menuconfig shows M5 selected
grep "CONFIG_BOARD_M5_ECHO_BASE=y" sdkconfig

# Flash and monitor simultaneously
idf.py flash monitor
```

**Steps**:
1. Tool starts uploading (2-3 minutes)
2. Serial monitor opens
3. Device restarts and shows logs

#### 3.2 Observe Startup Sequence

Watch the serial output. You should see:

```
I (0) cpu_start: ESP-IDF v5.0 or later
I (0) cpu_start: Starting scheduler on APP CPU.
I (XXX) board_config: Initializing board configuration
I (XXX) m5_echo_base: I2C initialized on SDA=19 SCL=33, freq=100000 Hz
I (XXX) m5_echo_base: Initializing ES8311 codec...
I (XXX) m5_echo_base: ES8311 Chip ID: 0x18
I (XXX) m5_echo_base: ES8311 codec initialized successfully
I (XXX) m5_echo_base: I2S speaker initialized
I (XXX) m5_echo_base: I2S microphone initialized (full-duplex)
```

**✅ Success Indicators**:
- No crashes or watchdog resets
- Chip ID = 0x18 (confirms ES8311 found)
- All initialization messages appear

**❌ Failure Indicators**:
- `I2C read failed` - Check GPIO 19, 33 connections, pull-ups
- `Codec not found` - Verify hardware connections
- Watchdog timeout - Check stack sizes or CPU load

**If fails**: See "Troubleshooting" section below

---

### Step 4: Test Audio Codec (I2C Communication) (10 minutes)

#### 4.1 Verify Codec Is Responding

From serial monitor, look for:
```
[I] m5_echo_base: ES8311 Chip ID: 0x18
```

This means I2C communication is working.

#### 4.2 Manual I2C Test (Optional)

Edit `main/app_main.c` and add test code:

```c
// Add after codec initialization
uint8_t reg_value;
if (es8311_read_reg(I2C_NUM_0, 0x00, &reg_value) == ESP_OK) {
    ESP_LOGI(TAG, "I2C read test passed: reg[0x00] = 0x%02x", reg_value);
} else {
    ESP_LOGE(TAG, "I2C read test FAILED");
}
```

Rebuild and flash to verify I2C is working.

---

### Step 5: Test Speaker Output (I2S0) (20 minutes)

#### 5.1 Prepare Test Tone

The firmware should generate a test tone automatically at startup.

**Expected behavior**:
1. Device boots
2. Generates 3 second log sweep (20 Hz → 20 kHz)
3. Plays through speaker
4. LED shows progress (green → yellow → red)

#### 5.2 Listen for Test Tone

1. Position speaker near your ear
2. Watch LED on Echo Base
3. Expect to hear:
   - Chirp sound (increasing frequency)
   - Duration: 3-5 seconds
   - No distortion or crackling

#### 5.3 Measure Output (Optional)

Use phone or audio analyzer to verify:
- [ ] Sound is audible (> -40 dBFS)
- [ ] No obvious distortion
- [ ] Frequency sweeps from low to high

**If no sound**:
1. Check I2S pins (GPIO 0, 22, 23, 33)
2. Verify speaker is connected
3. Check codec volume (should be mid-range by default)
4. See troubleshooting below

#### 5.4 Debug I2S Output

Add debug code to `main/app_main.c`:

```c
// After I2S initialization
uint8_t test_data[16] = {0x00, 0x00}; // Silent PCM
size_t bytes_written;
esp_err_t ret = m5_echo_base_write_speaker(&m5_dev, test_data,
                                            sizeof(test_data),
                                            &bytes_written,
                                            pdMS_TO_TICKS(100));
if (ret == ESP_OK) {
    ESP_LOGI(TAG, "I2S write test passed: %d bytes written", bytes_written);
} else {
    ESP_LOGE(TAG, "I2S write test FAILED: %s", esp_err_to_name(ret));
}
```

---

### Step 6: Test Microphone Input (I2S1) (15 minutes)

#### 6.1 Record Audio

Add recording test code to `main/app_main.c`:

```c
// Record 1 second of audio at 16 kHz
#define SAMPLE_RATE 16000
#define RECORD_TIME_SEC 1
#define BUFFER_SIZE (SAMPLE_RATE * 2 * RECORD_TIME_SEC) // 16-bit = 2 bytes

int16_t *recording_buffer = malloc(BUFFER_SIZE);
if (!recording_buffer) {
    ESP_LOGE(TAG, "Memory allocation failed");
    return;
}

size_t bytes_read;
esp_err_t ret = m5_echo_base_read_microphone(&m5_dev,
                                              recording_buffer,
                                              BUFFER_SIZE,
                                              &bytes_read,
                                              pdMS_TO_TICKS(2000));

if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Mic read successful: %d bytes", bytes_read);

    // Print first 10 samples to verify data
    for (int i = 0; i < 10 && i < bytes_read/2; i++) {
        ESP_LOGI(TAG, "Sample[%d] = %d", i, recording_buffer[i]);
    }
} else {
    ESP_LOGE(TAG, "Mic read FAILED: %s", esp_err_to_name(ret));
}

free(recording_buffer);
```

#### 6.2 Analyze Recorded Data

1. Make noise near microphone while recording
2. Check serial output for sample values
3. Values should vary (not all zeros)
4. Check SNR (signal should be > noise floor)

**Expected Sample Range**: ±5000 to ±32000 (16-bit signed)
- All zeros = microphone not working
- All full scale (±32768) = clipping

#### 6.3 Test Full-Duplex Mode (Advanced)

Create a test that simultaneously records and plays:

```c
// Simultaneous record + playback
// Record 2 seconds while playing test tone
// Verify no crosstalk

int16_t playback_buffer[SAMPLE_RATE];  // 1 sec @ 16kHz
int16_t record_buffer[SAMPLE_RATE * 2];

// Fill playback with 1 kHz sine wave
for (int i = 0; i < SAMPLE_RATE; i++) {
    float freq = 1000.0;
    playback_buffer[i] = (int16_t)(32767 * sin(2*M_PI*freq*i/SAMPLE_RATE));
}

// Start playback in task
// Simultaneously read microphone
// Analyze recorded data for 1 kHz tone

// Check if recorded audio contains playback + mic input
```

---

### Step 7: Test LED Indicator (10 minutes)

#### 7.1 Verify LED Communication

The firmware should control the SK6812 LED on GPIO 46.

**Expected behavior during startup**:
1. LED turns on (usually blue)
2. LED brightness varies with audio playback
3. LED transitions from green → yellow → red during test tone

#### 7.2 Manual LED Test

Add test code to `main/app_main.c`:

```c
// Test LED color cycling
const uint8_t colors[][3] = {
    {255, 0, 0},     // Red
    {0, 255, 0},     // Green
    {0, 0, 255},     // Blue
    {255, 255, 0},   // Yellow
    {255, 0, 255},   // Magenta
};

for (int i = 0; i < 5; i++) {
    m5_echo_base_set_led_color(&m5_dev, colors[i][0],
                                colors[i][1], colors[i][2]);
    vTaskDelay(pdMS_TO_TICKS(500));
}

// Turn off
m5_echo_base_set_led_color(&m5_dev, 0, 0, 0);
```

#### 7.3 Debug LED Issues

If LED doesn't respond:
1. Verify GPIO 46 pin connection
2. Check RMT driver initialization (should print log)
3. Verify LED voltage (should be 3.3V or 5V depending on LED)
4. Test with multimeter for GPIO 46 output

---

### Step 8: WiFi Connectivity Test (15 minutes)

#### 8.1 Configure WiFi

```bash
idf.py menuconfig
# Navigate to: Voice Assistant Configuration
#   → WiFi SSID: Your network name
#   → WiFi Password: Your password
```

#### 8.2 Build & Flash

```bash
idf.py build
idf.py flash monitor
```

#### 8.3 Verify WiFi Connection

Watch for in serial output:
```
I (XXX) wifi_manager: Starting WiFi...
I (XXX) wifi_manager: WiFi SSID: Your Network
I (XXX) wifi_manager: Connecting...
I (XXX) wifi_manager: Connected!
I (XXX) wifi_manager: IP Address: 192.168.x.x
```

**If fails**:
1. Verify SSID and password are correct
2. Check if router is 2.4 GHz (5 GHz may not be supported)
3. Try connecting with same device to verify credentials
4. Check signal strength in location

#### 8.4 Ping Test

Once connected, verify internet:
```bash
# From your computer
ping 8.8.8.8

# You should see successful responses
```

---

### Step 9: Test Gemini API Integration (20 minutes)

#### 9.1 Configure API Key

```bash
idf.py menuconfig
# Navigate to: Voice Assistant Configuration
#   → Gemini API Key: [Your key from console.cloud.google.com]
```

**How to get API key**:
1. Go to https://console.cloud.google.com
2. Create new project
3. Enable APIs: Speech-to-Text, Generative Language, Text-to-Speech
4. Create API key (Credentials → Create Credentials → API Key)

#### 9.2 Test STT (Speech-to-Text)

Add test code:

```c
// Test STT with a sample audio file or recording
// Example: Convert recorded microphone data to text
const char *audio_data = "..."; // Audio bytes
esp_err_t ret = gemini_speech_to_text(audio_data, &transcript);
if (ret == ESP_OK) {
    ESP_LOGI(TAG, "STT Result: %s", transcript);
}
```

#### 9.3 Test LLM (Language Model)

```c
// Query Gemini with a prompt
const char *prompt = "What is 2+2?";
char *response = NULL;
esp_err_t ret = gemini_query(prompt, &response);
if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Gemini Response: %s", response);
}
```

#### 9.4 Test TTS (Text-to-Speech)

```c
// Convert text to speech
const char *text = "Hello, I am ready";
esp_err_t ret = gemini_text_to_speech(text, audio_buffer, &audio_len);
if (ret == ESP_OK) {
    // Play audio through speaker
    size_t written;
    m5_echo_base_write_speaker(&m5_dev, audio_buffer,
                                audio_len, &written,
                                pdMS_TO_TICKS(5000));
}
```

---

### Step 10: Full Integration Test (30 minutes)

#### 10.1 End-to-End Voice Command

Complete test sequence:

1. **Record** (via microphone for 5 seconds)
2. **Process** (STT → LLM → TTS)
3. **Playback** (response audio)
4. **Feedback** (LED shows progress)

#### 10.2 Test Voice Commands

Try these commands (after pressing wake button or detecting wake word):

| Command | Expected Response |
|---------|-------------------|
| "Hello" | Device acknowledges |
| "What time is it?" | Provides current time |
| "What's the weather?" | Weather for your location |
| "Tell me a joke" | Tells a joke |
| "Stop" | Stops processing |

#### 10.3 Measure Performance

Record metrics:

```
Latency (ms):          [STT] + [LLM] + [TTS] = TOTAL
Audio Quality (dB):    Signal level: _____ SNR: _____
LED Response (ms):     Time from start to first LED change: _____
Memory Usage (MB):     Heap free: _____ PSRAM free: _____
```

---

## 🐛 Troubleshooting

### Issue: "I2C Read Failed" on Startup

**Symptoms**:
```
E (XXX) m5_echo_base: I2C read failed (reg=0x00): ESP_ERR_TIMEOUT
```

**Causes**:
- I2C pins not connected properly
- Missing pull-up resistors on GPIO 19, 33
- ES8311 not powered

**Solutions**:
1. Check physical connections:
   ```bash
   # Verify GPIO output with multimeter
   # GPIO 19 SDA should toggle
   # GPIO 33 SCL should toggle
   ```

2. Add pull-up resistors (10kΩ):
   - GPIO 19 to 3.3V
   - GPIO 33 to 3.3V

3. Verify power:
   - Echo Base should be powered (check LED)
   - 3.3V should be stable

### Issue: No Sound from Speaker

**Symptoms**:
- Serial logs show ES8311 initialized
- No audio from speaker
- LED works fine

**Causes**:
- I2S pins disconnected
- Speaker muted or volume too low
- Codec not configured for output

**Solutions**:
1. Check I2S connections (GPIO 0, 22, 23, 33)
2. Increase volume:
   ```bash
   idf.py menuconfig
   # Audio → Increase CONFIG_LED_AUDIO_BRIGHTNESS
   ```
3. Add debug: Verify I2S writes return ESP_OK

### Issue: Microphone Records Silence

**Symptoms**:
- Recorded samples are all zeros or very low
- No change when making noise near mic

**Causes**:
- Microphone not enabled in codec
- Input level too low
- Microphone not receiving data

**Solutions**:
1. Check microphone is physically connected
2. Test with louder noise first
3. Debug I2S read in code:
   ```c
   // Verify bytes are being read
   ESP_LOGI(TAG, "Read %d bytes from mic", bytes_read);
   ```

### Issue: WiFi Won't Connect

**Symptoms**:
```
I (XXX) wifi_manager: WiFi SSID: MyNetwork
I (XXX) wifi_manager: Connecting...
E (XXX) wifi_manager: Failed to connect (timeout)
```

**Causes**:
- Wrong SSID or password
- Router using 5 GHz only (M5 supports 2.4 GHz only)
- Signal too weak
- WiFi credentials format error

**Solutions**:
1. Verify credentials:
   ```bash
   # Test credentials on another device
   ```
2. Ensure router broadcasts 2.4 GHz
3. Move closer to router
4. Restart router and try again

### Issue: Gemini API Fails

**Symptoms**:
```
E (XXX) gemini: HTTP request failed: ESP_ERR_TIMEOUT
E (XXX) gemini: API error: 401 Unauthorized
```

**Causes**:
- API key invalid or expired
- APIs not enabled in Google Cloud
- Quota exceeded
- Network connectivity issue

**Solutions**:
1. Verify API key is valid:
   ```bash
   # Check if APIs are enabled in Google Cloud Console
   # Speech-to-Text: enabled
   # Generative Language: enabled
   # Text-to-Speech: enabled
   ```
2. Check quota:
   ```bash
   # Google Cloud Console → APIs & Services → Quotas
   ```
3. Test connectivity:
   ```bash
   # On M5: Try pinging 8.8.8.8
   ```

---

## ✅ Validation Checklist

After completing all steps, verify:

### Hardware
- [ ] ES8311 codec initializes (Chip ID = 0x18)
- [ ] I2C communication working (no timeouts)
- [ ] Speaker plays test tone audibly
- [ ] Microphone captures voice clearly
- [ ] LED responds to commands
- [ ] No watchdog resets or crashes

### Connectivity
- [ ] WiFi connects to your network
- [ ] IP address assigned correctly
- [ ] Can ping external hosts
- [ ] DNS resolves correctly

### API Integration
- [ ] Gemini API key is valid
- [ ] STT converts audio to text
- [ ] LLM responds to queries
- [ ] TTS converts text to audio

### Performance
- [ ] Startup time < 5 seconds
- [ ] Latency < 3 seconds per query
- [ ] Memory usage within limits
- [ ] No memory leaks

### Audio Quality
- [ ] Speaker volume adequate (>20 dB)
- [ ] Microphone SNR > 50 dB
- [ ] No audio distortion
- [ ] Frequency response 100 Hz - 8 kHz

---

## 📊 Expected Results Summary

### Serial Output on Startup
```
I (0) cpu_start: ESP-IDF v5.0
I (245) m5_echo_base: I2C initialized on SDA=19 SCL=33, freq=100000 Hz
I (255) m5_echo_base: Initializing ES8311 codec...
I (265) m5_echo_base: ES8311 Chip ID: 0x18
I (275) m5_echo_base: ES8311 codec initialized successfully
I (285) m5_echo_base: I2S speaker initialized: MCLK=0 BCLK=23 LRCLK=33 DOUT=22
I (295) m5_echo_base: I2S microphone initialized (full-duplex)
I (300) wifi_manager: Starting WiFi...
I (350) wifi_manager: WiFi connected!
I (360) wifi_manager: IP Address: 192.168.1.x
I (370) voice_assistant: Voice assistant ready
I (380) app_main: Playing test tone...
```

### Performance Metrics (Expected)
- Boot to ready: ~2 seconds
- Test tone playback: 3-5 seconds
- Microphone latency: <100 ms
- WiFi connection: <5 seconds
- Gemini API response: 1-3 seconds
- Memory available: >2 MB heap, >4 MB PSRAM

---

## 🎯 Next Steps After Phase 2

Once Phase 2 is complete:

1. **Phase 3: Application Integration**
   - Update audio_player.c with board abstraction
   - Integrate full voice assistant pipeline
   - Test end-to-end commands

2. **Phase 4: Optimization**
   - Profile performance
   - Optimize memory usage
   - Improve audio quality

3. **Phase 5: Production**
   - Final validation
   - Documentation updates
   - Release preparation

---

## 📞 Getting Help

If you encounter issues:

1. Check **[M5_QUICKSTART.md](M5_QUICKSTART.md#troubleshooting)**
2. Review **[HARDWARE_REFERENCE.md](HARDWARE_REFERENCE.md)**
3. Check serial logs for error messages
4. Refer to **[PORTING_M5_GUIDE.md](PORTING_M5_GUIDE.md)** for architecture details

---

**Status**: Phase 2 Ready to Begin
**Estimated Time**: 4-6 hours for complete testing
**Next Milestone**: All hardware components verified
