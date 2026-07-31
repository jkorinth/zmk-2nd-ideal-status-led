if(CONFIG_ZMK_2NDIDEAL_STATUS_LED)
	zephyr_library()
	zephyr_library_sources(src/status_led.c)
endif()
