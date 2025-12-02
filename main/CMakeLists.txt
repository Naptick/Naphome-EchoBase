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
)
