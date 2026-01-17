idf_component_register(
    SRCS 
        "src/minigui.c"
        "src/minigui_menu.c"
        "src/screens/screen_home.c"
        "src/screens/screen_logs.c"
        "src/screens/screen_settings.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lvgl esp_timer main
)