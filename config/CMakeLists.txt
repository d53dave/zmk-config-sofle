# Layer-based underglow color. Central-only: layer state doesn't exist
# on the peripheral half.
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_library_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW src/rgb_layer_color.c)
endif()

# Custom status screen - step 0 (mechanism test). Applies to both halves,
# each has its own display.
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM src/status_screen.c)

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
