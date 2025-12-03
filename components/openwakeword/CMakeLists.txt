idf_component_register(
    SRCS
        "openwakeword_wrapper.cpp"
        "openwakeword_esp32.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        freertos
        nvs_flash
)

# Note: OpenWakeWord models need to be downloaded first
# Run: ./download_models.sh in this directory
# 
# To use actual OpenWakeWord models, you need:
# 1. TensorFlow Lite for Microcontrollers (TFLM) component
# 2. Models downloaded to models/ directory
# 3. Update openwakeword_esp32.cpp to use TFLM for inference
#
# For now, this is a placeholder implementation that simulates detection
