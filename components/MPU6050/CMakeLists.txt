idf_component_register(SRCS "MPU6050.cpp"
                      INCLUDE_DIRS "."
                      PRIV_REQUIRES driver esp_timer
                      REQUIRES I2Cdev)
