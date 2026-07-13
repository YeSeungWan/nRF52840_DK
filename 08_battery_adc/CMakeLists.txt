cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(08_battery_adc)


# ---------------------------------------------
# Application Source Configuration
# ---------------------------------------------

# Add source files to the application target
target_sources(app PRIVATE 
    src/app/src/main.c
    src/app/src/app_uart.c
    src/app/src/app_watchdog.c
    src/app/src/app_battery.c
    src/bsp/src/bsp_led.c
    src/bsp/src/bsp_button.c
    src/bsp/src/bsp_uart.c
    src/bsp/src/bsp_battery.c
)

# Include directories for header files
target_include_directories(app PRIVATE src/app/inc)
target_include_directories(app PRIVATE src/bsp/inc)

target_link_libraries(app PRIVATE zephyr_interface)