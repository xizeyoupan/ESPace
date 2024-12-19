idf_component_register(SRCS "http_util.c" "nvs_util.c" "main.c" "wand_server_task.c" "mpu6050.cpp" "wifi.c" "udp_trans.c" "web_server.c" "web_client.c" "tftest.cc"
    INCLUDE_DIRS "."
    EMBED_FILES "../html/error.html"
                "../html/favicon.ico"
                "../html/main.js"
                "../html/root.html"
                "../html/main.css")
