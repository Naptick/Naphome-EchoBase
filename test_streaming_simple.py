#!/usr/bin/env python3
"""
Simple streaming TTS test - no dependencies on test_gemini_api
"""

import base64
import json
import requests
import os

# API key from test_gemini_api.py
API_KEY = "AIzaSyCjrdIBkpGWGXa4u9UileFFIMBZ_ZnMZ1w"

class StreamingBase64Decoder:
    """Streaming base64 decoder - matches C version"""

    def __init__(self):
        self.pending = bytearray()
        self.started = False

    def decode_chunk(self, input_data: bytes) -> bytes:
        """Decode base64 incremental, buffering incomplete groups"""
        if not input_data:
            return b""

        to_process = self.pending + input_data
        complete_groups = len(to_process) // 4
        remainder = len(to_process) % 4

        output = b""

        if complete_groups > 0:
            try:
                decoded = base64.b64decode(bytes(to_process[: complete_groups * 4]))
                output = decoded
            except Exception as e:
                print(f"Decode error: {e}")
                return b""

        if remainder > 0:
            self.pending = to_process[complete_groups * 4 :]
        else:
            self.pending = bytearray()

        self.started = True
        return output

    def decode_finish(self) -> bytes:
        """Finalize with padding"""
        if len(self.pending) == 0:
            return b""

        padded = bytes(self.pending) + b"=" * (4 - len(self.pending))
        try:
            return base64.b64decode(padded)
        except Exception as e:
            print(f"Finish decode error: {e}")
            return b""


def test_streaming_tts():
    """Test streaming TTS"""
    print("\n" + "=" * 70)
    print("Testing Streaming TTS with Callback")
    print("=" * 70)

    text = "Just say Hey, Nap to talk to me."
    print(f"\nText: {text}")

    url = f"https://texttospeech.googleapis.com/v1/text:synthesize?key={API_KEY}"
    payload = {
        "input": {"text": text},
        "voice": {
            "languageCode": "en-US",
            "name": "en-US-Neural2-D"
        },
        "audioConfig": {
            "audioEncoding": "LINEAR16",
            "sampleRateHertz": 24000
        }
    }

    print("\nFetching TTS from API...")
    response = requests.post(url, json=payload, timeout=30)
    print(f"Status: {response.status_code}")

    if response.status_code != 200:
        print(f"ERROR: {response.status_code}")
        print(response.text)
        return False

    data = response.json()
    if "audioContent" not in data:
        print("ERROR: No audioContent in response")
        print(f"Keys: {list(data.keys())}")
        return False

    base64_audio = data["audioContent"]
    print(f"Received {len(base64_audio)} bytes of base64")

    # Stream in chunks
    chunk_size = 256
    total_samples = 0
    chunks = 0
    decoder = StreamingBase64Decoder()

    print(f"\nStreaming in {chunk_size}-byte chunks:")
    print("-" * 70)

    pos = 0
    while pos < len(base64_audio):
        chunk = base64_audio[pos : pos + chunk_size]
        pos += chunk_size

        decoded = decoder.decode_chunk(chunk.encode() if isinstance(chunk, str) else chunk)
        if decoded:
            samples = len(decoded) // 2  # 16-bit samples
            total_samples += samples
            chunks += 1
            print(f"  Chunk {chunks}: {samples} samples ({len(decoded)} bytes)")

    # Final chunk
    final = decoder.decode_finish()
    if final:
        samples = len(final) // 2
        total_samples += samples
        print(f"  Final: {samples} samples ({len(final)} bytes)")

    print("-" * 70)
    print(f"\nTotal chunks: {chunks}")
    print(f"Total samples: {total_samples}")
    print(f"Duration: {total_samples / 24000:.2f} seconds")
    print("✅ Streaming test passed!")
    return True


if __name__ == "__main__":
    test_streaming_tts()
