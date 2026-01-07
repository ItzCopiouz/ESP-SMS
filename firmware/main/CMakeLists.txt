# Component CMakeLists.txt for the main application
# In ESP-IDF, "main" is treated as a special component

# Register this as a component and list all source files
idf_component_register(
    SRCS 
        "main.c"
        "wifi.c"
        "camera.c"
        "http_client.c"
        "power.c"
    INCLUDE_DIRS 
        "."
    REQUIRES
        esp_wifi
        esp_http_client
        esp_event
        nvs_flash
        esp_netif
        driver
        esp_adc
        esp-tls
        esp_timer
)


