# Google Gemini STT-LLM-TTS Integration

## Overview

Complete voice assistant integration using Google Gemini API for Speech-to-Text (STT), Large Language Model (LLM), and Text-to-Speech (TTS) functionality.

## Architecture

```
┌─────────────┐
│  Microphone │ (16kHz, 16-bit PCM, mono)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Wake Word   │ (OpenWakeWord)
│  Detection  │
└──────┬──────┘
       │ (Wake word detected)
       ▼
┌─────────────┐
│ Record Audio│ (5 seconds or until silence)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│     STT     │ (Google Cloud Speech-to-Text)
│ Audio→Text  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│     LLM     │ (Google Gemini API)
│ Text→Text   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│     TTS     │ (Google Cloud Text-to-Speech)
│ Text→Audio  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Speaker   │ (ES8311 Codec, 48kHz)
└─────────────┘
```

## Components

### 1. Gemini API Component (`components/gemini/`)

**Files:**
- `include/gemini_api.h` - API definitions
- `gemini_api.c` - Implementation

**Functions:**
- `gemini_api_init()` - Initialize with API key and model
- `gemini_stt()` - Speech-to-Text conversion
- `gemini_llm()` - LLM query/response
- `gemini_tts()` - Text-to-Speech synthesis

### 2. Voice Assistant Manager (`main/voice_assistant.c`)

**Orchestrates the complete flow:**
- Waits for wake word detection
- Records audio command
- Processes through STT → LLM → TTS
- Plays audio response

### 3. WiFi Manager (`main/wifi_manager.c`)

**Handles WiFi connectivity:**
- Connect to WiFi network
- Get IP address
- Connection status monitoring

## API Endpoints

### Speech-to-Text
- **Service**: Google Cloud Speech-to-Text API
- **Endpoint**: `https://speech.googleapis.com/v1/speech:recognize`
- **Format**: 16-bit PCM, 16kHz mono, Base64 encoded

### LLM (Gemini)
- **Service**: Google Gemini API
- **Endpoint**: `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent`
- **Models**: 
  - `gemini-1.5-flash` (fast, default)
  - `gemini-1.5-pro` (more capable)
- **Format**: JSON request/response

### Text-to-Speech
- **Service**: Google Cloud Text-to-Speech API
- **Endpoint**: `https://texttospeech.googleapis.com/v1/text:synthesize`
- **Format**: LINEAR16 PCM, 24kHz (configurable)
- **Voice**: `en-US-Neural2-D` (configurable)

## Configuration

### 1. Get API Keys

You need API keys from [Google Cloud Console](https://console.cloud.google.com/):

1. **Create a project** (or use existing)
2. **Enable APIs:**
   - Cloud Speech-to-Text API
   - Generative Language API (Gemini)
   - Cloud Text-to-Speech API
3. **Create API key:**
   - Go to "APIs & Services" → "Credentials"
   - Create API key
   - (Optional) Restrict key to specific APIs

### 2. Configure WiFi

```c
wifi_manager_config_t wifi_cfg = {
    .ssid = "YOUR_WIFI_SSID",
    .password = "YOUR_WIFI_PASSWORD"
};
wifi_manager_connect(&wifi_cfg);
```

### 3. Initialize Voice Assistant

```c
voice_assistant_config_t va_config = {
    .gemini_api_key = "YOUR_API_KEY",
    .gemini_model = "gemini-1.5-flash"
};
voice_assistant_init(&va_config);
voice_assistant_start();
```

## Usage Flow

1. **Wake Word Detection:**
   - OpenWakeWord listens continuously
   - When wake word detected, callback triggered

2. **Command Recording:**
   - Microphone captures audio (5 seconds or until silence)
   - Audio sent to STT

3. **Speech-to-Text:**
   - Audio converted to text
   - Text logged and passed to LLM

4. **LLM Processing:**
   - Text query sent to Gemini
   - Response generated

5. **Text-to-Speech:**
   - LLM response converted to audio
   - Audio generated at 24kHz

6. **Audio Playback:**
   - TTS audio played through ES8311 codec
   - (May need resampling from 24kHz to 48kHz)

## Current Status

✅ **Implemented:**
- Gemini API client (STT, LLM, TTS)
- WiFi manager
- Voice assistant orchestration
- Integration with wake word detection
- Integration with audio player

⚠️ **To Complete:**
- Actual wake word → command recording flow
- Audio resampling (24kHz TTS → 48kHz playback)
- Voice Activity Detection (VAD) for better recording
- Error handling and retries
- API key configuration via menuconfig

## Limitations

1. **Network Dependency**: Requires WiFi and internet connection
2. **Latency**: Network round-trips add latency (~1-3 seconds total)
3. **Cost**: API usage may incur costs (check Google Cloud pricing)
4. **Rate Limits**: Subject to API rate limits
5. **Sample Rate**: TTS outputs 24kHz, may need resampling for 48kHz playback

## Future Enhancements

- [ ] Local STT option (e.g., Whisper.cpp) for offline operation
- [ ] Streaming STT for lower latency
- [ ] Audio resampling pipeline (24kHz → 48kHz)
- [ ] Voice Activity Detection (VAD)
- [ ] Conversation context/memory
- [ ] Multiple language support
- [ ] Custom wake words
- [ ] LED feedback during processing
- [ ] API key storage in NVS (encrypted)
- [ ] OTA updates for models/config

## Testing

### Manual Test

```c
// Test STT
int16_t test_audio[16000]; // 1 second at 16kHz
// ... record audio ...
char text[512];
gemini_stt(test_audio, 16000, text, sizeof(text));

// Test LLM
char response[2048];
gemini_llm("Hello, how are you?", response, sizeof(response));

// Test TTS
int16_t audio[48000];
size_t samples;
gemini_tts(response, audio, 48000, &samples);
```

### Integration Test

1. Flash firmware with API key configured
2. Connect to WiFi
3. Say wake word
4. Speak command
5. Listen for response

## Files Created

- `components/gemini/include/gemini_api.h`
- `components/gemini/gemini_api.c`
- `components/gemini/CMakeLists.txt`
- `components/gemini/README.md`
- `main/voice_assistant.h`
- `main/voice_assistant.c`
- `main/wifi_manager.h`
- `main/wifi_manager.c`

## Resources

- [Google Gemini API Docs](https://ai.google.dev/docs)
- [Google Cloud Speech-to-Text](https://cloud.google.com/speech-to-text/docs)
- [Google Cloud Text-to-Speech](https://cloud.google.com/text-to-speech/docs)
- [Gemini Models](https://ai.google.dev/models/gemini)
- [API Pricing](https://cloud.google.com/pricing)

## Notes

- The implementation uses separate Google Cloud APIs for STT/TTS
- Gemini API is used specifically for LLM functionality
- All API calls are synchronous (blocking)
- Consider implementing async/streaming for better UX
- API keys should be stored securely (NVS encryption recommended)
