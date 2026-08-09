# Layer-based underglow color. Central-only: layer state doesn't exist
# on the peripheral half.
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  zephyr_library_sources_ifdef(CONFIG_ZMK_RGB_UNDERGLOW src/rgb_layer_color.c)
endif()

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
