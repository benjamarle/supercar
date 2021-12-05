set(COMPONENT_SRCS  "supercar_main.c"
                    "supercar_motor.c"
                    "esp_hid_gap.c"
                    "esp_hid_host.c"
                    "supercar_sensor.c"
                    "esp_rest_main.c"
                    "rest_server.c")

idf_component_register(SRCS "supercar_config.c" "supercar_sensor.c" "supercar_motor.c" "supercar_main.c" "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "./"
                    )



if(CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front")
    if(EXISTS ${WEB_SRC_DIR}/build)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/build FLASH_IN_PROJECT)
    else()
        message(FATAL_ERROR "${WEB_SRC_DIR}/build doesn't exit. Please run 'yarn build' in ${WEB_SRC_DIR}")
    endif()
endif()

