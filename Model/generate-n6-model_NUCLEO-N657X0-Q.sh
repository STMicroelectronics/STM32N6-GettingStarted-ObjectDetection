#!/bin/bash

stedgeai generate --model quantized_tiny_yolo_v2_224_.tflite --target stm32n6 --st-neural-art default@user_neuralart_NUCLEO-N657X0-Q.json --input-data-type uint8
cp st_ai_output/network.c NUCLEO-N657X0-Q/
cp st_ai_output/network_ecblobs.h NUCLEO-N657X0-Q/
cp st_ai_output/network_atonbuf.xSPI2.raw NUCLEO-N657X0-Q/network_data.xSPI2.bin
arm-none-eabi-objcopy -I binary NUCLEO-N657X0-Q/network_data.xSPI2.bin --change-addresses 0x70380000 -O ihex NUCLEO-N657X0-Q/network_data.hex
