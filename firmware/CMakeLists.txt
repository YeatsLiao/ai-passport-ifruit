cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ai-passport-ifruit)

# Complete firmware includes validated device audio by default.
option(IFRUIT_PACK_AUDIO "Include the device audio partition when flashing" ON)
if(IFRUIT_PACK_AUDIO)
    set(AUDIO_DIR "${CMAKE_CURRENT_LIST_DIR}/resources/audio")
    file(GLOB AUDIO_WAVS "${AUDIO_DIR}/*.wav")
    if(NOT AUDIO_WAVS)
        message(FATAL_ERROR "Missing firmware/resources/audio; restore prepared device inputs or run python -m tools.pack.prepare_firmware from the repository root")
    endif()
    spiffs_create_partition_image(assets "${AUDIO_DIR}" FLASH_IN_PROJECT)
endif()
