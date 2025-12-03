# Google Gemini STT-LLM-TTS Integration

This component provides integration with Google Gemini API for Speech-to-Text (STT), Large Language Model (LLM), and Text-to-Speech (TTS) functionality.

## Overview

The Gemini integration enables a complete voice assistant pipeline:
1. **STT**: Convert speech to text using Google Cloud Speech-to-Text API
2. **LLM**: Process text queries and get responses using Gemini API
3. **TTS**: Convert text responses to speech using Google Cloud Text-to-Speech API

## API Endpoints

### Speech-to-Text
- **Service**: Google Cloud Speech-to-Text API
- **Endpoint**: `https://speech.googleapis.com/v1/speech:recognize`
- **Format**: 16-bit PCM, 16kHz mono
- **Encoding**: Base64 encoded audio in JSON

### LLM (Gemini)
- **Service**: Google Gemini API
- **Endpoint**: `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent`
- **Models**: `gemini-1.5-flash` (fast), `gemini-1.5-pro` (more capable)
- **Format**: JSON request/response

### Text-to-Speech
- **Service**: Google Cloud Text-to-Speech API
- **Endpoint**: `https://texttospeech.googleapis.com/v1/text:synthesize`
- **Format**: LINEAR16 PCM, 24kHz (configurable)
- **Encoding**: Base64 encoded audio in JSON response

## Configuration

### API Key Setup

You need API keys for:
1. **Google Cloud Speech-to-Text API**
2. **Google Gemini API** (same key may work for both)
3. **Google Cloud Text-to-Speech API**

Get your API key from:
- [Google Cloud Console](https://console.cloud.google.com/)
- Enable the required APIs:
  - Cloud Speech-to-Text API
  - Generative Language API (Gemini)
  - Cloud Text-to-Speech API

### Usage

```c
// Initialize Gemini API
gemini_config_t config = {
    .api_key = "YOUR_API_KEY",
    .model = "gemini-1.5-flash"
};
gemini_api_init(&config);

// Speech-to-Text
char text[512];
gemini_stt(audio_samples, num_samples, text, sizeof(text));

// LLM
char response[2048];
gemini_llm("What's the weather?", response, sizeof(response));

// Text-to-Speech
int16_t audio[48000];
size_t samples;
gemini_tts(response, audio, 48000, &samples);
```

## Voice Assistant Integration

The `voice_assistant` component orchestrates the complete flow:

```
Wake Word Detected
    ↓
Record Audio (5 seconds)
    ↓
STT: Audio → Text
    ↓
LLM: Text → Response
    ↓
TTS: Response → Audio
    ↓
Play Audio
```

## Current Status

⚠️ **Note**: This implementation uses Google Cloud APIs, not direct Gemini endpoints for STT/TTS.

For STT and TTS, you may want to:
- Use Gemini's multimodal capabilities (if available)
- Or use separate Google Cloud APIs as implemented
- Or use alternative TTS services

## Limitations

1. **API Keys**: Requires valid Google Cloud API keys
2. **Network**: Requires WiFi connection
3. **Latency**: Network round-trips add latency
4. **Cost**: API usage may incur costs (check Google Cloud pricing)
5. **Rate Limits**: API calls are subject to rate limits

## Future Enhancements

- [ ] Local STT option (e.g., Whisper.cpp)
- [ ] Streaming STT for lower latency
- [ ] Caching common responses
- [ ] Voice activity detection (VAD) for better recording
- [ ] Audio resampling for TTS output
- [ ] Support for multiple languages
- [ ] Custom wake words

## Resources

- [Google Gemini API Docs](https://ai.google.dev/docs)
- [Google Cloud Speech-to-Text](https://cloud.google.com/speech-to-text)
- [Google Cloud Text-to-Speech](https://cloud.google.com/text-to-speech)
- [Gemini Models](https://ai.google.dev/models/gemini)
