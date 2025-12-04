# Deployment Guide: Korvo1 Development → Echo Base Distribution

This guide explains the development and deployment workflow where:
- **Development**: Primarily on Naphome-Korvo1 (more powerful hardware, more features)
- **Deployment**: Distributed on Naphome-EchoBase for developers (easier to distribute, more affordable)

## Workflow Overview

```
┌─────────────────────────────────────────────────────────────┐
│        Development on Korvo1 (Main Repository)              │
│  - More powerful hardware (ESP32-S3, 16MB flash, 8MB PSRAM) │
│  - All features available                                    │
│  - Easier debugging with PDM microphone                      │
│  - Primary development target                                │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ Merge upstream changes
                     │ when ready for release
                     ↓
┌─────────────────────────────────────────────────────────────┐
│    Sync to Echo Base (Fork Repository)                       │
│  - Pull changes from Korvo1                                  │
│  - Resolve board-specific differences                        │
│  - Verify on Echo Base hardware                              │
│  - Tag release version                                       │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ Deploy to developers
                     │ (GitHub release, documentation)
                     ↓
┌─────────────────────────────────────────────────────────────┐
│  Developers use Echo Base (Easier to distribute)             │
│  - More affordable hardware                                  │
│  - Pre-configured for common use cases                       │
│  - Updates via pull from Echo Base repo                      │
└─────────────────────────────────────────────────────────────┘
```

## Repository Roles

### Naphome-Korvo1 (Upstream)
- **Primary Development**: All feature development happens here
- **Target Audience**: Internal developers with Korvo1 hardware
- **Maintenance**: High-frequency updates, bleeding edge
- **Branch Strategy**:
  - `main`: Stable development version
  - `feature/*`: Feature branches for new work
  - `release/*`: Release branches for stabilization

### Naphome-EchoBase (Fork)
- **Distribution Target**: External developers with Echo Base
- **Target Audience**: Developers who want pre-configured firmware
- **Maintenance**: Stable, synced from Korvo1 at release points
- **Branch Strategy**:
  - `main`: Stable, synced with Korvo1 releases
  - `upstream`: Tracks Korvo1 main for reference

## Development Process

### Phase 1: Active Development on Korvo1

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# Create feature branch
git checkout -b feature/new-feature
git push origin feature/new-feature

# Work on feature...
# Commit frequently
git commit -m "Work on new feature"

# Create PR when ready
gh pr create --title "Feature: description" --body "Details"

# Code review and testing
# - Test on Korvo1 hardware
# - Verify audio I2S works correctly
# - Check memory usage
# - Test with 12x LED ring
```

### Phase 2: Testing & Stabilization

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# When ready for release, create release branch
git checkout -b release/v1.0.0 main
git push origin release/v1.0.0

# Only critical fixes on release branch
git commit -m "Fix critical issue in release"

# Merge back to main
git checkout main
git merge release/v1.0.0
git push origin main

# Tag the release
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

### Phase 3: Sync to Echo Base for Distribution

```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# Fetch latest from Korvo1
git fetch upstream

# View changes
git log origin/main..upstream/main --oneline

# Merge and resolve conflicts
git merge upstream/main

# Handle conflicts:
# - Accept audio abstraction changes from upstream
# - Keep board-specific configurations
# - Test build: idf.py build

# Commit merge
git commit -m "Sync with Korvo1 v1.0.0 release"

# Create release tag in Echo Base
git tag -a v1.0.0-echo-base -m "Echo Base v1.0.0"

# Push to Echo Base repository
git push origin main
git push origin v1.0.0-echo-base

# Create GitHub release with binaries
gh release create v1.0.0-echo-base \
  --title "Echo Base v1.0.0" \
  --notes "See deployment guide for flashing"
```

## Build Configuration for Each Board

### Building for Korvo1

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# Set Korvo1 as target
idf.py set-target esp32s3

# Configure for Korvo1
idf.py menuconfig
# Select: Board Selection → Korvo1 Development Board

# Build firmware
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

### Building for M5 Echo Base

```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# Set Echo Base target (ESP32-PICO-D4 is similar to ESP32)
idf.py set-target esp32

# Configure for Echo Base
idf.py menuconfig
# Select: Board Selection → M5 Atom S3R + Atom Echo Base

# Build firmware
idf.py build

# Flash
idf.py flash

# Monitor (Echo Base uses USB CDC)
idf.py monitor
```

## Key Differences for Developers Using Echo Base

### Hardware Limitations

| Feature | Korvo1 | Echo Base | Handling |
|---------|--------|-----------|----------|
| Flash | 16 MB | 4 MB | Reduced feature set for Echo Base |
| PSRAM | 8 MB | 0 MB | Memory-constrained apps |
| Microphone | PDM (I2S1) | Full-duplex I2S | Cannot record+playback simultaneously |
| LED | 12x WS2812 | 1x SK6812 | Auto-detected and handled |
| Record+Playback | Independent ports | Shared I2S | Feature flag detection |

### Configuration Flags

Use these to handle Echo Base limitations:

```c
#include "board_config.h"

// Check board capabilities
if (!audio_abstraction_supports_simultaneous_record_playback()) {
    // Echo Base: Must stop playback before recording
    audio_player_stop();
}

if (audio_abstraction_get_psram_size_mb() == 0) {
    // Echo Base: Use more conservative memory settings
    CONFIG_MAX_AUDIO_BUFFER = 32 * 1024;  // 32 KB instead of 256 KB
}

// Check LED count
int led_count = audio_abstraction_get_led_count();
if (led_count == 1) {
    // Echo Base: Single LED, use binary state (on/off)
    led_strip_set_pixel(strip, 0, 255, 0, 0);  // Red
} else {
    // Korvo1: 12x LED ring, can use animations
    for (int i = 0; i < led_count; i++) {
        led_strip_set_pixel(strip, i, i*20, 0, 0);  // Gradient
    }
}
```

## Synchronization Workflow

### Automatic Sync Script

```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# Sync with upstream Korvo1 and push
./sync-with-upstream.sh --push

# This script:
# 1. Fetches upstream/main (Korvo1 changes)
# 2. Shows what will merge
# 3. Merges changes
# 4. Verifies build (optional)
# 5. Pushes to origin/main
```

### Manual Sync Process

```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase

# 1. Fetch upstream changes
git fetch upstream main

# 2. Review changes
git log origin/main..upstream/main --stat

# 3. Merge with conflict resolution
git merge upstream/main

# 4. Resolve conflicts if needed
# Typically: accept Korvo1 code, keep board headers

# 5. Build verification
idf.py build

# 6. Test on Echo Base hardware
idf.py flash
idf.py monitor
# Verify audio works, LEDs respond, etc.

# 7. Commit and push
git add .
git commit -m "Sync with Korvo1 improvements"
git push origin main
```

## Version Management

### Semantic Versioning

Use semantic versioning for releases:
- **MAJOR**: Breaking changes to API or hardware support
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes

### Release Checklist

Before syncing to Echo Base:

- [ ] All tests pass on Korvo1
- [ ] Code review completed
- [ ] Documentation updated
- [ ] Audio functionality verified (both speaker and microphone)
- [ ] LED animations tested
- [ ] WiFi/Gemini API tested (if included)
- [ ] Memory usage checked (should fit in 4 MB for Echo Base)
- [ ] Build verified for both boards

### Creating Releases

```bash
# On Korvo1
cd /Users/danielmcshan/GitHub/Naphome-Korvo1
git tag -a v1.0.0 -m "Release 1.0.0: Added feature X"
git push origin v1.0.0
gh release create v1.0.0 --title "v1.0.0" --notes "Release notes here"

# Then sync to Echo Base
cd /Users/danielmcshan/GitHub/Naphome-EchoBase
git fetch upstream
git merge upstream/main
git tag -a v1.0.0-echo-base -m "Echo Base v1.0.0"
git push origin v1.0.0-echo-base
gh release create v1.0.0-echo-base --title "Echo Base v1.0.0"
```

## Handling Echo Base-Specific Issues

### Issue: "Image too large for flash"

Echo Base has 4 MB flash (vs 16 MB on Korvo1). If you get this error:

```
Error: Image too large for flash
```

Solutions:
1. Disable non-essential features in menuconfig
2. Reduce audio buffer sizes
3. Use compressed assets
4. Consider splitting into two builds:
   - `echo-base-minimal`: Voice assistant only
   - `echo-base-full`: With wake word detection

### Issue: "Microphone input not working"

Echo Base uses full-duplex I2S (shared with speaker). If recording fails:

```c
// Check if simultaneous operation is attempted
if (audio_abstraction_supports_simultaneous_record_playback() == 0) {
    // Must stop speaker before recording
    audio_player_stop();
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for I2S to settle
    mic_start_recording();
}
```

### Issue: "I2C communication fails"

Echo Base has fixed I2C pins that cannot be changed. If I2C fails:

1. Verify GPIO 19 (SDA) and GPIO 33 (SCL) are not used elsewhere
2. Check pull-up resistors on I2C bus
3. Ensure ES8311 codec is responding:
   ```c
   audio_abstraction_print_i2s_pins();  // Verify pins
   ```

## Deployment Checklist

### Pre-Deployment (on Korvo1)

- [ ] Feature complete and tested
- [ ] All tests passing
- [ ] Code review completed
- [ ] Performance verified
- [ ] Memory usage acceptable for 4 MB flash
- [ ] Audio I2S configuration verified with `audio_abstraction_print_i2s_pins()`
- [ ] Documentation updated

### Syncing to Echo Base

- [ ] Run `./sync-with-upstream.sh --push`
- [ ] Verify build completes: `idf.py build`
- [ ] Test on Echo Base hardware
- [ ] Verify audio output (speaker)
- [ ] Verify audio input (microphone)
- [ ] Check LED functionality
- [ ] Test WiFi connectivity (if applicable)

### Distribution

- [ ] Create GitHub release with binaries
- [ ] Update README with download link
- [ ] Document build instructions for Echo Base
- [ ] Provide flashing guide for developers
- [ ] Monitor for issues reported by developers

## Summary

This deployment strategy allows you to:
1. **Develop primarily on Korvo1** - More powerful, full feature set
2. **Sync to Echo Base** - Make available for developers
3. **Maintain compatibility** - Board abstraction layer handles differences
4. **Easy distribution** - Echo Base is more accessible and affordable

The key is the board abstraction layer that makes the code portable between boards while preserving board-specific capabilities.
