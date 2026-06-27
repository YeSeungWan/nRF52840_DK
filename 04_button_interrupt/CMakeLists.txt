cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(03_sysinit_driver)


# ---------------------------------------------
# Application Source Configuration
# ---------------------------------------------

# Add source files to the application target
target_sources(app PRIVATE 
    src/main.c
    bsp/src/bsp_led.c
    bsp/src/bsp_button.c
)

# Include directories for header files
target_include_directories(app PRIVATE bsp/inc)