#ifndef _SCRL_
#define _SCRL_

#include <stdint.h>

typedef enum {
  SCRL_LAYER_0,
  SCRL_LAYER_1,
  SCRL_LAYER_NB
} SCRL_Layer;

typedef enum {
  SCRL_RGB565,
  SCRL_ARGB4444,
  SCRL_YUV422,
  SCRL_ARGB8888,/* AKA st ABGR888 */
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

/* Initialize scrl subsystem
 *
 * layers_config : describe layers characteristics
 * screen_config : describe screen characteristic. Only use in UVCL mode
 *
 * return 0 in case of success else negative value is returned
 */
int SCRL_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config);
/* Update layer destination buffer address. After this call all UTIL_LCD_*() that target layer will draw into address
 * buffer.
 *
 * address : new address of layer buffer where drawing APIs will draw.
 * layer : targeted layer
 *
 * return 0 in case of success else negative value is returned
 */
int SCRL_SetAddress_NoReload(void *address, SCRL_Layer layer);
/* Previously given address is now use for display for given layer
 *
 * layer : targeted layer
 *
 * return 0 in case of success else negative value is returned
 */
int SCRL_ReloadLayer(SCRL_Layer layer);
/* Draw screen. This is a no op for LTDC mode.
 *
 * return 0 in case of success else negative value is returned
 */
int SRCL_Update(void);

#endif
