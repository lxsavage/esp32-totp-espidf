cmake_minimum_required(VERSION 3.16.0)

idf_component_register(
    SRCS "rtc.c"
    INCLUDE_DIRS "."
    REQUIRES common
    PRIV_REQUIRES freertos esp_wifi esp_event esp_netif lwip storage
)
