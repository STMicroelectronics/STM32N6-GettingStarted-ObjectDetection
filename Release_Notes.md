# Release Notes for STM32N6-GettingStarted-ObjectDetection Application

## Purpose

Object Detection application to enable the deployment of object detections models on STM32N6570-DK and NUCLEO-N657X0-Q boards.

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

| Name                          | Version             | Release notes
|-----                          | -------             | -------------
| STM32Cube.AI runtime          |  v10.1.0            |
| ai-postprocessing-wrapper     |  v1.0.2             | [release notes](Middlewares/ai-postprocessing-wrapper/Release_Notes.md)
| screenl                       |  v2.1.1             | [release notes](Middlewares/screenl/Release_Notes.md)
| Camera Middleware             |  v1.4.2             | [release notes](Middlewares/Camera_Middleware/Release_Notes.md)
| lib_vision_models_pp Library  |  v0.8.0             | [release notes](Middlewares/lib_vision_models_pp/lib_vision_models_pp/README.md)
| CMSIS                         |  v5.9.0             | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Documentation/index.html)
| STM32N6xx CMSIS Device        |  v1.1.0             | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Release_Notes.html)
| STM32N6xx HAL/LL Drivers      |  v1.1.0             | [release notes](STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Release_Notes.html)
| STM32N6570-DK BSP Drivers     |  v1.1.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/Release_Notes.html)
| NUCLEO-N657X0-Q BSP Drivers   |  v1.0.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6xx_Nucleo/Release_Notes.html)
| BSP Component aps256xx        |  v1.0.6             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/Release_Notes.html)
| BSP Component Common          |  v7.3.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/Common/Release_Notes.html)
| BSP Component mx66uw1g45g     |  v1.1.0             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/Release_Notes.html)
| BSP Component rk050hr18       |  v1.0.1             | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/rk050hr18/Release_Notes.html)
| Fonts Utility                 |  v2.0.3             | [release notes](STM32Cube_FW_N6/Utilities/Fonts/Release_Notes.html)
| lcd Utility                   |  v2.2.0             | [release notes](STM32Cube_FW_N6/Utilities/lcd/Release_Notes.html)

## Available documentation

- README.md
- Doc/Application-Overview.md
- Doc/Boot-Overview.md
- Doc/Build-Options.md
- Doc/Deploy-your-tflite-Model-NUCLEO-N657X0-Q.md
- Doc/Deploy-your-tflite-Model-STM32N6570-DK.md
- Doc/Program-Hex-Files-STM32CubeProgrammer.md

## Update history

### V2.0.0 / April 2025

- Add support of the NUCLEO-N657X0-Q board
- Update miscellaneous drivers
- Update Camera Middleware to v1.4.2
- Miscellaneous refactoring

### V1.0.0 / December 2024

Initial Release