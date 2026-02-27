#[[
File: CMakeLists.txt
Description: Build configuration for the MiniGUI component.
Responsibilities:
1. Register source files for the MiniGUI module.
2. Define include directories for public headers.
3. Specify private dependencies required by the component (LVGL).
]]

# 1. Register component sources and requirements
# Rationale: LVGL is a private requirement as it is used for internal UI implementation
# but not necessarily required in full by users of minigui.h (which proxies certain calls).
idf_component_register(
    SRCS
        # 1. Main UI manager
        "src/minigui.c"
        # 2. Sidebar/Sidebar menu system
        "src/minigui_menu.c"
        # 3. Individual screen implementations
        "src/screens/screen_home.c"
        "src/screens/screen_logs.c"
        "src/screens/screen_settings.c"
    INCLUDE_DIRS 
        # 1. Public headers for external module usage
        "include"
    PRIV_REQUIRES 
        # 1. Core graphics library
        lvgl
)

# 2. Console Feedback
message(STATUS "MiniGUI: Component registered successfully.")
