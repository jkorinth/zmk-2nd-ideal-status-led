list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

if(CONFIG_ZMK_2NDIDEAL_STATUS_LED)
	zephyr_library()
	zephyr_nanopb_sources(${ZEPHYR_CURRENT_LIBRARY} proto/si-sl.proto)
	zephyr_library_sources(src/status_led.c src/behaviors/behavior_status_led.c)
	zephyr_library_sources(src/behaviors/behavior_bl_select.c
	                        src/behaviors/behavior_bl_adjust.c
	                        src/behaviors/behavior_bl_commit.c)
	if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
		zephyr_library_sources(src/bl_store.c)
	endif()
	zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
	zephyr_include_directories(include)
endif()
