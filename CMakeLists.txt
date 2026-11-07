cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ai-passport-ifruit)

# Audio image is opt-in; empty/unprepared content never overwrites device storage.
if(IFRUIT_PACK_AUDIO)
    file(GLOB AUDIO_WAVS "${CMAKE_SOURCE_DIR}/build/audio/*.wav")
    if(NOT AUDIO_WAVS)
        message(FATAL_ERROR "No prepared WAVs; import originals and run tools/prepare_audio.py first")
    endif()
    spiffs_create_partition_image(assets "${CMAKE_SOURCE_DIR}/build/audio" FLASH_IN_PROJECT)
endif()
