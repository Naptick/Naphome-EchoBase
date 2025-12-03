idf_component_register(
    SRCS 
        "app_main.c"
        "audio_player.c"
    INCLUDE_DIRS "."
    REQUIRES
        led_strip
        helix_mp3
        korvo1
        driver
        freertos
        nvs_flash
    EMBED_FILES "../256kMeasSweep_0_to_20000_-12_dBFS_48k_Float_LR_refL.wav"
)
