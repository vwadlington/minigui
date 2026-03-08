#[[
File: CMakeLists.txt
Description: Build configuration for the MiniGUI component.
Responsibilities:
1. Register source files for the MiniGUI module.
2. Define include directories for public headers.
3. Specify private dependencies required by the component (LVGL)
4. Register the component with ESP-IDF build system.
5. Console Feedback
]]

# 1. Register source files for the MiniGUI module.
set(MINIGUI_SOURCES
    "src/minigui.c"
    "src/minigui_menu.c"
    "src/screens/screen_home.c"
    "src/screens/screen_logs.c"
    "src/screens/screen_settings.c"
)

# 2. Define include directories for public headers.
set(MINIGUI_INCLUDE_DIRS
    "include"
    "include/screens"
)

# 3. Specify private dependencies required by the component (LVGL).
set(MINIGUI_REQUIRES
    # Identify the LVGL component name dynamically
    idf_component_get_listed_name(lvgl_lib "lvgl__lvgl")
    if(NOT lvgl_lib)
        set(lvgl_lib "lvgl")
    endif()
)

# 4. Register the component with ESP-IDF build system.
idf_component_register(SRCS ${MINIGUI_SOURCES}
                       INCLUDE_DIRS ${MINIGUI_INCLUDE_DIRS}
                       PRIV_REQUIRES ${MINIGUI_REQUIRES})

# 5. Console Feedback
message(STATUS "MiniGUI: Component registered successfully.")
