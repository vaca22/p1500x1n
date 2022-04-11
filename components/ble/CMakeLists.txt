set(srcs)

idf_component_register(SRCS "myble.c"
                    INCLUDE_DIRS  "."
                    REQUIRES console nvs_flash bt
                    REQUIRED_IDF_TARGETS esp32)
