docker run --rm -v $PWD:/project -w /project -u $UID -e HOME=/tmp intemd/esp-adf:v2.7-idf-v5.2-esp32 idf.py build
