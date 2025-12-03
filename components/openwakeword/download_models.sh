#!/bin/bash

# Download OpenWakeWord TFLite models for ESP32
# These models will be embedded in the firmware

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODELS_DIR="$SCRIPT_DIR/models"
RELEASE_VERSION="v0.5.1"
BASE_URL="https://github.com/dscripka/openWakeWord/releases/download/${RELEASE_VERSION}"

mkdir -p "$MODELS_DIR"

echo "Downloading OpenWakeWord models..."

# Download feature models (required for preprocessing)
echo "Downloading feature models..."
curl -L -o "$MODELS_DIR/melspectrogram.tflite" "${BASE_URL}/melspectrogram.tflite"
curl -L -o "$MODELS_DIR/embedding_model.tflite" "${BASE_URL}/embedding_model.tflite"

# Download wake word models (choose one or more)
echo "Downloading wake word models..."
curl -L -o "$MODELS_DIR/hey_jarvis_v0.1.tflite" "${BASE_URL}/hey_jarvis_v0.1.tflite"
# Uncomment to download other models:
# curl -L -o "$MODELS_DIR/alexa_v0.1.tflite" "${BASE_URL}/alexa_v0.1.tflite"
# curl -L -o "$MODELS_DIR/hey_mycroft_v0.1.tflite" "${BASE_URL}/hey_mycroft_v0.1.tflite"

echo "Models downloaded to $MODELS_DIR"
echo ""
echo "Available models:"
ls -lh "$MODELS_DIR"/*.tflite
