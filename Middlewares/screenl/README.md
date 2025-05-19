# Screen library

## Purpose

This document provides an overview of the Screen Library (aka screenl).

Screenl allows application to abstract underlying display. It's compatible with `stm32_lcd.h` display API's.
It has a very simple API and can be configured at compile time.

## Build configuration

You can configure following items at build time :

* Type of display : Can be either LTDC, UVCL or SPI
* Rtos support : Can be either THREADX, FREERTOS or NONE. These is only for UVCL and SPI display.

## APIs

### Initialization

```C
typedef enum {
  SCRL_LAYER_0,
  SCRL_LAYER_1,
  SCRL_LAYER_NB
} SCRL_Layer;

typedef enum {
  SCRL_RGB565,
  SCRL_ARGB4444,
  SCRL_YUV422,
  SCRL_RGB888, /* AKA st BGR888 */
  SCRL_BGR888, /* AKA st RGB888 */
  SCRL_FORMAT_NB
} SCRL_Format;

typedef struct {
  uint16_t x;
  uint16_t y;
} SCRL_Point;

typedef struct {
  uint16_t width;
  uint16_t height;
} SCRL_Size;

typedef struct {
  SCRL_Point origin;
  SCRL_Size size;
  SCRL_Format format;
  void *address;
} SCRL_LayerConfig;

typedef struct {
  SCRL_Size size;
  SCRL_Format format;
  void *address;
  uint16_t fps;
} SCRL_ScreenConfig;
```

```C
/* layers_config : describe layers characteristics
 * screen_config : describe screen characteristic. Only use in UVCL mode
 *
 * return 0 in case of success else negative value is returned
 */
int SCRL_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config);
```

### draw into a layer

For that draw into buffer directly of by using UTIL_LCD_*() API's.
Once you have finish to draw into one or multiple layers you must call SRCL_Update().

```C
int SRCL_Update(void);
```

### update display content without tearing

If you want to avoid tearing while drawing you must use double buffering and use following sequence.

- Call SCRL_SetAddress_NoReload() with next buffer to use for given layer.
- Draw directly into buffer or use UTIL_LCD_*() API's.
- Call SCRL_ReloadLayer() to commit your buffer change().
- Call SRCL_Update() to refresh screen

```C
int SCRL_SetAddress_NoReload(void *address, SCRL_Layer layer);
int SCRL_ReloadLayer(SCRL_Layer layer);
```
