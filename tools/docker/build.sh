docker build -t intemd/esp-adf:v2.x-idfv5.4.0-esp32 \
    --build-arg IDF_INSTALL_TARGETS=esp32 \
    --build-arg ADF_CLONE_BRANCH_OR_TAG=release/v2.x \
    --build-arg IDF_CLONE_BRANCH_OR_TAG=v5.4 \
    --build-arg IDF_CLONE_SHALLOW=1 \
    --build-arg ADF_CLONE_SHALLOW=1 \
    tools/docker
