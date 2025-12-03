#!/usr/bin/env python3
"""
Automated audio measurement recording script
Records audio from the default input device while ESP32 plays the log sweep
"""

import argparse
import sys
import time
import numpy as np

try:
    import sounddevice as sd
except ImportError:
    print("Error: sounddevice not installed")
    print("Install with: pip install sounddevice")
    sys.exit(1)

try:
    import soundfile as sf
except ImportError:
    print("Error: soundfile not installed")
    print("Install with: pip install soundfile")
    sys.exit(1)


def list_devices():
    """List available audio devices"""
    print("\nAvailable audio devices:")
    print(sd.query_devices())
    print()


def record_measurement(duration, sample_rate=48000, channels=2, device=None, output_file=None):
    """Record audio measurement"""
    print(f"Recording {duration} seconds at {sample_rate} Hz, {channels} channels...")
    if device is not None:
        print(f"Using device: {device}")
    print("Press Enter when ready to start recording, then trigger playback on your ESP32 device...")
    input()
    
    print("Recording...")
    print("(Trigger playback on your ESP32 device now!)")
    
    # Record audio
    frames = int(duration * sample_rate)
    recording = sd.rec(frames=frames, samplerate=sample_rate, channels=channels, device=device)
    sd.wait()  # Wait until recording is finished
    
    print("Recording complete!")
    
    # Save to file
    if output_file:
        print(f"Saving to: {output_file}")
        sf.write(output_file, recording, sample_rate)
        print(f"✅ Saved: {output_file}")
    else:
        print("No output file specified (use --output)")
    
    return recording


def main():
    parser = argparse.ArgumentParser(description="Record audio measurement from ESP32 log sweep")
    parser.add_argument("--duration", type=float, default=5.0, help="Recording duration in seconds")
    parser.add_argument("--sample-rate", type=int, default=48000, help="Sample rate (Hz)")
    parser.add_argument("--channels", type=int, default=2, help="Number of channels")
    parser.add_argument("--device", type=int, default=None, help="Audio device ID (use --list-devices to see options)")
    parser.add_argument("--output", type=str, default=None, help="Output WAV file path")
    parser.add_argument("--list-devices", action="store_true", help="List available audio devices")
    
    args = parser.parse_args()
    
    if args.list_devices:
        list_devices()
        return
    
    if not args.output:
        import datetime
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        args.output = f"measurement_{timestamp}.wav"
    
    try:
        recording = record_measurement(
            duration=args.duration,
            sample_rate=args.sample_rate,
            channels=args.channels,
            device=args.device,
            output_file=args.output
        )
        
        # Print some stats
        max_amplitude = np.max(np.abs(recording))
        print(f"\nRecording stats:")
        print(f"  Max amplitude: {max_amplitude:.4f}")
        print(f"  Duration: {len(recording) / args.sample_rate:.2f} seconds")
        print(f"  Sample rate: {args.sample_rate} Hz")
        print(f"  Channels: {args.channels}")
        
    except KeyboardInterrupt:
        print("\nRecording interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
