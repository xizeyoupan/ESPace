docker build -t esp-adf:v2.7-esp32 \
    --build-arg IDF_INSTALL_TARGETS=esp32 \
    --build-arg ADF_CLONE_BRANCH_OR_TAG=v2.7 \
    --build-arg IDF_CLONE_BRANCH_OR_TAG=release/v5.2 \
    --build-arg IDF_CLONE_SHALLOW=1 \
    --build-arg ADF_CLONE_SHALLOW=1 \
    tools/docker
