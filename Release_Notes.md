# Release Notes for STM32N6-GettingStarted-ObjectDetection Application

## Purpose

Object Detection application to enable the deployment of object detection models on STM32N6570-DK and NUCLEO-N657X0-Q boards.

This application is prebuilt with an object detection model.

## Key Features

- Sequential application flow
- NPU accelerated quantized AI model inference
- Dual DCMIPP pipes
- DCMIPP crop, decimation, downscale
- DCMIPP ISP usage
- LTDC dual-layer implementation
- Dev mode
- Boot from External Flash

## Software components

| Name                                        | Version             | Release notes
|-----                                        | -------             | -------------
| STM32Cube.AI runtime                        |  v12.0.0            |
| ai-postprocessing-wrapper                   |  v1.1.0             | [release notes](Middlewares/ai-postprocessing-wrapper/Release_Notes.md)
| screenl                                     |  v3.0.2             | [release notes](Middlewares/screenl/Release_Notes.md)
| Camera Middleware                           |  v1.5.1             | [release notes](Middlewares/stm32-mw-camera/Release_Notes.md)
| stm32-vision-models-postprocessing Library  |  v0.14.2            | [release notes](Middlewares/stm32-vision-models-postprocessing/lib_vision_models_pp/README.md)
| CMSIS                                       |  v6.2.0             | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/st_readme.txt)
| STM32N6xx CMSIS Device                      |  v1.3.0             | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Release_Notes.html)
| STM32N6xx HAL/LL Drivers                    |  v1.3.0             | [release notes](STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Release_Notes.html)
| STM32N6570-DK BSP Drivers                   |  v1.3.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/Release_Notes.html)
| NUCLEO-N657X0-Q BSP Drivers                 |  v1.2.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6xx_Nucleo/Release_Notes.html)
| BSP Component aps256xx                      |  v1.0.6             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/Release_Notes.html)
| BSP Component Common                        |  v7.3.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/Common/Release_Notes.html)
| BSP Component mx66uw1g45g                   |  v1.1.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/Release_Notes.html)
| BSP Component rk050hr18                     |  v1.0.1             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/rk050hr18/Release_Notes.html)
| Fonts Utility                               |  v2.0.3             | [release notes](STM32Cube_FW_N6/Utilities/Fonts/Release_Notes.html)
| lcd Utility                                 |  v2.2.0             | [release notes](STM32Cube_FW_N6/Utilities/lcd/Release_Notes.html)
| ai fsbl                                     |  v1.4.0             | [release notes](FSBL/Release_Notes.md)

## Available documentation

- README.md
- Doc/Application-Overview.md
- Doc/Boot-Overview.md
- Doc/Build-Options.md
- Doc/Neural-ART-Description-and-Operation.md
- Doc/Deploy-your-Quantized-Model.md
- Doc/Program-Hex-Files-STM32CubeProgrammer.md

## Update history

### v2.3.1 / September 2026

- Fix STM32_SigningTool_CLI API break since v2.21 by adding `--align` flag

### v2.3.0 / April 2026

- New configuration for ModelZoo allows deploying larger models by storing epoch controller blob in flash memory
- Update inference function
- Update STEdgeAI to v4.0.0
- Update ai-postprocessing-wrapper to v1.1.0

### v2.2.1 / January 2026

- Add Yolo-D and new SSD models support
- Update lib_vision_models_pp to v0.14.2
- Update ai-postprocessing-wrapper to v1.0.10

### v2.2.0 / January 2026

- Update Camera Middleware to v1.5.0, adding VD1943 support
- Update lib_vision_models_pp to v0.13.0
- Update ai-postprocessing-wrapper to v1.0.9
- Update Cube FW to v1.3.0
- Update STEdgeAI to v3.0.0
- Switch to STEdgeAI `st-ai` API

### v2.1.1 / August 2025

- Update Camera Middleware to v1.4.3
- Update lib_vision_models_pp to v0.10.0
- Update ai-postprocessing-wrapper to v1.0.7
- Update screenl to v3.0.0
- Documentation improvements

### v2.1.0 / July 2025

- Update to Cube FW to v1.2.0
- Update to STEdgeAI v2.2.0
- Add int8 post processing support for:
  - Object detection Yolo v2
  - Object detection Yolo v8
  - Object detection ST Yolo X
  - Single pose estimation Movenet
  - Multi pose estimation Yolo v8
  - Semantic segmentation Deeplab v3
- Add support for BlazeFace post processing

### v2.0.0 / April 2025

- Add support of the NUCLEO-N657X0-Q board
- Update miscellaneous drivers
- Update Camera Middleware to v1.4.2
- Miscellaneous refactoring

### v1.0.0 / December 2024

Initial Release