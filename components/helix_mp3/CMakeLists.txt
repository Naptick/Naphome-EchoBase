idf_component_register(
    SRCS
        "src/mp3_decoder.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        driver
        freertos
        esp_timer
)

# Define MINIMP3_IMPLEMENTATION to enable the implementation in the header
target_compile_definitions(${COMPONENT_LIB} PRIVATE MINIMP3_IMPLEMENTATION)
