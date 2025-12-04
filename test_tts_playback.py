#!/usr/bin/env python3
"""
Test TTS generation and playback locally
"""
import requests
import base64
import wave
import subprocess
import os
from pathlib import Path

api_key = "AIzaSyCjrdIBkpGWGXa4u9UileFFIMBZ_ZnMZ1w"

def generate_tts(text):
    """Generate TTS audio from text"""
    url = f"https://texttospeech.googleapis.com/v1/text:synthesize?key={api_key}"
    
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
    
    print(f"🎙️  Generating TTS: '{text}'")
    response = requests.post(url, json=payload)
    
    if response.status_code != 200:
        print(f"❌ Error: {response.status_code}")
        print(response.text)
        return None
    
    result = response.json()
    
    if "audioContent" not in result:
        print("❌ No audioContent in response")
        return None
    
    return result["audioContent"]

def save_wav(base64_audio, filename):
    """Save base64 audio to WAV file"""
    pcm_data = base64.b64decode(base64_audio)
    
    with wave.open(filename, 'wb') as wav:
        wav.setnchannels(1)  # Mono
        wav.setsampwidth(2)  # 16-bit
        wav.setframerate(24000)  # 24kHz
        wav.writeframes(pcm_data)
    
    duration = len(pcm_data) // (24000 * 2)
    print(f"✅ Saved {filename}")
    print(f"📊 Size: {len(pcm_data):,} bytes, Duration: {duration:.2f}s")
    return filename

def play_wav(filename):
    """Play WAV file using system audio player"""
    print(f"🔊 Playing: {filename}")
    
    # Try different players
    players = [
        ["afplay", filename],  # macOS
        ["ffplay", "-nodisp", "-autoexit", filename],  # ffplay (cross-platform)
        ["paplay", filename],  # PulseAudio (Linux)
        ["aplay", filename],  # ALSA (Linux)
        ["vlc", "--play-and-exit", filename],  # VLC (cross-platform)
    ]
    
    for player_cmd in players:
        try:
            result = subprocess.run(player_cmd, timeout=30, capture_output=True)
            if result.returncode == 0:
                print(f"✅ Played successfully with {player_cmd[0]}")
                return True
        except FileNotFoundError:
            continue
        except Exception as e:
            print(f"⚠️  {player_cmd[0]} failed: {e}")
            continue
    
    print("❌ No audio player found. Install afplay, ffplay, or vlc")
    return False

if __name__ == "__main__":
    # Generate TTS
    text = "Just say Hey, Nap to talk to me."
    base64_audio = generate_tts(text)
    
    if not base64_audio:
        exit(1)
    
    # Save to WAV
    wav_file = "/tmp/test_tts.wav"
    save_wav(base64_audio, wav_file)
    
    # Play it
    play_wav(wav_file)
    
    print("\n✅ Test complete!")
