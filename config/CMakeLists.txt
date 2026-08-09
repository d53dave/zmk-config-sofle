if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library_sources(src/status_screen.c)
  zephyr_library_sources(src/widgets/cat.c)

  if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(src/widgets/info.c)
  else()
    zephyr_library_sources(src/widgets/link_status.c)
  endif()

  zephyr_include_directories(include)
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
endif()
