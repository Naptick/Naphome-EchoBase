# Board Synchronization Guide

This document explains how to keep Naphome-Korvo1 and Naphome-EchoBase repositories synchronized while managing board-specific differences.

## Repository Structure

### Naphome-Korvo1 (Main Repository)
- **URL**: https://github.com/Naptick/Naphome-Korvo1.git
- **Purpose**: Main development repository for Korvo1 board
- **Role**: Upstream source for EchoBase fork

### Naphome-EchoBase (Fork)
- **URL**: https://github.com/Naptick/Naphome-EchoBase.git
- **Purpose**: M5 Atom Echo Base specific implementation
- **Remote Setup**:
  - `origin` → https://github.com/Naptick/Naphome-EchoBase.git (push/pull here)
  - `upstream` → https://github.com/Naptick/Naphome-Korvo1.git (sync from here)

## Architecture: Board Abstraction Layer

### File Organization

```
main/
├── board_config.h              ← Selects board at compile time
├── audio_abstraction.h         ← Board-independent audio interface
├── audio_abstraction.c         ← Implementation using board configs
├── audio_player.h              ← Audio player interface
├── audio_player.c              ← Audio player implementation
└── app_main.c                  ← Application (uses audio_abstraction)

boards/
├── korvo1.h                    ← Korvo1 GPIO definitions
└── m5_echo_base.h              ← M5 Echo Base GPIO definitions
```

### Board Configuration System

**board_config.h** selects the appropriate board header at compile time:

```c
#if defined(CONFIG_BOARD_M5_ECHO_BASE)
    #include "../boards/m5_echo_base.h"
#elif defined(CONFIG_BOARD_KORVO1)
    #include "../boards/korvo1.h"
#endif
```

**Key Definitions** (board-specific):
- `GPIO_I2S0_*`: Speaker I2S pins
- `GPIO_I2S1_*`: Microphone I2S pins
- `GPIO_I2C_*`: Codec control pins
- `GPIO_RGB_LED`: LED pin
- Feature flags: `HAS_MICROPHONE`, `MICROPHONE_MODE`, etc.

### Audio Abstraction Layer

**audio_abstraction.h** provides board-independent functions:

```c
// Get board-specific GPIO pins
audio_abstraction_get_speaker_pins()
audio_abstraction_get_microphone_pins()

// Get board capabilities
audio_abstraction_supports_simultaneous_record_playback()
audio_abstraction_get_microphone_mode()

// Debugging
audio_abstraction_print_board_info()
audio_abstraction_print_i2s_pins()
```

**Usage in code**:
```c
// Instead of hardcoding GPIO_I2S0_MCLK, use:
gpio_num_t led_gpio = audio_abstraction_get_led_gpio();

// Check board capabilities before attempting operations:
if (!audio_abstraction_supports_simultaneous_record_playback()) {
    // Cannot record and playback simultaneously on M5 Echo Base
}
```

## Synchronization Workflow

### 1. Syncing EchoBase with Korvo1 (Pulling Upstream Changes)

When Korvo1 receives updates you want in EchoBase:

```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# Fetch latest from Korvo1 upstream
git fetch upstream

# View what changed
git log origin/main..upstream/main --oneline

# Merge upstream changes (no board-specific files)
git merge upstream/main

# Resolve conflicts if needed (only board_config.h, audio_abstraction.h, etc.)
git add .
git commit -m "Sync with upstream Korvo1 changes"

# Push to EchoBase repository
git push origin main
```

### 2. Syncing Korvo1 with EchoBase (Pushing Enhancements)

If you make general improvements in EchoBase that should go to Korvo1:

```bash
# In Naphome-EchoBase
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# Create PR to upstream
gh pr create --title "Description of changes" \
  --body "Upstream PR from EchoBase improvements"
```

Or manually:

```bash
# In Naphome-Korvo1
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# Add EchoBase as remote
git remote add echobase https://github.com/Naptick/Naphome-EchoBase.git

# Fetch and review changes
git fetch echobase
git log main..echobase/main --oneline

# Cherry-pick specific commits
git cherry-pick <commit-hash>

# Or merge entire branch
git merge echobase/main
```

## Handling Board-Specific Differences

### GPIO/Pin Differences

**Already Handled** ✓
- All GPIO pins defined in `boards/korvo1.h` and `boards/m5_echo_base.h`
- Board selection at compile time via `CONFIG_BOARD_*`
- Audio player uses board-specific pins from `board_config.h`

**Example: I2S MCLK Pin**

Korvo1:
```c
// boards/korvo1.h
#define GPIO_I2S0_MCLK (42)
```

M5 Echo Base:
```c
// boards/m5_echo_base.h
#define GPIO_I2S0_MCLK (0)
```

Usage in code:
```c
// boards/board_config.h selects the right header
// So GPIO_I2S0_MCLK automatically gets correct value for chosen board
i2s_cfg.mclk_io_num = GPIO_I2S0_MCLK;
```

### Microphone Differences

**Korvo1** (PDM Microphone):
```c
// Independent I2S1 port, separate GPIO pins
#define GPIO_I2S1_DIN  (35)  // PDM data
#define GPIO_I2S1_BCLK (36)  // PDM clock
#define GPIO_I2S1_WS   (37)  // Word select
```

**M5 Echo Base** (Full-Duplex Microphone):
```c
// Shares pins with speaker I2S0
#define GPIO_I2S1_DIN  (23)  // Shared with I2S0_BCLK
#define GPIO_I2S1_BCLK (33)  // Shared with I2S0_LRCLK
#define GPIO_I2S1_WS   (33)  // Shared with I2S0_LRCLK
```

**Board-Aware Code**:
```c
// In audio_abstraction.c
int audio_abstraction_supports_simultaneous_record_playback(void)
{
#if defined(CONFIG_BOARD_KORVO1)
    return 1;  // Independent I2S ports
#elif defined(CONFIG_BOARD_M5_ECHO_BASE)
    return 0;  // Shared I2S pins
#endif
}

// Usage in app code
if (!audio_abstraction_supports_simultaneous_record_playback()) {
    // Stop playback before recording, or vice versa
    audio_player_stop();
}
```

### LED Differences

**Korvo1**:
```c
#define GPIO_RGB_LED  (19)
#define RGB_LED_COUNT (12)  // 12x WS2812 ring
```

**M5 Echo Base**:
```c
#define GPIO_RGB_LED  (46)
#define RGB_LED_COUNT (1)   // 1x SK6812 LED
```

**Usage**:
```c
// Automatically detects board and uses correct GPIO/count
led_strip_handle_t strip;
led_strip_new_rmt_device(
    .max_leds = RGB_LED_COUNT,  // 12 or 1
    .strip_gpio_num = GPIO_RGB_LED,  // 19 or 46
    ...
);
```

## Conflict Resolution Strategy

### Merging from Upstream (Korvo1 → EchoBase)

**Low Priority Conflicts** (Accept Remote):
- Feature-specific code for Korvo1-only features (e.g., PDM microphone control)
- Korvo1-specific optimizations
- Documentation mentioning Korvo1

**Manual Merge Conflicts**:
```bash
# For audio_player.c conflicts with Korvo1 changes
# Strategy: Keep EchoBase version if it's just GPIO pins
git checkout --ours main/audio_player.c

# For board-specific headers
# Strategy: Keep both versions
git add boards/korvo1.h boards/m5_echo_base.h

# Then commit
git add .
git commit -m "Merge upstream Korvo1: resolve board-specific conflicts"
```

### Merging to Upstream (EchoBase → Korvo1)

**Cherry-pick General Improvements**:
```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

git fetch echobase
git cherry-pick echobase/<commit-hash>  # Only general improvements
git push origin main
```

**Don't merge EchoBase-specific code**:
- Don't push M5-specific GPIO configurations to Korvo1
- Don't push I2C fixed-pin workarounds to Korvo1
- Keep board differences in separate headers

## Adding a New Board

To add a third board (e.g., Naphome-Esp32Dev):

1. **Create board header** `boards/esp32dev.h`:
   ```c
   #define GPIO_I2S0_MCLK (10)
   #define GPIO_I2S0_BCLK (11)
   // ... all GPIO definitions
   ```

2. **Update board_config.h**:
   ```c
   #elif defined(CONFIG_BOARD_ESP32_DEV)
       #include "../boards/esp32dev.h"
       #define BOARD_NAME "ESP32 Dev Board"
   ```

3. **Add Kconfig option** in `main/Kconfig.projbuild`:
   ```
   config BOARD_ESP32_DEV
       bool "ESP32 Dev Board"
   ```

4. **Audio abstraction automatically works** with new board!

## Workflow Checklist

### When Syncing Korvo1 → EchoBase:

- [ ] Fetch upstream: `git fetch upstream`
- [ ] Review changes: `git log origin/main..upstream/main`
- [ ] Merge: `git merge upstream/main`
- [ ] Resolve conflicts if any
- [ ] Build test: `idf.py build`
- [ ] Commit: `git commit`
- [ ] Push: `git push origin main`

### When Merging EchoBase → Korvo1:

- [ ] Review changes in EchoBase
- [ ] Identify general vs. board-specific changes
- [ ] Cherry-pick general improvements to Korvo1
- [ ] Create PR to upstream
- [ ] Test in Korvo1

### Before Committing Changes:

- [ ] ✓ Use `audio_abstraction_*()` instead of `GPIO_*` constants directly
- [ ] ✓ Update both board headers if adding new pin definition
- [ ] ✓ Test on target board (Korvo1 or EchoBase)
- [ ] ✓ Check for hardcoded GPIO values (should all be in board headers)

## Debugging Board Configuration

Use the debugging functions to verify configuration:

```c
#include "audio_abstraction.h"

void app_main(void)
{
    audio_abstraction_print_board_info();
    audio_abstraction_print_i2s_pins();
}
```

Output for Korvo1:
```
I (123) audio_abstraction: === BOARD CONFIGURATION ===
I (123) audio_abstraction: Board: Korvo1
I (123) audio_abstraction: Flash: 16 MB
I (123) audio_abstraction: PSRAM: 8 MB
I (123) audio_abstraction: LED GPIO: 19 (count: 12)
I (123) audio_abstraction: Microphone: Yes (PDM)
I (123) audio_abstraction: Simultaneous Record+Playback: Yes
I (123) audio_abstraction: === I2S SPEAKER PINS (I2S0) ===
...
```

Output for M5 Echo Base:
```
I (123) audio_abstraction: === BOARD CONFIGURATION ===
I (123) audio_abstraction: Board: M5 Atom Echo Base
I (123) audio_abstraction: Flash: 4 MB
I (123) audio_abstraction: PSRAM: 0 MB
I (123) audio_abstraction: LED GPIO: 46 (count: 1)
I (123) audio_abstraction: Microphone: Yes (I2S Full-Duplex)
I (123) audio_abstraction: Simultaneous Record+Playback: No
I (123) audio_abstraction: === I2S SPEAKER PINS (I2S0) ===
...
```

## Common Pitfalls

❌ **DON'T**: Hardcode GPIO numbers in application code
```c
// BAD
i2s_pin_config_t pin_config = {
    .mclk_io_num = 42,  // Only works on Korvo1!
    .bclk_io_num = 40,
};
```

✅ **DO**: Use board abstractions
```c
// GOOD
gpio_num_t mclk, bclk, lrclk, dout;
audio_abstraction_get_speaker_pins(&mclk, &bclk, &lrclk, &dout);
```

❌ **DON'T**: Merge EchoBase-specific code to Korvo1 upstream
```bash
# BAD: This pollutes Korvo1 with Echo Base-specific logic
git merge echobase/main
```

✅ **DO**: Cherry-pick only general improvements
```bash
# GOOD: Only take the improvements, not board-specific code
git cherry-pick echobase/<commit-hash>
```

## Repository Health Checks

### Check for hardcoded GPIO values:
```bash
grep -r "GPIO_NUM_\|GPIO\s*(" main/*.c --exclude="*abstraction*" | grep -v "GPIO_NUM_NC"
```

### Verify board headers consistency:
```bash
# Both should have same GPIO definitions (with different values)
diff <(grep "define GPIO_" boards/korvo1.h) <(grep "define GPIO_" boards/m5_echo_base.h)
```

### Check for #ifdef CONFIG_BOARD misuse:
```bash
# Should only be in board_config.h and audio_abstraction.c
grep -r "CONFIG_BOARD_" main/ boards/ | grep -v "board_config.h" | grep -v "audio_abstraction"
```
