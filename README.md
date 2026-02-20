# MiniGUI

A lightweight, standalone, and portable UI manager for LVGL-based embedded systems. Designed for 800x480 resolution but adaptable to various display sizes.

## 🌟 Key Features

- **Standalone Architecture**: Zero dependencies outside of LVGL (v9.2+).
- **Flexible Layout**: Uses LVGL Flex layouts for responsive containers (Status Bar, Content Area).
- **Navigation System**: Built-in sidebar menu (hamburger style) for quick screen switching.
- **Pre-built Screens**:
  - **Home**: Dashboard view.
  - **System Logs**: Dynamic table view with filtering (supports internal mock data for testing).
  - **Settings**: Control panel for device parameters (e.g., Brightness).
- **Hardware Integration Hooks**: Easy-to-use callback registration for brightness control and other hardware-specific tasks.

## 🎨 Typography

MiniGUI uses a consistent hierarchy based on the **Montserrat** font:

- **36px**: Status Bar titles.
- **24px**: Page headers and sidebar menu buttons.
- **20px**: Clock details and primary sub-section headers.
- **16px**: Secondary details and small labels.

## 📂 Project Structure

```text
minigui/
├── include/
│   ├── minigui.h         # Main Public API & Common Types
│   ├── minigui_menu.h    # Menu Controller Interface
│   └── screens/          # Individual Screen Headers
├── src/
│   ├── minigui.c         # Layout & Orchestration
│   ├── minigui_menu.c    # Sidebar Menu Logic
│   └── screens/          # Screen Implementations (Home, Logs, Settings)
└── CMakeLists.txt        # IDF-compatible component definition
```

## 🚀 Getting Started

### Prerequisites

- [LVGL v9.4](https://github.com/lvgl/lvgl)
- C99 compatible compiler

### Integration

1.  **Add as a Submodule**:
    ```bash
    git submodule add https://github.com/vwadlington/minigui components/minigui
    ```

2.  **Initialize Application**:
    In your `main.c` (or wherever your LVGL task is initialized):
    ```c
    #include "minigui.h"

    int main() {
        // 1. Initialize LVGL and your Display/Input HAL
        lv_init();
        your_hal_init();

        // 2. Initialize MiniGUI
        minigui_init();

        // 3. (Optional) Hook up hardware controls
        minigui_register_brightness_cb(my_hardware_brightness_fn);

        while(1) {
            lv_timer_handler();
            usleep(5000);
        }
    }
    ```

## 📊 Mock Data & Portability

MiniGUI is designed to be environment-aware. It handles logging through a **Log Provider API**:

1.  **Automatic Mocking (Simulator)**: If you compile with `-DMINIGUI_USE_MOCK_LOGS=1`, the UI will automatically fall back to internal mock data if no external provider is registered. This is the default in the VSCode simulator.
2.  **Hardware Logs (Embedded)**: When moving to an actual project, simply register your real log retrieval function:
    ```c
    // Your real ESP-IDF bridge function
    size_t my_esp_log_bridge(minigui_log_entry_t *logs, size_t max, const char *f) {
        return app_bridge_get_logs(logs, max, f);
    }

    // Register it during startup
    minigui_set_log_provider(my_esp_log_bridge);
    ```

This architecture ensures that `minigui` remains a clean, standalone component that doesn't need its source code modified when switching between a simulator and real hardware.

## 🛠 Public API

### `minigui_init()`
Initializes the main UI structure. Loads the Home screen by default.

### `minigui_set_log_provider(minigui_log_provider_t provider)`
Registers a callback to retrieve system logs. Required for the Logs screen to function on real hardware.

### `minigui_set_time_provider(minigui_time_provider_t provider)`
Registers a callback to retrieve formatted time. If not set, the UI falls back to the standard C `<time.h>` library.

### `minigui_switch_screen(minigui_screen_t screen)`
Switches the active screen in the content area.

### `minigui_register_brightness_cb(minigui_brightness_cb_t cb)`
Registers a function pointer to handle brightness changes.

### `minigui_set_brightness(uint8_t brightness)`
Updates the display brightness (proxies to the registered callback).

### `minigui_register_wifi_save_cb(minigui_wifi_save_cb_t cb)`
Registers a callback to handle saving WiFi credentials.

### `minigui_register_wifi_scan_provider(minigui_wifi_scan_provider_t provider)`
Registers a function to perform WiFi network scanning.

### `minigui_scan_wifi(minigui_wifi_network_t *networks, size_t max_count)`
Triggers a WiFi scan (uses registered provider or internal mock).

### `minigui_register_system_stats_provider(minigui_system_stats_provider_t provider)`
Registers a function to retrieve system health/stats (Voltage, CPU, RAM).

### `minigui_get_system_stats(minigui_system_stats_t *stats)`
Retrieves current system statistics.

### `minigui_register_network_status_provider(minigui_network_status_provider_t provider)`
Registers a function to retrieve network connection status (IP, MAC, SSID).

### `minigui_get_network_status(minigui_network_status_t *status)`
Retrieves current network status.

```c
typedef struct {
    char ssid[64];
    char password[128];
} minigui_wifi_credentials_t;
```

## ⚖️ License

MIT License. See [LICENSE](LICENSE) for details.
