idf_component_register(
    SRCS
    "CNN.cpp"
    "nvs_util.cpp"
    "main.cpp"
    "./task/play_mp3.cpp"
    "./task/button.cpp"
    "./task/handle_req_task.cpp"
    "./task/mpu_task.cpp"
    "./task/wand_server_task.cpp"
    "./task/ws_task.cpp"
    "./task/ws2812_task.cpp"
    "./service/wifi_service.cpp"
    "./service/device_service.cpp"
    "./service/button_service.cpp"
    "./service/http_service.cpp"
    "./service/littlefs_service.cpp"
    "./service/ws2812_service.cpp"
    "./service/mpu_service.cpp"
    INCLUDE_DIRS
    "."
    "./service"
    "./task"
)

# littlefs_create_partition_image(models ../flash_data FLASH_IN_PROJECT)
