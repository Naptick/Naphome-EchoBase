# Shared Code Policy: Korvo1 ↔ Echo Base

This document defines which code should be identical across both repositories and which can differ.

## The Golden Rule

**All business logic, API integrations, and algorithms should be identical across both boards.** Only I2S hardware configuration should differ.

## Shared Code (Must be Identical)

### ✅ Always Keep Synchronized

#### 1. Gemini API Integration
```
components/gemini/
├── gemini_api.c      ← SHARED (identical on both boards)
├── gemini_api.h      ← SHARED
└── README.md
```

**Why**: Speech-to-Text, LLM, and TTS logic doesn't depend on hardware.

**File location**: `components/gemini/`
**Sync strategy**: Always merge from upstream

**Example - Same code on both boards**:
```c
// Both Korvo1 and Echo Base use identical logic
esp_err_t voice_assistant_transcribe_audio(const uint8_t *audio_data, size_t len)
{
    // STT request to Gemini API - IDENTICAL on both boards
    http_client_config_t config = {
        .host = "generativelanguage.googleapis.com",
        .path = "/v1beta/speech:recognize",
        .method = HTTP_METHOD_POST,
    };
    // ... same implementation
}
```

#### 2. Voice Assistant Logic
```
main/
├── voice_assistant.c  ← SHARED (identical on both boards)
├── voice_assistant.h  ← SHARED
└── README.md
```

**Why**: Conversation flow, context management, and response handling are board-agnostic.

**Example - Same on both boards**:
```c
// Same conversation loop on both boards
esp_err_t voice_assistant_handle_response(const char *gemini_response)
{
    // Parse JSON response
    // Generate TTS
    // Play audio via audio_player (which handles board-specific I2S)
    // Return result
}
```

#### 3. Wake Word Detection
```
components/openwakeword/
├── openwakeword_esp32.cpp     ← SHARED
├── openwakeword_wrapper.cpp   ← SHARED
└── README.md
```

**Why**: Wake word detection algorithm is identical; only I2S input differs.

**How it abstracts I2S differences**:
```c
// wrapper abstracts microphone input
// Uses audio_abstraction_get_microphone_pins() to handle board differences
void openwakeword_feed_audio(const int16_t *samples, int num_samples)
{
    // Same processing on both boards
    // I2S input differences handled by board abstraction layer
}
```

#### 4. Audio EQ/DSP
```
main/
├── audio_eq.c        ← SHARED (identical on both boards)
├── audio_eq.h        ← SHARED
└── README.md
```

**Why**: DSP algorithms don't depend on hardware.

**Example**:
```c
// Same EQ processing on both boards
audio_eq_process(&eq_state, audio_samples, num_samples);
```

#### 5. WiFi Management
```
main/
├── wifi_manager.c    ← SHARED (identical on both boards)
├── wifi_manager.h    ← SHARED
└── README.md
```

**Why**: WiFi driver and connection logic is identical; both boards use same ESP32 WiFi module.

#### 6. Configuration & Constants
```
main/
├── Kconfig.projbuild ← MOSTLY SHARED
└── sdkconfig.defaults ← BOARD-SPECIFIC (different for each board)
```

**Shared in Kconfig.projbuild**:
- API key configuration
- WiFi settings
- Model selection
- Feature flags

**Differs in sdkconfig.defaults**:
- Board selection (CONFIG_BOARD_KORVO1 vs CONFIG_BOARD_M5_ECHO_BASE)
- I2S sample rates
- LED configuration
- Memory settings

## Board-Specific Code (Can Differ)

### ⚠️ Board Differences Only

#### 1. GPIO/I2S Pin Configuration
```
boards/
├── korvo1.h          ← Korvo1 specific pins
└── m5_echo_base.h    ← Echo Base specific pins
```

**Why**: Different hardware, different pin mappings.

**Syncing strategy**: Keep separate, never merge between boards

```c
// Korvo1
#define GPIO_I2S0_MCLK (42)
#define GPIO_I2S1_DIN  (35)

// Echo Base
#define GPIO_I2S0_MCLK (0)
#define GPIO_I2S1_DIN  (23)  // Shared with BCLK!
```

#### 2. Audio Player Implementation Details
```
main/
├── audio_player.c    ← Uses board-specific pins from board_config.h
└── audio_player.h    ← Interface is identical, implementation differs
```

**Why**: Hardware audio I2S differs between boards.

**Interface** (identical):
```c
// Same function signature on both boards
esp_err_t audio_player_init(const audio_player_config_t *cfg);
esp_err_t audio_player_play_wav(const uint8_t *wav_data, size_t len, callback);
```

**Implementation** (board-specific):
```c
// Korvo1: Separate I2S0 and I2S1
// Echo Base: Shared I2S pins
// But interface is same - caller doesn't care about I2S details
```

#### 3. LED Animations
```
main/
├── app_main.c        ← LED animations based on RGB_LED_COUNT
└── board_config.h    ← Defines RGB_LED_COUNT per board
```

**Why**: Different LED counts (12 vs 1).

**Handling**:
```c
// Auto-adapts to board
for (int i = 0; i < RGB_LED_COUNT; i++) {  // 12 on Korvo1, 1 on Echo Base
    led_strip_set_pixel(strip, i, ...);
}
```

#### 4. Memory-Constrained Features
```
main/ (conditional compilation)
```

**Why**: Echo Base has 4 MB flash vs 16 MB on Korvo1.

**Example**:
```c
#if defined(CONFIG_BOARD_KORVO1)
    #define MAX_AUDIO_BUFFER_SIZE (256 * 1024)  // 256 KB
    #include "advanced_feature.h"                // Full features
#elif defined(CONFIG_BOARD_M5_ECHO_BASE)
    #define MAX_AUDIO_BUFFER_SIZE (32 * 1024)   // 32 KB (conservative)
    // Skip optional features
#endif
```

## Merging Strategy

### When Merging Korvo1 → Echo Base

```bash
cd Naphome-EchoBase
git fetch upstream

# Review commits
git log origin/main..upstream/main --stat

# Merge - this brings in Gemini, voice assistant, EQ, etc.
git merge upstream/main

# In case of conflicts:
# ✅ ACCEPT upstream changes for:
#   - components/gemini/*
#   - main/voice_assistant.*
#   - main/audio_eq.*
#   - main/wifi_manager.*
#   - main/Kconfig.projbuild

# ⚠️ RESOLVE MANUALLY for:
#   - main/app_main.c (may have Korvo1-specific LED animations)
#   - main/audio_player.c (I2S configuration)

# ❌ KEEP Echo Base version for:
#   - boards/m5_echo_base.h
#   - sdkconfig.defaults (Echo Base config)
#   - drivers/audio/m5_echo_base/*
```

### When Submitting Echo Base Improvements to Korvo1

Only cherry-pick commits that improve **shared** code:

```bash
cd Naphome-Korvo1

# DON'T merge entire branch
# git merge echobase/main  ← WRONG

# DO cherry-pick improvements
git cherry-pick echobase/<commit-for-gemini-fix>
git cherry-pick echobase/<commit-for-voice-assistant>
git cherry-pick echobase/<commit-for-equ-improvement>

# DON'T cherry-pick
# git cherry-pick echobase/<commit-for-echo-base-i2s-fix>  ← WRONG
```

## File Status Reference

| File | Status | Korvo1 | Echo Base | Notes |
|------|--------|--------|-----------|-------|
| `components/gemini/*` | SHARED ✅ | identical | identical | Same implementation |
| `main/voice_assistant.*` | SHARED ✅ | identical | identical | Business logic only |
| `main/audio_eq.*` | SHARED ✅ | identical | identical | DSP algorithm |
| `main/wifi_manager.*` | SHARED ✅ | identical | identical | WiFi driver |
| `main/audio_player.h` | SHARED ✅ | same interface | same interface | Public API identical |
| `main/audio_player.c` | DIFFERENT ⚠️ | Korvo1 I2S | Echo Base I2S | Implementation differs |
| `main/audio_abstraction.*` | SHARED ✅ | identical | identical | Board abstraction |
| `main/app_main.c` | MOSTLY ✅ | Korvo1 LED logic | Echo Base LED logic | LED animations differ |
| `boards/korvo1.h` | UNIQUE | Korvo1 pins | — | Don't sync |
| `boards/m5_echo_base.h` | UNIQUE | — | Echo Base pins | Don't sync |
| `drivers/audio/korvo1/*` | UNIQUE | Korvo1 driver | — | Don't sync |
| `drivers/audio/m5_echo_base/*` | UNIQUE | — | Echo Base driver | Don't sync |
| `main/CMakeLists.txt` | SHARED ✅ | identical | identical | Same build config |
| `main/Kconfig.projbuild` | SHARED ✅ | identical | identical | Board selection in same file |
| `sdkconfig.defaults` | DIFFERENT ⚠️ | Korvo1 config | Echo Base config | Different board configs |

## Sync Procedure Summary

### Sync Korvo1 → Echo Base (Typical)

```bash
cd Naphome-EchoBase

# Pull all the shared code improvements from Korvo1
git fetch upstream
git merge upstream/main

# Resolve conflicts:
# - Accept changes for: components/gemini, main/voice_assistant, etc.
# - Manually merge: main/app_main.c, main/audio_player.c
# - Keep Echo Base: boards/m5_echo_base.h, drivers/audio/m5_echo_base

# Test
idf.py build
idf.py flash

# Commit
git commit -m "Sync with Korvo1: Improved Gemini integration, etc."
git push origin main
```

### Sync Echo Base → Korvo1 (Selective)

```bash
cd Naphome-Korvo1

# Only cherry-pick improvements to shared code
git fetch echobase

# Verify it's a shared-code improvement
git show echobase/<commit-hash>

# Cherry-pick
git cherry-pick echobase/<commit-hash>

# Push
git push origin main
```

## Testing Checklist

After syncing, verify:

### Shared Code (Must Work on Both)
- [ ] Gemini API connectivity
- [ ] STT transcription
- [ ] LLM response handling
- [ ] TTS synthesis
- [ ] WiFi connection
- [ ] Audio EQ processing
- [ ] Wake word detection

### Board-Specific Code
- [ ] **Korvo1**: 12x LED animations
- [ ] **Echo Base**: 1x LED status indicator
- [ ] **Korvo1**: PDM microphone input
- [ ] **Echo Base**: Full-duplex I2S input (can't record+playback)
- [ ] Audio output on both boards

## Code Review Checklist

When reviewing PRs:

- [ ] Shared code changes? → Will be merged to both repos
- [ ] Board-specific changes? → Only affects intended board
- [ ] Uses hardcoded GPIO? → Should use `GPIO_I2S0_*` or `audio_abstraction_*()`
- [ ] #ifdef CONFIG_BOARD? → Only in board_config.h or audio_abstraction.c
- [ ] New Gemini logic? → Should work identically on both boards
- [ ] Memory optimization? → Check both 4MB and 16MB boards
- [ ] LED changes? → Tested on both 1x and 12x LED configurations

## Questions to Ask When Merging

1. **Is this improvement Gemini/voice assistant related?** → Keep it shared
2. **Is this a hardware pin configuration?** → Keep it board-specific
3. **Does this algorithm depend on hardware?** → Only if yes, board-specific
4. **Will developers using Echo Base need this fix?** → Sync it
5. **Is this only useful for Korvo1?** → Can remain in upstream only

Default to **shared** - only make board-specific if necessary!
