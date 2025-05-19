#ifndef _SCRL_COMMON_
#define _SCRL_COMMON_

#include "scrl.h"
#include "stm32n6xx_hal.h"
#include "stm32_lcd.h"

struct scrl_common_ctx {
  SCRL_LayerConfig layers[SCRL_LAYER_NB];
  SCRL_ScreenConfig screen;
  DMA2D_HandleTypeDef hdma2d;
  SCRL_Layer layer;
  const LCD_UTILS_Drv_t *layer_driver[SCRL_LAYER_NB];
  void *display_address[SCRL_LAYER_NB];
};

extern const LCD_UTILS_Drv_t Common_Driver;

int SCRC_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config,
              struct scrl_common_ctx *ctx);
void SCRC_Composition_Start(struct scrl_common_ctx *ctx, int is_output_byte_swap_enable);
/* To be implemented by user */
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx);

#endif
