set(COMPONENT_SRCS  "supercar_main.c"
                    "esp_hid_gap.c"
                    "esp_hid_host.c"
                    "http_server.c")

idf_component_register(SRCS "supercar_motor.c" "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "./"
                    )
