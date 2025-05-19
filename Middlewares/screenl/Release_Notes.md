# Release Notes for STM32N6 Screen Library (aka screenl)

## Purpose

Screenl purpose is to abstract display for application. It currently supports LDTC or Usb Uvc display.

## Key Features

- Use either LDTC, Usb Uvc or SPI as display.
- Can be use either with FreeRTOS, threadx or without any rtos

## Software components

| Name                          | Version             | Release notes
|-----                          | -------             | -------------
| uvcl                          | v2.0.2              | [release notes](uvcl/Release_Notes.md)

## Update history

### V2.1.1 / May 2025

- Update release notes format

### V2.1.0 / April 2025

- For SPI and UVCL mode remove those constraints
  - Layers can have a size different that one of the screen
  - Layers can have an origin different of (0, 0)

### V2.0.0 / March 2025

- Add SCRL_YUV422, SCRL_RGB888, SCRL_BGR888 and SCRL_ARGB8888 new formats
- Add SPI mode with X-NUCLEO-GFX01M2 support
- Add FreeRTOS support
- Fix missing CLK_SLEEP_ENABLE for miscellaneous IPs

### V1.0.0 / December 2024

Initial Version
