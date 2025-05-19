#include "scrl_common.h"

#include <assert.h>
#include <stdio.h>

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

/* Store current DMA2D_HandleTypeDef instance so we can propagate to irq handler */
static DMA2D_HandleTypeDef *dma2d_current;

static uint32_t cvt_lcd_pixel_format(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
    return LCD_PIXEL_FORMAT_RGB565;
  case SCRL_ARGB4444:
    return LCD_PIXEL_FORMAT_ARGB4444;
  case SCRL_BGR888:
    return LCD_PIXEL_FORMAT_RGB888;
  case SCRL_RGB888:
    return LCD_PIXEL_FORMAT_RGB888;
  default:
    assert(0);
  }

  return LCD_PIXEL_FORMAT_RGB565;
}

static int32_t SPI_FillRGBRectBpp2(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                                   uint32_t Height)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint16_t *pel;
  int w, h;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = (uint16_t *) ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width) + Xpos;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      pel[w] = *(uint16_t *)pData;
      pData += 2;
    }
    pel += layer_width;
  }

  return 0;
}

static int32_t SPI_DrawHLineBpp2(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint16_t *pel;
  int i;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = (uint16_t *) ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width) + Xpos;
  for (i = 0; i < Length; i++)
    *pel++ = Color;

  return 0;
}

static int32_t SPI_DrawVLineBpp2(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint16_t *pel;
  int i;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = (uint16_t *) ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width) + Xpos;
  for (i = 0; i < Length; i++) {
    *pel = Color;
    pel += layer_width;
  }

  return 0;
}

static int32_t SPI_FillRectBpp2(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                           uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint16_t *pel;
  int w, h;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = (uint16_t *) ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width) + Xpos;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      pel[w] = Color;
    }
    pel += layer_width;
  }

  return 0;
}

static int32_t SPI_SetPixelBpp2(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint16_t *pel;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = (uint16_t *) ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width) + Xpos;
  *pel = Color;

  return 0;
}

static const LCD_UTILS_Drv_t Driver_Bpp2 = {
  .FillRGBRect = SPI_FillRGBRectBpp2,
  .DrawHLine = SPI_DrawHLineBpp2,
  .DrawVLine = SPI_DrawVLineBpp2,
  .FillRect = SPI_FillRectBpp2,
  .SetPixel = SPI_SetPixelBpp2,
};

static int32_t SPI_FillRGBRectBpp3(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                                   uint32_t Height)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint8_t *pel;
  int w, h;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width * 3) + Xpos * 3;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      pel[3 * w + 0] = *pData++;
      pel[3 * w + 1] = *pData++;
      pel[3 * w + 2] = *pData++;
    }
    pel += layer_width + 3;
  }

  return 0;
}

static int32_t SPI_FillRectBpp3(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                           uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;
  int layer_width;
  uint8_t r, g, b;
  uint8_t *pel;
  int w, h;

  r = (Color >> 16) & 0xff;
  g = (Color >> 8) & 0xff;
  b = (Color >> 0) & 0xff;

  assert(ctx);

  layer_width = ctx->layers[ctx->layer].size.width;
  pel = ctx->layers[ctx->layer].address;
  pel += (Ypos * layer_width * 3) + Xpos * 3;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      /* FIXME */
      pel[3 * w + 0] = b;
      pel[3 * w + 1] = g;
      pel[3 * w + 2] = r;
    }
    pel += layer_width * 3;
  }

  return 0;
}

static int32_t SPI_DrawHLineBpp3(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return SPI_FillRectBpp3(Instance, Xpos, Ypos, Length , 1, Color);
}

static int32_t SPI_DrawVLineBpp3(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return SPI_FillRectBpp3(Instance, Xpos, Ypos, 1 , Length, Color);
}

static int32_t SPI_SetPixelBpp3(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  return SPI_FillRectBpp3(Instance, Xpos, Ypos, 1 , 1, Color);
}

static const LCD_UTILS_Drv_t Driver_Bpp3 = {
  .FillRGBRect = SPI_FillRGBRectBpp3,
  .DrawHLine = SPI_DrawHLineBpp3,
  .DrawVLine = SPI_DrawVLineBpp3,
  .SetPixel = SPI_SetPixelBpp3,
  .FillRect = SPI_FillRectBpp3,
};

static int32_t CMN_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  assert(0);

  return 0;
}

static int32_t CMN_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                              uint32_t Height)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx->layer_driver[ctx->layer]->FillRGBRect);
  return ctx->layer_driver[ctx->layer]->FillRGBRect(Instance, Xpos, Ypos, pData, Width, Height);
}

static int32_t CMN_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx->layer_driver[ctx->layer]->DrawHLine);
  return ctx->layer_driver[ctx->layer]->DrawHLine(Instance, Xpos, Ypos, Length, Color);
}

static int32_t CMN_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx->layer_driver[ctx->layer]->DrawVLine);
  return ctx->layer_driver[ctx->layer]->DrawVLine(Instance, Xpos, Ypos, Length, Color);
}

static int32_t CMN_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                           uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx->layer_driver[ctx->layer]->FillRect);
  return ctx->layer_driver[ctx->layer]->FillRect(Instance, Xpos, Ypos, Width, Height, Color);
}

static int32_t CMN_GetPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  assert(0);

  return 0;
}

static int32_t CMN_SetPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx->layer_driver[ctx->layer]->SetPixel);
  return ctx->layer_driver[ctx->layer]->SetPixel(Instance, Xpos, Ypos, Color);
}

static int32_t CMN_GetXSize(uint32_t Instance, uint32_t *XSize)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  *XSize = ctx ? ctx->layers[ctx->layer].size.width : 0;

  return 0;
}

static int32_t CMN_GetYSize(uint32_t Instance, uint32_t *YSize)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  *YSize = ctx ? ctx->layers[ctx->layer].size.height : 0;

  return 0;
}

static int32_t CMN_SetLayer(uint32_t Instance, uint32_t LayerIndex)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  assert(ctx);

  ctx->layer = LayerIndex;

  return 0;
}

static int32_t CMN_GetFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  struct scrl_common_ctx *ctx = (struct scrl_common_ctx *) Instance;

  *PixelFormat = cvt_lcd_pixel_format(ctx ? ctx->layers[ctx->layer].format : SCRL_RGB565);

  return 0;
}

const LCD_UTILS_Drv_t Common_Driver = {
  .DrawBitmap = CMN_DrawBitmap,
  .FillRGBRect = CMN_FillRGBRect,
  .DrawHLine = CMN_DrawHLine,
  .DrawVLine = CMN_DrawVLine,
  .FillRect = CMN_FillRect,
  .GetPixel = CMN_GetPixel,
  .SetPixel = CMN_SetPixel,
  .GetXSize = CMN_GetXSize,
  .GetYSize = CMN_GetYSize,
  .SetLayer = CMN_SetLayer,
  .GetFormat = CMN_GetFormat,
};

static int get_bpp(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
    return 2;
  case SCRL_ARGB4444:
    return 2;
  case SCRL_YUV422:
    return 2;
  case SCRL_BGR888:
    return 3;
  case SCRL_RGB888:
    return 3;
  default:
    assert(0);
  }

  return 2;
}

static uint32_t cvt_dma2d_pixel_format_output(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
    return DMA2D_OUTPUT_RGB565;
  case SCRL_YUV422:
    /* We will do sw convertion from RGB565 to YUV422 */
    return DMA2D_OUTPUT_RGB565;
  default:
    assert(0);
  }

  return DMA2D_OUTPUT_RGB565;
}

static uint32_t cvt_dma2d_pixel_format_input(SCRL_Format fmt)
{
  switch (fmt) {
    case SCRL_RGB565:
      return DMA2D_INPUT_RGB565;
    case SCRL_ARGB4444:
      return DMA2D_INPUT_ARGB4444;
    case SCRL_BGR888:
      return DMA2D_INPUT_RGB888;
    case SCRL_RGB888:
      return DMA2D_INPUT_RGB888;
    default:
      assert(0);
  }

  return DMA2D_INPUT_RGB565;
}

static uint32_t cvt_dma2d_pixel_format(SCRL_Format fmt, int is_output)
{
  return is_output ? cvt_dma2d_pixel_format_output(fmt) :
                     cvt_dma2d_pixel_format_input(fmt);
}

static int get_layer_buffer_size(struct scrl_common_ctx *ctx, SCRL_Layer layer)
{
  return ctx->layers[layer].size.width * ctx->layers[layer].size.height * get_bpp(ctx->layers[layer].format);
}

static int get_screen_buffer_size(struct scrl_common_ctx *ctx)
{
  return ctx->screen.size.width * ctx->screen.size.height * get_bpp(ctx->screen.format);
}

static void SCRC_dma2d_cb(DMA2D_HandleTypeDef *hdma2d)
{
  struct scrl_common_ctx *ctx = container_of(hdma2d, struct scrl_common_ctx, hdma2d);
  int ret;

  HAL_NVIC_DisableIRQ(DMA2D_IRQn);
  ret = HAL_DMA2D_DeInit(&ctx->hdma2d);
  assert(ret == HAL_OK);
  dma2d_current = NULL;

  SCRC_Composition_Done_Callback(ctx);
}

static void SCRC_dma2d_error_cb(DMA2D_HandleTypeDef *hdma2d)
{
  assert(0);
}

int SCRC_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config,
              struct scrl_common_ctx *ctx)
{
  int i;

  ctx->screen = *screen_config;
  for (i = 0; i < SCRL_LAYER_NB; i++) {
    ctx->layers[i] = *layers_config[i];
    ctx->display_address[i] = ctx->layers[i].address;
    switch (get_bpp(ctx->layers[i].format)) {
    case 2:
      ctx->layer_driver[i] = &Driver_Bpp2;
      break;
    case 3:
      ctx->layer_driver[i] = &Driver_Bpp3;
      break;
    default:
      assert(0);
    }
  }
  ctx->layer = SCRL_LAYER_0;

  __HAL_RCC_DMA2D_CLK_ENABLE();
  __HAL_RCC_DMA2D_CLK_SLEEP_ENABLE();
  __HAL_RCC_DMA2D_FORCE_RESET();
  __HAL_RCC_DMA2D_RELEASE_RESET();

  return 0;
}

void SCRC_Composition_Start(struct scrl_common_ctx *ctx, int is_output_byte_swap_enable)
{
  uint32_t src_buffer[2];
  uint32_t dst_buffer;
  int ret;
  int i;

  /* both layers have same size and origin and fit fully in screen */

  /* copy display buffers */
  for (i = 0; i < 2; i++)
    src_buffer[i] = (uint32_t) ctx->display_address[i];
  dst_buffer = (uint32_t) ctx->screen.address;
  dst_buffer += ctx->layers[0].origin.y * ctx->screen.size.width * get_bpp(ctx->screen.format);
  dst_buffer += ctx->layers[0].origin.x * get_bpp(ctx->screen.format);

  /* clean source buffers */
  SCB_CleanDCache_by_Addr((void *) src_buffer[0], get_layer_buffer_size(ctx, SCRL_LAYER_0));
  SCB_CleanDCache_by_Addr((void *) src_buffer[1], get_layer_buffer_size(ctx, SCRL_LAYER_1));

  /* invalidate destination buffer */
  SCB_InvalidateDCache_by_Addr((void *) ctx->screen.address, get_screen_buffer_size(ctx));

  dma2d_current = &ctx->hdma2d;
  ctx->hdma2d.Instance             = DMA2D;
  ctx->hdma2d.Init.Mode            = DMA2D_M2M_BLEND;
  ctx->hdma2d.Init.ColorMode       = cvt_dma2d_pixel_format(ctx->screen.format, 1);
  ctx->hdma2d.Init.OutputOffset    = ctx->screen.size.width - ctx->layers[0].size.width;
  ctx->hdma2d.Init.AlphaInverted   = DMA2D_REGULAR_ALPHA;
  ctx->hdma2d.Init.RedBlueSwap     = DMA2D_RB_REGULAR;
  ctx->hdma2d.Init.LineOffsetMode  = DMA2D_LOM_PIXELS;
  ctx->hdma2d.Init.BytesSwap       = is_output_byte_swap_enable ? DMA2D_BYTES_SWAP : DMA2D_BYTES_REGULAR;
  ret = HAL_DMA2D_Init(&ctx->hdma2d);
  assert(ret == HAL_OK);

  ctx->hdma2d.LayerCfg[0].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
  ctx->hdma2d.LayerCfg[0].InputAlpha     = 0xff;
  ctx->hdma2d.LayerCfg[0].InputColorMode = cvt_dma2d_pixel_format(ctx->layers[0].format, 0);
  ctx->hdma2d.LayerCfg[0].InputOffset    = 0;
  ctx->hdma2d.LayerCfg[0].AlphaInverted  = DMA2D_REGULAR_ALPHA;
  ctx->hdma2d.LayerCfg[0].RedBlueSwap    = ctx->layers[0].format == SCRL_RGB888 ? DMA2D_RB_SWAP : DMA2D_RB_REGULAR;
  ctx->hdma2d.LayerCfg[1].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
  ctx->hdma2d.LayerCfg[1].InputAlpha     = 0xFF;
  ctx->hdma2d.LayerCfg[1].InputColorMode = cvt_dma2d_pixel_format(ctx->layers[1].format, 0);
  ctx->hdma2d.LayerCfg[1].InputOffset    = 0;
  ctx->hdma2d.LayerCfg[1].AlphaInverted  = DMA2D_REGULAR_ALPHA;
  ctx->hdma2d.LayerCfg[1].RedBlueSwap    = ctx->layers[1].format == SCRL_RGB888 ? DMA2D_RB_SWAP : DMA2D_RB_REGULAR;
  ret = HAL_DMA2D_ConfigLayer(&ctx->hdma2d, 0);
  assert(ret == HAL_OK);
  ret = HAL_DMA2D_ConfigLayer(&ctx->hdma2d, 1);
  assert(ret == HAL_OK);

  ctx->hdma2d.XferCpltCallback = SCRC_dma2d_cb;
  ctx->hdma2d.XferErrorCallback = SCRC_dma2d_error_cb;
  HAL_NVIC_EnableIRQ(DMA2D_IRQn);
  ret = HAL_DMA2D_BlendingStart_IT(&ctx->hdma2d, src_buffer[1], src_buffer[0], dst_buffer, ctx->layers[0].size.width,
                                   ctx->layers[0].size.height);
  assert(ret == HAL_OK);
}

void DMA2D_IRQHandler(void)
{
  HAL_DMA2D_IRQHandler(dma2d_current);
}
