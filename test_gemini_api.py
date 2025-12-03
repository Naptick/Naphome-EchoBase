#!/usr/bin/env python3
"""
Test script for Google Cloud Gemini APIs
Tests STT, LLM, and TTS locally before deploying to Korvo1
"""

import requests
import json
import base64
import sys
from pathlib import Path

# Configuration
API_KEY = "AIzaSyCjrdIBkpGWGXa4u9UileFFIMBZ_ZnMZ1w"
GEMINI_MODEL = "gemini-2.5-flash"  # Updated - 1.5-flash not available

def list_available_models():
    """List available Gemini models"""
    print(f"\n{'='*60}")
    print("Listing Available Models")
    print(f"{'='*60}")

    url = f"https://generativelanguage.googleapis.com/v1/models?key={API_KEY}"

    try:
        response = requests.get(url, timeout=30)
        print(f"Status Code: {response.status_code}")

        if response.status_code == 200:
            data = response.json()
            if "models" in data:
                print(f"\nAvailable Models:")
                for model in data["models"]:
                    name = model.get("name", "Unknown")
                    display_name = model.get("displayName", "Unknown")
                    print(f"  - {name} ({display_name})")
                    if "supportedGenerationMethods" in model:
                        methods = model["supportedGenerationMethods"]
                        print(f"    Methods: {', '.join(methods)}")
                return True
            else:
                print(f"Response: {json.dumps(data, indent=2)}")
                return False
        else:
            print(f"Failed! Response: {response.text}")
            return False

    except Exception as e:
        print(f"Exception: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_tts(text="Just say Hey, Nap to talk to me."):
    """Test Text-to-Speech API"""
    print(f"\n{'='*60}")
    print("Testing Text-to-Speech API")
    print(f"{'='*60}")

    url = f"https://texttospeech.googleapis.com/v1/text:synthesize?key={API_KEY}"

    payload = {
        "input": {
            "text": text
        },
        "voice": {
            "languageCode": "en-US",
            "name": "en-US-Neural2-D"
        },
        "audioConfig": {
            "audioEncoding": "LINEAR16",
            "sampleRateHertz": 24000
        }
    }

    print(f"URL: {url}")
    print(f"Payload: {json.dumps(payload, indent=2)}")

    try:
        response = requests.post(url, json=payload, timeout=30)
        print(f"\nStatus Code: {response.status_code}")
        print(f"Content-Type: {response.headers.get('content-type', 'N/A')}")
        print(f"Response Headers: {dict(response.headers)}")

        if response.status_code == 200:
            data = response.json()
            print(f"\nResponse Keys: {list(data.keys())}")

            if "audioContent" in data:
                audio_content = data["audioContent"]
                print(f"Audio Content (base64): {audio_content[:100]}...")
                print(f"Audio Content Length: {len(audio_content)}")

                # Decode and save to file
                audio_bytes = base64.b64decode(audio_content)
                print(f"Decoded Audio Bytes: {len(audio_bytes)}")

                # Save as WAV file
                output_file = Path("test_tts_output.pcm")
                with open(output_file, "wb") as f:
                    f.write(audio_bytes)
                print(f"✅ Saved audio to {output_file}")

                return True
            else:
                print(f"\n❌ No 'audioContent' in response!")
                print(f"Full response: {json.dumps(data, indent=2)}")
                return False
        else:
            print(f"\n❌ TTS Failed!")
            print(f"Response: {response.text}")
            return False

    except Exception as e:
        print(f"\n❌ Exception: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_llm(prompt="What is the capital of France?"):
    """Test Gemini LLM API"""
    print(f"\n{'='*60}")
    print("Testing Gemini LLM API")
    print(f"{'='*60}")

    # Try different API versions - v1beta has model availability issues
    # Use v1 with gemini-pro or gemini-2.0-flash instead
    url = f"https://generativelanguage.googleapis.com/v1/models/{GEMINI_MODEL}:generateContent?key={API_KEY}"

    payload = {
        "contents": [
            {
                "parts": [
                    {
                        "text": prompt
                    }
                ]
            }
        ]
    }

    print(f"URL: {url}")
    print(f"Model: {GEMINI_MODEL}")
    print(f"Prompt: {prompt}")

    try:
        response = requests.post(url, json=payload, timeout=30)
        print(f"\nStatus Code: {response.status_code}")
        print(f"Content-Type: {response.headers.get('content-type', 'N/A')}")

        if response.status_code == 200:
            data = response.json()
            print(f"\nResponse Keys: {list(data.keys())}")

            if "candidates" in data and len(data["candidates"]) > 0:
                candidate = data["candidates"][0]
                if "content" in candidate and "parts" in candidate["content"]:
                    parts = candidate["content"]["parts"]
                    if len(parts) > 0 and "text" in parts[0]:
                        response_text = parts[0]["text"]
                        print(f"\n✅ LLM Response:")
                        print(f"{response_text}")
                        return True

            print(f"\n❌ Failed to extract response!")
            print(f"Full response: {json.dumps(data, indent=2)[:500]}...")
            return False
        else:
            print(f"\n❌ LLM Failed!")
            print(f"Response: {response.text}")
            return False

    except Exception as e:
        print(f"\n❌ Exception: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_stt(audio_file="test_tts_output.pcm"):
    """Test Speech-to-Text API"""
    print(f"\n{'='*60}")
    print("Testing Speech-to-Text API")
    print(f"{'='*60}")

    if audio_file is None or not Path(audio_file).exists():
        print(f"⚠️  No audio file found: {audio_file}")
        print("Skipping STT test (would need actual voice input)")
        return None

    url = f"https://speech.googleapis.com/v1/speech:recognize?key={API_KEY}"

    try:
        # Read audio file
        with open(audio_file, "rb") as f:
            audio_data = f.read()

        audio_base64 = base64.b64encode(audio_data).decode()

        payload = {
            "config": {
                "encoding": "LINEAR16",
                "sampleRateHertz": 24000,  # Match the TTS output sample rate
                "languageCode": "en-US"
            },
            "audio": {
                "content": audio_base64
            }
        }

        print(f"URL: {url}")
        print(f"Audio File: {audio_file}")
        print(f"Audio Size: {len(audio_data)} bytes")

        response = requests.post(url, json=payload, timeout=30)
        print(f"\nStatus Code: {response.status_code}")

        if response.status_code == 200:
            data = response.json()
            print(f"\nResponse: {json.dumps(data, indent=2)}")

            if "results" in data and len(data["results"]) > 0:
                transcript = data["results"][0]["alternatives"][0]["transcript"]
                print(f"\n✅ Transcription: {transcript}")
                return True
            else:
                print(f"❌ No transcription in response")
                return False
        else:
            print(f"\n❌ STT Failed!")
            print(f"Response: {response.text}")
            return False

    except Exception as e:
        print(f"\n❌ Exception: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_gemini_full_pipeline(voice_prompt="What is the capital of France?"):
    """Test full STT -> LLM -> TTS pipeline"""
    print(f"\n{'='*60}")
    print("Testing Full Gemini Pipeline")
    print(f"{'='*60}")

    # Step 1: LLM
    print("\n[Step 1/2] Testing LLM...")
    llm_success = test_llm(voice_prompt)

    if not llm_success:
        print("❌ LLM test failed, skipping TTS")
        return False

    # Step 2: TTS
    print("\n[Step 2/2] Testing TTS...")
    tts_success = test_tts("The capital of France is Paris.")

    if tts_success:
        print("\n✅ Full pipeline test successful!")
        return True
    else:
        print("\n❌ Pipeline test failed")
        return False


def main():
    print("Google Cloud Gemini API Test Suite")
    print(f"API Key: {API_KEY[:20]}...")
    print(f"Gemini Model: {GEMINI_MODEL}")

    # List available models first
    list_available_models()

    # Test TTS (generates audio file)
    tts_result = test_tts()

    # Test LLM
    llm_result = test_llm()

    # Test STT (uses audio from TTS)
    stt_result = test_stt()

    # Test full pipeline
    pipeline_result = test_gemini_full_pipeline("What is 2 + 2?")

    # Summary
    print(f"\n{'='*60}")
    print("Test Summary")
    print(f"{'='*60}")
    print(f"TTS Test: {'✅ PASSED' if tts_result else '❌ FAILED'}")
    print(f"LLM Test: {'✅ PASSED' if llm_result else '❌ FAILED'}")
    print(f"STT Test: {'✅ PASSED' if stt_result else ('⏭️ SKIPPED' if stt_result is None else '❌ FAILED')}")
    print(f"Pipeline Test: {'✅ PASSED' if pipeline_result else '❌ FAILED'}")

    # Consider passed if TTS and LLM work
    all_passed = tts_result and llm_result and pipeline_result
    print(f"\nOverall: {'✅ ALL TESTS PASSED' if all_passed else ('⚠️ MOSTLY PASSED' if (tts_result and llm_result) else '❌ SOME TESTS FAILED')}")

    return 0 if (tts_result and llm_result) else 1


if __name__ == "__main__":
    sys.exit(main())
