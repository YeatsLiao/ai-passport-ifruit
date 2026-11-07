if(NOT EXISTS "${PROJECT_DIR}/build/generated/ui_assets.c")
    message(FATAL_ERROR "Run python tools/prepare_ui.py before building (requires Pillow)")
endif()
idf_component_register(SRCS "main.c" "phone.c" "world.c" "ui.c" "audio.c" "wav.c" "storage.c" "${PROJECT_DIR}/build/generated/ui_assets.c" "${PROJECT_DIR}/build/generated/phone_content.c" INCLUDE_DIRS "." "${PROJECT_DIR}/build/generated" REQUIRES bsp lvgl spiffs nvs_flash)
