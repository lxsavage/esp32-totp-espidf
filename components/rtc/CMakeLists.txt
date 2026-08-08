cmake_minimum_required(VERSION 3.16.0)

idf_component_register(
    SRCS "rtc.c"
    INCLUDE_DIRS "."
    REQUIRES common
    PRIV_REQUIRES esp_timer esp_wifi storage
)
