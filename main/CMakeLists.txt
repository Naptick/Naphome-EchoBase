set(COMPONENT_REQUIRES
    led_strip
    helix_mp3
    openwakeword
    gemini
    driver
    freertos
    nvs_flash
    esp_http_client
    json
    esp_wifi
    esp_netif
    lwip
)

# Add board-specific driver
if(CONFIG_BOARD_KORVO1)
    list(APPEND COMPONENT_REQUIRES korvo1)
elseif(CONFIG_BOARD_M5_ECHO_BASE)
    list(APPEND COMPONENT_REQUIRES m5_echo_base)
endif()

set(COMPONENT_SRCS
    "app_main.c"
    "audio_player.c"
    "audio_eq.c"
    "audio_abstraction.c"
    "voice_assistant.c"
    "wifi_manager.c"
)

# TODO: wake_word_manager.c needs board abstraction (korvo1 vs m5)
# Currently disabled for M5 build. Will be re-enabled after Phase 2
# if(CONFIG_BOARD_KORVO1)
#     list(APPEND COMPONENT_SRCS "wake_word_manager.c")
# endif()

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS "."
    REQUIRES ${COMPONENT_REQUIRES}
    EMBED_FILES "../256kMeasSweep_0_to_20000_-12_dBFS_48k_Float_LR_refL.wav"
)
