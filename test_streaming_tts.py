#!/usr/bin/env python3
"""
Test streaming TTS with callback pattern.
Demonstrates the same streaming base64 decoding approach used on device.
"""

import base64
import os
import sys
import io
import wave
from typing import Callable

# Add the test directory to path for importing test_gemini_api
sys.path.insert(0, os.path.dirname(__file__))

from test_gemini_api import call_gemini_tts_api


class StreamingBase64Decoder:
    """Python implementation of streaming base64 decoder (matches C version)"""

    def __init__(self):
        self.pending = bytearray()
        self.pending_len = 0
        self.started = False

    def decode_chunk(self, input_data: bytes) -> bytes:
        """
        Decode base64 data incrementally.
        Returns decoded bytes from complete 4-byte base64 groups.
        Buffers incomplete groups for next call.
        """
        if not input_data:
            return b""

        # Combine pending bytes with new input
        to_process = self.pending + input_data
        to_process_len = len(to_process)

        # Calculate complete 4-byte groups
        complete_groups = to_process_len // 4
        remainder = to_process_len % 4

        output = b""

        if complete_groups > 0:
            # Decode complete groups
            try:
                decoded = base64.b64decode(bytes(to_process[: complete_groups * 4]))
                output = decoded
            except Exception as e:
                print(f"Error decoding base64: {e}")
                return b""

        # Save remainder for next call
        if remainder > 0:
            self.pending = to_process[complete_groups * 4 :]
            self.pending_len = remainder
        else:
            self.pending = bytearray()
            self.pending_len = 0

        self.started = True
        return output

    def decode_finish(self) -> bytes:
        """
        Finalize decoding - decode any remaining pending bytes with padding.
        """
        if self.pending_len == 0:
            return b""

        # Pad with '=' to complete the base64 group
        padded = bytes(self.pending) + b"=" * (4 - self.pending_len)

        try:
            decoded = base64.b64decode(padded)
            self.pending_len = 0
            self.pending = bytearray()
            return decoded
        except Exception as e:
            print(f"Error decoding padded bytes: {e}")
            return b""


def bytes_to_pcm_samples(audio_bytes: bytes) -> list:
    """Convert raw PCM bytes to 16-bit samples"""
    samples = []
    for i in range(0, len(audio_bytes), 2):
        if i + 1 < len(audio_bytes):
            # Little-endian 16-bit signed integer
            sample = int.from_bytes(audio_bytes[i : i + 2], byteorder="little", signed=True)
            samples.append(sample)
    return samples


def test_streaming_tts_callback():
    """Test streaming TTS with callback pattern"""
    print("\n" + "=" * 70)
    print("Testing Streaming TTS with Callback Pattern")
    print("=" * 70)

    text = "Hello, this is a test of streaming text to speech synthesis."
    print(f"\nText: {text}")

    # Get base64 audio from API
    print("\nFetching TTS from Gemini API...")
    response_data = call_gemini_tts_api(text)

    if not response_data or "audioContent" not in response_data:
        print("ERROR: No audio content in response")
        return False

    base64_audio = response_data["audioContent"]
    print(f"Received {len(base64_audio)} bytes of base64-encoded audio")

    # Simulate streaming by processing in chunks
    chunk_size = 256  # Same as device implementation
    total_pcm_bytes = 0
    total_samples = 0
    chunks_processed = 0

    decoder = StreamingBase64Decoder()

    # Callback function to simulate audio player submission
    def playback_callback(samples: list) -> bool:
        """Called with decoded PCM samples as they arrive"""
        nonlocal total_samples, total_pcm_bytes, chunks_processed
        chunks_processed += 1
        sample_count = len(samples)
        total_samples += sample_count
        total_pcm_bytes += sample_count * 2  # 16-bit samples = 2 bytes each

        print(
            f"  Chunk {chunks_processed}: {sample_count} samples ({sample_count * 2} bytes) - "
            f"Total: {total_samples} samples ({total_pcm_bytes} bytes)"
        )
        return True

    # Process base64 audio in chunks
    print(f"\nStreaming audio in {chunk_size}-byte chunks:")
    print("-" * 70)

    pos = 0
    while pos < len(base64_audio):
        chunk = base64_audio[pos : pos + chunk_size]
        pos += chunk_size

        # Decode chunk incrementally
        decoded_bytes = decoder.decode_chunk(chunk.encode() if isinstance(chunk, str) else chunk)

        if decoded_bytes:
            samples = bytes_to_pcm_samples(decoded_bytes)
            if samples:
                playback_callback(samples)

    # Finalize any remaining pending bytes
    final_bytes = decoder.decode_finish()
    if final_bytes:
        samples = bytes_to_pcm_samples(final_bytes)
        if samples:
            print(f"  Final chunk: {len(samples)} samples ({len(final_bytes)} bytes)")
            total_samples += len(samples)
            total_pcm_bytes += len(final_bytes)

    print("-" * 70)
    print(f"\nResults:")
    print(f"  Total chunks processed: {chunks_processed}")
    print(f"  Total samples decoded: {total_samples}")
    print(f"  Total PCM bytes: {total_pcm_bytes}")
    print(f"  Audio duration at 24kHz: {total_samples / 24000:.2f} seconds")

    # Write audio to WAV file for verification
    wav_path = "/tmp/streaming_tts_output.wav"
    try:
        # Decode entire audio for WAV file
        full_decoded = base64.b64decode(base64_audio)
        with wave.open(wav_path, "wb") as wav_file:
            wav_file.setnchannels(1)  # Mono
            wav_file.setsampwidth(2)  # 16-bit
            wav_file.setframerate(24000)  # 24kHz
            wav_file.writeframes(full_decoded)
        print(f"\n✅ WAV file written to {wav_path}")
    except Exception as e:
        print(f"\n⚠️  Failed to write WAV file: {e}")

    return True


if __name__ == "__main__":
    try:
        success = test_streaming_tts_callback()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
