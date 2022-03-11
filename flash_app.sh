#/bin/sh


APP_NAME=$1
APP_VERSION=$2

esptool.py write_flash 0x01000 output/dist/${APP_NAME}_${APP_VERSION}/${APP_NAME}_8M_UA_TLS_${APP_VERSION}.bin