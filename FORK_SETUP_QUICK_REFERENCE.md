# Fork Setup Quick Reference

Quick reference for working with the Naphome-Korvo1 and Naphome-EchoBase fork structure.

## One-Time Setup

### EchoBase Repository Setup

```bash
# Clone EchoBase (already have?)
cd ~/GitHub/Naphome-EchoBase

# Add upstream (already done)
git remote add upstream https://github.com/Naptick/Naphome-Korvo1.git

# Verify
git remote -v
# origin     https://github.com/Naptick/Naphome-EchoBase.git (fetch)
# origin     https://github.com/Naptick/Naphome-EchoBase.git (push)
# upstream   https://github.com/Naptick/Naphome-Korvo1.git (fetch)
# upstream   https://github.com/Naptick/Naphome-Korvo1.git (push)
```

## Daily Workflows

### I. Developing on Korvo1 (Main Development)

```bash
cd ~/GitHub/Naphome-Korvo1

# Create feature branch
git checkout -b feature/my-feature
# ... make changes ...
git commit -m "Feature: description"
git push origin feature/my-feature

# Create PR on GitHub
gh pr create --title "Feature: description" --body "Details"

# Configure board (if not set)
idf.py menuconfig
# Select: Board Selection → Korvo1 Development Board

# Build and test
idf.py build
idf.py flash
idf.py monitor
```

### II. Syncing EchoBase with Korvo1 Updates

**Option A: Using the automated script** (Recommended)
```bash
cd ~/GitHub/Naphome-EchoBase
./sync-with-upstream.sh --push
# Automates: fetch, merge, build verify, push
```

**Option B: Manual sync**
```bash
cd ~/GitHub/Naphome-EchoBase

# Fetch and merge
git fetch upstream main
git merge upstream/main

# Resolve conflicts
git status  # See conflicts
# Edit files...
git add .
git commit -m "Merge upstream changes"

# Push
git push origin main
```

### III. Testing on Echo Base After Sync

```bash
cd ~/GitHub/Naphome-EchoBase

# Configure for Echo Base
idf.py menuconfig
# Select: Board Selection → M5 Atom S3R + Atom Echo Base

# Build
idf.py build

# Flash
idf.py flash

# Test audio I2S pins
idf.py monitor
# Look for: audio_abstraction_print_i2s_pins() output
```

### IV. Creating a Release

```bash
# On Korvo1
cd ~/GitHub/Naphome-Korvo1
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
gh release create v1.0.0

# Then sync to EchoBase
cd ~/GitHub/Naphome-EchoBase
./sync-with-upstream.sh --push
git tag -a v1.0.0-echo-base -m "Echo Base v1.0.0"
git push origin v1.0.0-echo-base
gh release create v1.0.0-echo-base --title "Echo Base v1.0.0"
```

## File Organization

```
Naphome-Korvo1 (Upstream - Main Development)
├── components/gemini/           ← SHARED (Gemini API)
├── main/
│   ├── voice_assistant.*        ← SHARED (identical)
│   ├── audio_eq.*               ← SHARED (identical)
│   ├── wifi_manager.*           ← SHARED (identical)
│   ├── audio_player.*           ← Different implementation
│   ├── board_config.h           ← SHARED
│   └── boards/
│       ├── korvo1.h             ← Korvo1 pins
│       └── m5_echo_base.h       ← Echo Base pins (in Korvo1 too)
└── sdkconfig.defaults           ← Korvo1 config

Naphome-EchoBase (Fork - Distribution)
├── components/gemini/           ← SHARED (identical)
├── main/
│   ├── voice_assistant.*        ← SHARED (identical)
│   ├── audio_eq.*               ← SHARED (identical)
│   ├── wifi_manager.*           ← SHARED (identical)
│   ├── audio_player.*           ← Different implementation
│   ├── board_config.h           ← SHARED
│   ├── audio_abstraction.*      ← Board-independent interface
│   └── boards/
│       ├── korvo1.h             ← Korvo1 pins (ref only)
│       └── m5_echo_base.h       ← Echo Base pins (primary)
├── sync-with-upstream.sh        ← Sync helper script
├── sdkconfig.defaults           ← Echo Base config
├── BOARD_SYNC_GUIDE.md          ← Detailed sync procedures
├── DEPLOYMENT_GUIDE.md          ← Development workflow
├── SHARED_CODE_POLICY.md        ← What to sync vs not
└── FORK_SETUP_QUICK_REFERENCE.md ← This file
```

## Key Principles

### ✅ Keep SHARED (Always Sync)
- Gemini API code
- Voice assistant logic
- WiFi manager
- Audio EQ/DSP
- Board abstraction layer
- Audio player interface

### ⚠️ RESOLVE MANUALLY
- `app_main.c` - LED animations differ
- `audio_player.c` - I2S implementation differs
- `CMakeLists.txt` - Usually same, check for conflicts

### ❌ Keep DIFFERENT (Never Sync)
- `boards/korvo1.h` - Korvo1 pins only
- `boards/m5_echo_base.h` - Echo Base pins only
- `sdkconfig.defaults` - Board-specific config
- `drivers/audio/korvo1/*` - Korvo1 driver
- `drivers/audio/m5_echo_base/*` - Echo Base driver

## Conflict Resolution Quick Guide

| File | Conflict Type | Resolution |
|------|---------------|-----------|
| `components/gemini/*` | Common | Accept upstream ✅ |
| `main/voice_assistant.*` | Common | Accept upstream ✅ |
| `main/audio_player.c` | Uncommon | Manual merge ⚠️ |
| `main/app_main.c` | Common (LED) | Manual merge ⚠️ |
| `boards/korvo1.h` | Rare | Keep Korvo1 ❌ |
| `boards/m5_echo_base.h` | Never | Keep Echo Base ❌ |
| `sdkconfig.defaults` | Always | Keep Echo Base ❌ |

### Manual Merge for `audio_player.c`

```bash
# If conflicts in audio_player.c:

# Option 1: Accept upstream (often best)
git checkout --theirs main/audio_player.c

# Option 2: Manual merge (if upstream has audio I2S improvements)
# Edit file, resolve differences
# Ensure both Korvo1 and Echo Base I2S configs work
git add main/audio_player.c
```

## Useful Commands

### Check what changed in Korvo1
```bash
cd ~/GitHub/Naphome-EchoBase
git fetch upstream
git log origin/main..upstream/main --oneline
git diff origin/main..upstream/main --stat
```

### See sync status
```bash
git status
# If nothing: up to date with upstream
# If changes: ready to push
```

### Cherry-pick improvements back to Korvo1
```bash
cd ~/GitHub/Naphome-Korvo1
git fetch echobase
git cherry-pick echobase/<commit-hash>
git push origin main
```

### Build for a specific board
```bash
# For Korvo1
cd ~/GitHub/Naphome-Korvo1
idf.py set-target esp32s3
idf.py menuconfig  # Select Korvo1
idf.py build

# For Echo Base
cd ~/GitHub/Naphome-EchoBase
idf.py set-target esp32
idf.py menuconfig  # Select Echo Base
idf.py build
```

### Test audio configuration
```bash
# In app_main.c or any test code:
#include "audio_abstraction.h"

void test_board_config(void)
{
    audio_abstraction_print_board_info();
    audio_abstraction_print_i2s_pins();
}
```

## Troubleshooting

### "Image too large for flash" on Echo Base
- Echo Base has 4 MB flash (Korvo1 has 16 MB)
- Disable optional features in `menuconfig`
- Reduce buffer sizes for Echo Base variant

### "Merge conflicts everywhere"
- You probably merged something that shouldn't be shared
- Check SHARED_CODE_POLICY.md
- Use `./sync-with-upstream.sh` to automate proper merge

### "Audio not working after sync"
- Verify GPIO pins with: `audio_abstraction_print_i2s_pins()`
- Check board selection in menuconfig
- Verify I2S configuration matches board-specific pins

### "LED animations weird"
- Check `RGB_LED_COUNT` (12 on Korvo1, 1 on Echo Base)
- Use `audio_abstraction_get_led_count()` instead of hardcoding
- Verify LED GPIO with `audio_abstraction_print_board_info()`

## Documentation Reference

- **BOARD_SYNC_GUIDE.md** - Detailed sync procedures and architecture
- **DEPLOYMENT_GUIDE.md** - Development workflow and deployment strategy
- **SHARED_CODE_POLICY.md** - Which files to sync and which to keep separate
- **FORK_SETUP_QUICK_REFERENCE.md** - This quick reference

## Architecture Files

- **boards/korvo1.h** - Korvo1 GPIO definitions
- **boards/m5_echo_base.h** - Echo Base GPIO definitions
- **main/board_config.h** - Board selector (CONFIG_BOARD_* at compile time)
- **main/audio_abstraction.h** - Board-independent audio interface
- **main/audio_abstraction.c** - Audio abstraction implementation

## Key Files for Development

### Korvo1 Only
- `drivers/audio/korvo1/*` - Korvo1-specific audio driver

### Echo Base Only
- `drivers/audio/m5_echo_base/*` - Echo Base-specific audio driver

### Shared (Must be Identical)
- `components/gemini/*` - Gemini API integration
- `main/voice_assistant.*` - Voice assistant logic
- `main/wifi_manager.*` - WiFi management
- `main/audio_eq.*` - Audio EQ/DSP
- `main/audio_abstraction.*` - Board abstraction layer

## Summary

1. **Develop on Korvo1** - It's the main development target
2. **Sync to Echo Base** - When ready for distribution
3. **Keep shared code identical** - Especially Gemini, voice assistant, EQ
4. **Use abstraction layer** - Don't hardcode GPIO numbers
5. **Test on both boards** - Before releasing

For detailed information, see the documentation files in this repository.
