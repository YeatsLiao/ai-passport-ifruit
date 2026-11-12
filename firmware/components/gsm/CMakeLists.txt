idf_component_register(SRCS "src/add.c" "src/decode.c" "src/long_term.c" "src/rpe.c" "src/short_term.c" "src/table.c" "src/gsm_create.c" "src/gsm_destroy.c" "src/gsm_option.c" "src/gsm_decode.c" INCLUDE_DIRS "include")
target_compile_definitions(${COMPONENT_LIB} PRIVATE WAV49 SASR NDEBUG)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-comment -Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unused-const-variable -Wno-unused-variable)
