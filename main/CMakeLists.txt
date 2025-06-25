idf_component_register(
    SRCS
    "CNN.cpp"
    "nvs_util.c"
    "main.c"
    "./task/play_mp3.c"
    "./task/status_task.c"
    "./task/button.c"
    "./task/handle_req_task.c"
    "./task/mpu_task.c"
    "./task/wand_server_task.c"
    "./task/ws_task.c"
    "./task/ws2812_task.c"
    "./service/wifi_service.c"
    "./service/device_service.c"
    "./service/button_service.c"
    "./service/http_service.c"
    "./service/littlefs_service.c"
    "./service/ws2812_service.c"
    "./service/mpu_service.cpp"
    INCLUDE_DIRS
    "."
    "./service"
    "./task"
)

# littlefs_create_partition_image(models ../flash_data FLASH_IN_PROJECT)
