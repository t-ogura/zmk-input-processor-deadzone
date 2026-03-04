if(CONFIG_ZMK_INPUT_PROCESSOR_DEADZONE)
    zephyr_library()
    zephyr_library_sources(src/input_processor_deadzone.c)
endif()
