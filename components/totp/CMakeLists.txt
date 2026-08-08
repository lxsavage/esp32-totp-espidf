cmake_minimum_required(VERSION 3.16.0)

idf_component_register(
    SRCS "totp.c"
    INCLUDE_DIRS "."
    REQUIRES common mbedtls
    PRIV_REQUIRES storage
)
