# EQ Implementation Summary

## Overview

Implemented a 3-stage biquad EQ system for the Naphome v0.1 speaker tuning as specified in `EQ.md`. The EQ processes audio in the float domain before conversion to PCM, ensuring high-quality filtering.

## Implementation Details

### Files Created

1. **`main/audio_eq.h`** - EQ API definitions
2. **`main/audio_eq.c`** - EQ implementation with biquad filters

### Filter Chain

The EQ implements three filters in series:

1. **High-Pass Filter @ 90 Hz** (Q=0.7, Butterworth)
   - Purpose: Protect small driver from LF overload
   - Type: 2nd-order high-pass

2. **Peaking EQ @ 320 Hz** (-4 dB, Q=1.0)
   - Purpose: Reduce mid-bass "boom" and 250-400 Hz hump

3. **Peaking EQ @ 500 Hz** (-2 dB, Q=1.0)
   - Purpose: Reduce boxiness and "cardboard" tone in 400-700 Hz range

4. **Global Gain** (-3 dB = 0.707 linear)
   - Purpose: Preserve headroom and prevent clipping

### Processing Flow

```
Input (32-bit float WAV)
  ↓
Copy from flash to RAM
  ↓
Process through EQ chain (per channel):
  - HPF 90 Hz
  - Peak 320 Hz
  - Peak 500 Hz
  - Global gain (-3 dB)
  ↓
Clamp to [-1.0, 1.0]
  ↓
Convert to 16-bit PCM
  ↓
Output to I2S
```

### Features

- **Stereo Support**: Separate EQ instances for left and right channels
- **Mono Support**: Uses left channel EQ for mono audio
- **Sample Rate Adaptive**: Coefficients calculated dynamically based on actual sample rate
- **Low CPU Usage**: 3 biquads × 2 channels = 6 filters total (very light for ESP32-S3)
- **State Management**: Filter state (delay elements) reset at start of playback

### Integration

The EQ is integrated into `audio_player.c`:

- Initialized in `audio_player_init()` with default sample rate
- Re-initialized with actual sample rate when playback starts
- State reset at the start of each playback
- Processes samples in the float conversion loop (before PCM conversion)

### Configuration

Currently, EQ is **enabled by default**. To disable:

```c
audio_eq_init(&s_audio.eq_left, sample_rate, false);
audio_eq_init(&s_audio.eq_right, sample_rate, false);
```

### Testing

1. Flash the firmware with EQ enabled
2. Play the embedded log sweep WAV file
3. Capture with REW using the measurement scripts
4. Compare:
   - Original response (EQ disabled)
   - EQ'd response (current implementation)
5. Adjust filter parameters in `audio_eq_init()` if needed

### Future Enhancements

- Add configuration option to enable/disable EQ via menuconfig
- Support for different EQ profiles (e.g., "bedside" vs "desk" modes)
- Add high-frequency filters once full-band measurements are available
- Export filter coefficients from REW for precise matching

### Notes

- Coefficients are calculated for the actual sample rate (currently 48 kHz)
- If sample rate changes, coefficients are recalculated automatically
- Filter coefficients use standard biquad formulas (compatible with REW exports)
