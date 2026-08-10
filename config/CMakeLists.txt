# Layer-based underglow color. Central-only: layer state doesn't exist
# on the peripheral half.
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_library_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW src/rgb_layer_color.c)
endif()

# Custom status screen - step 0 (mechanism test). Applies to both halves,
# each has its own display.
#
# On this branch nice!oled provides the status screen and sets
# ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y itself (its Kconfig.defconfig
# defaults the ZMK_DISPLAY_STATUS_SCREEN choice to CUSTOM). Without this
# guard that symbol would drag our src/status_screen.c into the build
# alongside nice!oled's, and both define zmk_display_status_screen ->
# duplicate symbol at link. Ours stays out whenever nice!oled is active.
if(NOT CONFIG_NICE_OLED_ON AND NOT CONFIG_NICE_EPAPER_ON AND NOT CONFIG_NICE_CUSTOM_ON)
  zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM src/status_screen.c)
endif()

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
