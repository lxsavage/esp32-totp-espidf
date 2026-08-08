cmake_minimum_required(VERSION 3.16.0)

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(
  SRCS ${app_sources}
  REQUIRES freertos esp_timer esp_driver_gpio common display storage
)
