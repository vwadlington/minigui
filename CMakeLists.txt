# 1. Find all .c files in the 'src' directory and its subdirectories
file(GLOB_RECURSE SOURCES "src/*.c")

idf_component_register(SRCS "${SOURCES}"
                        INCLUDE_DIRS "include"
                        REQUIRES lvgl__lvgl)