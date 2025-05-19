#include "scrl.h"

#include <assert.h>

#include "stm32n6570_discovery_lcd.h"
#include "stm32_lcd.h"

static SCRL_Layer current_layer;
static int is_layer_rgb888[SCRL_LAYER_NB];
static int is_layer_argb8888[SCRL_LAYER_NB];

static HAL_StatusTypeDef SCRL_ConfigLayer_Layer_default(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  pLayerCfg.WindowX0 = Config->X0;
  pLayerCfg.WindowX1 = Config->X1;
  pLayerCfg.WindowY0 = Config->Y0;
  pLayerCfg.WindowY1 = Config->Y1;
  pLayerCfg.PixelFormat = Config->PixelFormat;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = Config->Address;
  pLayerCfg.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  return HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, LayerIndex);
}

static HAL_StatusTypeDef SCRL_ConfigLayer_Layer_rgb888(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerFlexARGBTypeDef pLayerCfg ={0};

  pLayerCfg.Layer.WindowX0 = Config->X0;
  pLayerCfg.Layer.WindowX1 = Config->X1;
  pLayerCfg.Layer.WindowY0 = Config->Y0;
  pLayerCfg.Layer.WindowY1 = Config->Y1;
  pLayerCfg.Layer.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Layer.Alpha0 = 0;
  pLayerCfg.Layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.Layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.Layer.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.Layer.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Layer.Backcolor.Blue = 0;
  pLayerCfg.Layer.Backcolor.Green = 0;
  pLayerCfg.Layer.Backcolor.Red = 0;

  pLayerCfg.FlexARGB.PixelSize = 3;/*LTDC_ARGB_PIXEL_SIZE_3_BYTES;*/
  pLayerCfg.FlexARGB.AlphaPos = 0;
  pLayerCfg.FlexARGB.RedPos = 0;
  pLayerCfg.FlexARGB.GreenPos = 8;
  pLayerCfg.FlexARGB.BluePos = 16;
  pLayerCfg.FlexARGB.AlphaWidth = 0;
  pLayerCfg.FlexARGB.RedWidth = 8;
  pLayerCfg.FlexARGB.GreenWidth = 8;
  pLayerCfg.FlexARGB.BlueWidth = 8;

  pLayerCfg.ARGBAddress = Config->Address;

  return HAL_LTDC_ConfigLayerFlexARGB(hltdc, &pLayerCfg, LayerIndex);
}

static HAL_StatusTypeDef SCRL_ConfigLayer_Layer_argb888(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerFlexARGBTypeDef pLayerCfg ={0};

  pLayerCfg.Layer.WindowX0 = Config->X0;
  pLayerCfg.Layer.WindowX1 = Config->X1;
  pLayerCfg.Layer.WindowY0 = Config->Y0;
  pLayerCfg.Layer.WindowY1 = Config->Y1;
  pLayerCfg.Layer.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Layer.Alpha0 = 0;
  pLayerCfg.Layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.Layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.Layer.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.Layer.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Layer.Backcolor.Blue = 0;
  pLayerCfg.Layer.Backcolor.Green = 0;
  pLayerCfg.Layer.Backcolor.Red = 0;

  pLayerCfg.FlexARGB.PixelSize = 4;/*LTDC_ARGB_PIXEL_SIZE_4_BYTES;*/
  pLayerCfg.FlexARGB.AlphaPos = 0;
  pLayerCfg.FlexARGB.RedPos = 0;
  pLayerCfg.FlexARGB.GreenPos = 8;
  pLayerCfg.FlexARGB.BluePos = 16;
  pLayerCfg.FlexARGB.AlphaWidth = 0;
  pLayerCfg.FlexARGB.RedWidth = 8;
  pLayerCfg.FlexARGB.GreenWidth = 8;
  pLayerCfg.FlexARGB.BlueWidth = 8;

  pLayerCfg.ARGBAddress = Config->Address;

  return HAL_LTDC_ConfigLayerFlexARGB(hltdc, &pLayerCfg, LayerIndex);
}

static uint32_t pixel_format_from_scrl_format(SCRL_Format fmt)
{
  uint32_t res = LCD_PIXEL_FORMAT_RGB565;

  switch (fmt) {
  case SCRL_RGB565:
    res = LCD_PIXEL_FORMAT_RGB565;
    break;
  case SCRL_ARGB4444:
    res = LCD_PIXEL_FORMAT_ARGB4444;
    break;
  case SCRL_BGR888:
    res = LCD_PIXEL_FORMAT_RGB888;
    break;
  case SCRL_RGB888:
    res = LCD_PIXEL_FORMAT_RGB888;
    break;
  case SCRL_ARGB8888:
    res = LCD_PIXEL_FORMAT_ARGB8888;
    break;
  default:
    assert(0);
  }

  return res;
}

static void fill_lcd_layer_config_from_scrl_layer_config(SCRL_LayerConfig *l, BSP_LCD_LayerConfig_t *layer_config)
{
  layer_config->X0 = l->origin.x;
  layer_config->Y0 = l->origin.y;
  layer_config->X1 = layer_config->X0 + l->size.width;
  layer_config->Y1 = layer_config->Y0 + l->size.height;
  layer_config->PixelFormat = pixel_format_from_scrl_format(l->format);
  layer_config->Address = (uint32_t) l->address;
}

static int SCRL_validate_parameters(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
{
  int i;

  /* First check for null pointer */
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (!layers_config[i])
      return -1;

  /* Also for adresses */
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (!layers_config[i]->address)
      return -1;

  /* SCRL_YUV422 format is not supported */
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (layers_config[i]->format == SCRL_YUV422)
      return -1;

  return 0;
}

static int32_t LTDC_FillRGBRectRgb888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                                      uint32_t Height)
{
  int layer_width;
  uint8_t *pel;
  int w, h;

  layer_width = Lcd_Ctx[Instance].XSize;
  pel = (uint8_t *) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress);
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

static int32_t LTDC_FillRGBRectArgb8888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                                      uint32_t Height)
{
  int layer_width;
  uint8_t *pel;
  int w, h;

  layer_width = Lcd_Ctx[Instance].XSize;
  pel = (uint8_t *) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress);
  pel += (Ypos * layer_width * 4) + Xpos * 4;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      pel[4 * w + 2] = *pData++;
      pel[4 * w + 1] = *pData++;
      pel[4 * w + 0] = *pData++;
      pel[4 * w + 3] = *pData++;
    }
    pel += layer_width + 4;
  }

  return 0;
}

static int32_t LTDC_FillRectRgb888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                                   uint32_t Color)
{
  int layer_width;
  uint8_t r, g, b;
  uint8_t *pel;
  int w, h;

  r = (Color >> 16) & 0xff;
  g = (Color >> 8) & 0xff;
  b = (Color >> 0) & 0xff;

  layer_width = Lcd_Ctx[Instance].XSize;
  pel = (uint8_t *) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress);
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

static int32_t LTDC_FillRectArgb8888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                                     uint32_t Color)
{
  int layer_width;
  uint8_t r, g, b;
  uint8_t *pel;
  int w, h;

  r = (Color >> 16) & 0xff;
  g = (Color >> 8) & 0xff;
  b = (Color >> 0) & 0xff;

  layer_width = Lcd_Ctx[Instance].XSize;
  pel = (uint8_t *) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress);
  pel += (Ypos * layer_width * 4) + Xpos * 4;
  for (h = 0; h < Height; h++) {
    for (w = 0; w < Width; w++) {
      /* FIXME */
      pel[4 * w + 0] = r;
      pel[4 * w + 1] = g;
      pel[4 * w + 2] = b;
      pel[4 * w + 3] = 0xff;
    }
    pel += layer_width * 4;
  }

  return 0;
}

static int32_t LTDC_DrawHLineRgb888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return LTDC_FillRectRgb888(0, Xpos, Ypos, Length , 1, Color);
}

static int32_t LTDC_DrawHLineArgb8888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return LTDC_FillRectArgb8888(0, Xpos, Ypos, Length , 1, Color);
}

static int32_t LTDC_DrawVLineRgb888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return LTDC_FillRectRgb888(0, Xpos, Ypos, 1 , Length, Color);
}

static int32_t LTDC_DrawVLineArgb8888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  return LTDC_FillRectArgb8888(0, Xpos, Ypos, 1 , Length, Color);
}

static int32_t LTDC_SetPixelRgb888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  return LTDC_FillRectRgb888(0, Xpos, Ypos, 1 , 1, Color);
}

static int32_t LTDC_SetPixelArgb8888(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  return LTDC_FillRectArgb8888(0, Xpos, Ypos, 1 , 1, Color);
}

static int32_t LTDC_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  return BSP_LCD_DrawBitmap(Instance, Xpos, Ypos, pBmp);
}

static int32_t LTDC_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width,
                                uint32_t Height)
{
  if (is_layer_rgb888[current_layer])
    return LTDC_FillRGBRectRgb888(Instance, Xpos, Ypos, pData, Width, Height);
  if (is_layer_argb8888[current_layer])
    return LTDC_FillRGBRectArgb8888(Instance, Xpos, Ypos, pData, Width, Height);

  return BSP_LCD_FillRGBRect(Instance, Xpos, Ypos, pData, Width, Height);
}

static int32_t LTDC_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  if (is_layer_rgb888[current_layer])
    return LTDC_DrawHLineRgb888(Instance, Xpos, Ypos, Length, Color);
  if (is_layer_argb8888[current_layer])
    return LTDC_DrawHLineArgb8888(Instance, Xpos, Ypos, Length, Color);

  return BSP_LCD_DrawHLine(Instance, Xpos, Ypos, Length, Color);
}

static int32_t LTDC_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  if (is_layer_rgb888[current_layer])
    return LTDC_DrawVLineRgb888(Instance, Xpos, Ypos, Length, Color);
  if (is_layer_argb8888[current_layer])
    return LTDC_DrawVLineArgb8888(Instance, Xpos, Ypos, Length, Color);

  return BSP_LCD_DrawVLine(Instance, Xpos, Ypos, Length, Color);
}

static int32_t LTDC_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height,
                            uint32_t Color)
{
  if (is_layer_rgb888[current_layer])
    return LTDC_FillRectRgb888(Instance, Xpos, Ypos, Width, Height, Color);
  if (is_layer_argb8888[current_layer])
    return LTDC_FillRectArgb8888(Instance, Xpos, Ypos, Width, Height, Color);

  return BSP_LCD_FillRect(Instance, Xpos, Ypos, Width, Height, Color);
}

static int32_t LTDC_GetPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  return BSP_LCD_ReadPixel(Instance, Xpos, Ypos, Color);
}

static int32_t LTDC_SetPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  if (is_layer_rgb888[current_layer])
    return LTDC_SetPixelRgb888(Instance, Xpos, Ypos, Color);
  if (is_layer_argb8888[current_layer])
    return LTDC_SetPixelArgb8888(Instance, Xpos, Ypos, Color);

  return BSP_LCD_WritePixel(Instance, Xpos, Ypos, Color);
}

static int32_t LTDC_GetXSize(uint32_t Instance, uint32_t *YSize)
{
  return BSP_LCD_GetXSize(Instance, YSize);
}

static int32_t LTDC_GetYSize(uint32_t Instance, uint32_t *YSize)
{
  return BSP_LCD_GetYSize(Instance, YSize);
}

static int32_t LTDC_SetLayer(uint32_t Instance, uint32_t LayerIndex)
{
  int32_t res;

  current_layer = LayerIndex;
  res = BSP_LCD_SetActiveLayer(Instance, LayerIndex);

  /* Fix wrong BppFactor/PixelFormat decision when in flexible format */
  if (is_layer_rgb888[LayerIndex]) {
    Lcd_Ctx[Instance].BppFactor = 3;
    Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_RGB888;
  }

  return res;
}

static int32_t LTDC_GetFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  return BSP_LCD_GetPixelFormat(Instance, PixelFormat);
}

static const LCD_UTILS_Drv_t Dummy_Driver = {
  .DrawBitmap = LTDC_DrawBitmap,
  .FillRGBRect = LTDC_FillRGBRect,
  .DrawHLine = LTDC_DrawHLine,
  .DrawVLine = LTDC_DrawVLine,
  .FillRect = LTDC_FillRect,
  .GetPixel = LTDC_GetPixel,
  .SetPixel = LTDC_SetPixel,
  .GetXSize = LTDC_GetXSize,
  .GetYSize = LTDC_GetYSize,
  .SetLayer = LTDC_SetLayer,
  .GetFormat = LTDC_GetFormat,
};

int SCRL_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
{
  BSP_LCD_LayerConfig_t LayerConfig = {0};
  int ret;

  ret = SCRL_validate_parameters(layers_config, screen_config);
  if (ret)
    return ret;

  current_layer = SCRL_LAYER_0;

  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

  fill_lcd_layer_config_from_scrl_layer_config(layers_config[SCRL_LAYER_0], &LayerConfig);
  is_layer_rgb888[SCRL_LAYER_0] = layers_config[SCRL_LAYER_0]->format == SCRL_RGB888;
  is_layer_argb8888[SCRL_LAYER_0] = layers_config[SCRL_LAYER_0]->format == SCRL_ARGB8888;
  BSP_LCD_ConfigLayer(0, LTDC_LAYER_1, &LayerConfig);

  fill_lcd_layer_config_from_scrl_layer_config(layers_config[SCRL_LAYER_1], &LayerConfig);
  is_layer_rgb888[SCRL_LAYER_1] = layers_config[SCRL_LAYER_1]->format == SCRL_RGB888;
  is_layer_argb8888[SCRL_LAYER_1] = layers_config[SCRL_LAYER_1]->format == SCRL_ARGB8888;
  BSP_LCD_ConfigLayer(0, LTDC_LAYER_2, &LayerConfig);

  UTIL_LCD_SetFuncDriver(&Dummy_Driver);

  return 0;
}

int SCRL_SetAddress_NoReload(void *address, SCRL_Layer layer)
{
  return HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, (uint32_t) address, layer);
}

int SCRL_ReloadLayer(SCRL_Layer layer)
{
  return HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, layer);
}

int SRCL_Update(void)
{
  /* Nothing to do */
  return 0;
}

HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  assert(LayerIndex < SCRL_LAYER_NB);

  if (is_layer_rgb888[LayerIndex])
    return SCRL_ConfigLayer_Layer_rgb888(hltdc, LayerIndex, Config);
  else if (is_layer_argb8888[LayerIndex])
    return SCRL_ConfigLayer_Layer_argb888(hltdc, LayerIndex, Config);
  else
    return SCRL_ConfigLayer_Layer_default(hltdc, LayerIndex, Config);
}
