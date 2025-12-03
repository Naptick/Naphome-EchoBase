# OpenWakeWord Integration for ESP32

## Overview

OpenWakeWord is a Python library, but we can use its pre-trained TFLite models on ESP32 using TensorFlow Lite for Microcontrollers (TFLM).

## Architecture

```
Microphone (16kHz, 16-bit PCM, mono)
    ↓
Audio Preprocessing (MFCC/Melspectrogram)
    ↓
Feature Extraction (Embedding Model)
    ↓
Wake Word Detection (Wake Word Model)
    ↓
Wake Word Detected Callback
```

## Models Required

1. **Feature Models** (required for preprocessing):
   - `melspectrogram.tflite` - Converts audio to melspectrogram
   - `embedding_model.tflite` - Extracts audio embeddings

2. **Wake Word Models** (choose one or more):
   - `hey_jarvis_v0.1.tflite` - "Hey Jarvis" wake word
   - `alexa_v0.1.tflite` - "Alexa" wake word
   - `hey_mycroft_v0.1.tflite` - "Hey Mycroft" wake word
   - `hey_rhasspy_v0.1.tflite` - "Hey Rhasspy" wake word
   - `timer_v0.1.tflite` - Timer-related phrases
   - `weather_v0.1.tflite` - Weather-related phrases

## Setup Steps

### 1. Download Models

Run the download script:

```bash
cd components/openwakeword
chmod +x download_models.sh
./download_models.sh
```

This will download the required TFLite models to `components/openwakeword/models/`.

### 2. Add TensorFlow Lite for Microcontrollers (TFLM)

TFLM needs to be added as a component. Options:

**Option A: Use ESP-IDF TFLM component (if available)**
- Check if ESP-IDF has TFLM support
- Add as managed component

**Option B: Add TFLM as submodule**
```bash
cd components/openwakeword
git submodule add https://github.com/tensorflow/tflite-micro.git tflm
```

**Option C: Use ESP-SR (Espressif Speech Recognition)**
- ESP-SR is Espressif's optimized speech recognition framework
- May be easier to integrate
- Check: https://github.com/espressif/esp-sr

### 3. Update Implementation

The current `openwakeword_esp32.cpp` is a placeholder. To use actual OpenWakeWord:

1. **Include TFLM headers**
2. **Load TFLite models** from flash or SPIFFS
3. **Implement audio preprocessing** (melspectrogram)
4. **Run inference** through feature models and wake word models
5. **Process results** and trigger callbacks

### 4. Model Storage

Models can be:
- **Embedded in firmware** (using EMBED_FILES in CMakeLists.txt)
- **Stored in SPIFFS partition** (loaded at runtime)
- **Downloaded OTA** (advanced)

For now, embedding is simplest but increases firmware size.

## Current Status

⚠️ **Placeholder Implementation**

The current code simulates wake word detection. To make it functional:

1. ✅ OpenWakeWord library added as submodule
2. ✅ Model download script created
3. ⚠️ TFLM integration needed
4. ⚠️ Audio preprocessing implementation needed
5. ⚠️ Model loading and inference needed

## Next Steps

1. **Download models:**
   ```bash
   cd components/openwakeword
   ./download_models.sh
   ```

2. **Add TFLM component** (choose one approach above)

3. **Update `openwakeword_esp32.cpp`** to:
   - Load TFLite models
   - Implement melspectrogram preprocessing
   - Run inference through models
   - Process predictions

4. **Test with actual audio** from microphone

## Alternative: ESP-SR

If TFLM integration proves complex, consider using **ESP-SR** (Espressif Speech Recognition) which:
- Is optimized for ESP32
- Has built-in wake word detection
- May be easier to integrate
- Has good documentation

See: https://github.com/espressif/esp-sr

## Resources

- [OpenWakeWord GitHub](https://github.com/dscripka/openWakeWord)
- [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers)
- [ESP-SR Documentation](https://github.com/espressif/esp-sr)
- [Model Download URLs](https://github.com/dscripka/openWakeWord/releases)
