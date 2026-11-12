set(DEVICE_RESOURCES "${PROJECT_DIR}/resources")
if(NOT EXISTS "${DEVICE_RESOURCES}/ui_assets.c")
    message(FATAL_ERROR "Missing firmware/resources; restore prepared device inputs or run python -m tools.pack.prepare_firmware from the repository root")
endif()
idf_component_register(
    SRCS "main.c" "screenshot.c" "phone.c" "communications.c" "world.c"
         "simulation.c" "story.c" "phonebook.c" "ui.c" "audio.c" "wav.c" "storage.c"
         "${DEVICE_RESOURCES}/ui_assets.c" "${DEVICE_RESOURCES}/ui_fonts.c"
         "${DEVICE_RESOURCES}/phone_content.c"
    INCLUDE_DIRS "." "${DEVICE_RESOURCES}"
    REQUIRES esp_driver_usb_serial_jtag esp_timer bsp lvgl spiffs nvs_flash gsm)
