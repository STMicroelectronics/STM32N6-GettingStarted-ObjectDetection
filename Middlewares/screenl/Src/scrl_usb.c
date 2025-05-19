#include "scrl.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"
#include "stm32_lcd.h"
#ifdef SCR_LIB_USE_THREADX
#include "tx_api.h"
#endif
#ifdef SCR_LIB_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif
#include "uvcl.h"
#include "scrl_common.h"

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define CLAMP(v, v_min, v_max) do { \
  v = v < v_min ? v_min : v; \
  v = v > v_max ? v_max : v; \
} while (0)

#define RGB_2_Y(r, g, b, y) do { \
  y = SCRU_RED_Y_LUT[r] + SCRU_GREEN_Y_LUT[g] + SCRU_BLUE_Y_LUT[b]; \
  CLAMP(y, 0, 255); \
} while(0)

#define RGB_2_CR(r, g, b, cr) do { \
  cr = SCRU_BLUE_CB_RED_CR_LUT[r] + SCRU_GREEN_CR_LUT[g] + SCRU_BLUE_CR_LUT[b] + 128; \
  CLAMP(cr, 0, 255); \
} while(0)

#define RGB_2_CB(r, g, b, cb) do { \
  cb = SCRU_RED_CB_LUT[r] + SCRU_GREEN_CB_LUT[g] + SCRU_BLUE_CB_RED_CR_LUT[b] + 128; \
  CLAMP(cb, 0, 255); \
} while(0)

struct scrl_usb_ctx {
  struct scrl_common_ctx common;
  struct uvcl_callbacks usb_cbs;
  int is_screen_ready_to_update;
#ifdef SCR_LIB_USE_THREADX
  TX_SEMAPHORE update_sem;
  TX_SEMAPHORE dma2d_sem;
  TX_THREAD update_thread;
#endif
#ifdef SCR_LIB_USE_FREERTOS
  SemaphoreHandle_t update_sem;
  StaticSemaphore_t update_sem_buffer;
  SemaphoreHandle_t dma2d_sem;
  StaticSemaphore_t dma2d_sem_buffer;
  StaticTask_t update_thread;
#endif
};

static int32_t SCRU_RED_Y_LUT[256];
static int32_t SCRU_RED_CB_LUT[256];
static int32_t SCRU_BLUE_CB_RED_CR_LUT[256];
static int32_t SCRU_GREEN_Y_LUT[256];
static int32_t SCRU_GREEN_CR_LUT[256];
static int32_t SCRU_GREEN_CB_LUT[256];
static int32_t SCRU_BLUE_Y_LUT[256];
static int32_t SCRU_BLUE_CR_LUT[256];

static struct scrl_usb_ctx scrl_ctx;
#ifdef SCR_LIB_USE_THREADX
static uint8_t update_thread_stack[4096];
#endif
#ifdef SCR_LIB_USE_FREERTOS
static StackType_t update_thread_stack[configMINIMAL_STACK_SIZE];
#endif

static int cvt_uvcl_payload_format(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
    return UVCL_PAYLOAD_FB_RGB565;
  case SCRL_YUV422:
    return UVCL_PAYLOAD_UNCOMPRESSED_YUY2;
  default:
    assert(0);
  }

  return SCRL_RGB565;
}

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

static int get_screen_buffer_size(struct scrl_common_ctx *ctx)
{
  return ctx->screen.size.width * ctx->screen.size.height * get_bpp(ctx->screen.format);
}

static void SCRU_cvt_dual_pel_rgb_to_yuv(uint8_t *r, uint8_t *g, uint8_t *b, int32_t *y, int32_t *cb, int32_t *cr)
{
  uint8_t red, green, blue;

  RGB_2_Y(r[0], g[0], b[0], y[0]);
  RGB_2_Y(r[1], g[1], b[1], y[1]);

  red = (r[0] + r[1] + 1) / 2;
  green = (g[0] + g[1] + 1) / 2;
  blue = (b[0] + b[1] + 1) / 2;

  RGB_2_CR(red, green, blue, cr[0]);
  RGB_2_CB(red, green, blue, cb[0]);
}

static void SCRU_cvt_rgb565_to_yuv422_internal(uint8_t *p_dst, uint8_t *p_src, int width, int height)
{
  uint32_t *p_src_dual_rgb565 = (uint32_t *)p_src;
  int32_t luma[2];
  int32_t cb, cr;
  uint8_t b[2];
  uint8_t g[2];
  uint8_t r[2];
  int x, y;

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x += 2)
    {
      uint32_t p = p_src_dual_rgb565[x / 2];

      b[0] = (p >> 0) & 0x1f;
      b[0] = (b[0] << 3) | (b[0] >> 2);
      g[0] = (p >> 5) & 0x3f;
      g[0] = (g[0] << 2) | (g[0] >> 4);
      r[0] = (p >> 11) & 0x1f;
      r[0] = (r[0] << 3) | (r[0] >> 2);
      b[1] = (p >> 16) & 0x1f;
      b[1] = (b[1] << 3) | (b[1] >> 2);
      g[1] = (p >> 21) & 0x3f;
      g[1] = (g[1] << 2) | (g[1] >> 4);
      r[1] = (p >> 27) & 0x1f;
      r[1] = (r[1] << 3) | (r[1] >> 2);

      SCRU_cvt_dual_pel_rgb_to_yuv(r, g, b, luma, &cb, &cr);
      *p_dst++ = luma[0];
      *p_dst++ = cb;
      *p_dst++ = luma[1];
      *p_dst++ = cr;
    }
    p_src_dual_rgb565 += width / 2;
  }
}

static void SCRU_cvt_rgb565_to_yuv422(struct scrl_common_ctx  *ctx_common)
{
  /* only convert layers area */
  int stride = ctx_common->screen.size.width * 2;
  int height = ctx_common->layers[0].size.height;
  uint8_t *buffer = ctx_common->screen.address;
  int width = ctx_common->layers[0].size.width;
  int y = ctx_common->layers[0].origin.y;
  int x = ctx_common->layers[0].origin.x;
  int r;

  buffer += y * stride + x * 2;

  for (r = 0; r < height; r++) {
    SCRU_cvt_rgb565_to_yuv422_internal(buffer, buffer, width, 1);
    buffer += stride;
  }
}

static void SCRU_uvcl_show_frame(struct scrl_usb_ctx *ctx)
{
  int ret;

  if (ctx->common.screen.format == SCRL_YUV422)
    SCRU_cvt_rgb565_to_yuv422(&ctx->common);
  ret = UVCL_ShowFrame(ctx->common.screen.address, get_screen_buffer_size(&ctx->common));
  if (ret)
    ctx->is_screen_ready_to_update = 1;
}

#ifdef SCR_LIB_USE_THREADX
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_usb_ctx *ctx = container_of(ctx_common, struct scrl_usb_ctx, common);
  int ret;

  ret = tx_semaphore_put(&ctx->dma2d_sem);
  assert(ret == 0);
}
#elif defined(SCR_LIB_USE_FREERTOS)
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_usb_ctx *ctx = container_of(ctx_common, struct scrl_usb_ctx, common);
  int ret;

  ret = xSemaphoreGive(ctx->dma2d_sem);
  assert(ret == pdTRUE);
}
#else
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_usb_ctx *ctx = container_of(ctx_common, struct scrl_usb_ctx, common);

  SCRU_uvcl_show_frame(ctx);
}
#endif

#ifdef SCR_LIB_USE_THREADX
static void update_thread_fct(ULONG arg)
{
  struct scrl_usb_ctx *ctx = (struct scrl_usb_ctx *) arg;
  int ret;

  while (1)
  {
    ret = tx_semaphore_get(&ctx->update_sem, TX_WAIT_FOREVER);
    assert(ret == 0);

    if (!ctx->is_screen_ready_to_update)
      continue;

    ctx->is_screen_ready_to_update = 0;
    SCRC_Composition_Start(&ctx->common, 0);
    ret = tx_semaphore_get(&ctx->dma2d_sem, TX_WAIT_FOREVER);
    assert(ret == 0);
    SCRU_uvcl_show_frame(ctx);
  }
}
#elif defined(SCR_LIB_USE_FREERTOS)
static void update_thread_fct(void *arg)
{
  struct scrl_usb_ctx *ctx = (struct scrl_usb_ctx *) arg;
  int ret;

  while (1)
  {
    ret = xSemaphoreTake(ctx->update_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    if (!ctx->is_screen_ready_to_update)
      continue;

    ctx->is_screen_ready_to_update = 0;
    SCRC_Composition_Start(&ctx->common, 0);
    ret = xSemaphoreTake(ctx->dma2d_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    SCRU_uvcl_show_frame(ctx);
  }
}
#else
static int start_composition(struct scrl_usb_ctx *ctx)
{
  if (!ctx->is_screen_ready_to_update)
      return 0;

  ctx->is_screen_ready_to_update = 0;
  SCRC_Composition_Start(&ctx->common, 0);

  return 0;
}
#endif

static void usb_frame_release_cb(struct uvcl_callbacks *cbs, void *frame)
{
  struct scrl_usb_ctx *ctx = container_of(cbs, struct scrl_usb_ctx, usb_cbs);

  ctx->is_screen_ready_to_update = 1;
}

static void SCRU_usb_init(PCD_TypeDef *pcd_instance, struct scrl_usb_ctx *ctx)
{
  UVCL_Conf_t conf = { 0 };
  int ret;

  conf.width = ctx->common.screen.size.width;
  conf.height = ctx->common.screen.size.height;
  conf.fps = ctx->common.screen.fps;
  conf.payload_type = cvt_uvcl_payload_format(ctx->common.screen.format);
  conf.is_immediate_mode = 1;

  ret = UVCL_Init(pcd_instance, &conf, &ctx->usb_cbs);
  assert(ret == 0);
}

static int is_layer_format_valid(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
  case SCRL_ARGB4444:
  case SCRL_BGR888:
  case SCRL_RGB888:
    return 1;
  default:
    return 0;
  }
}

static int is_output_format_valid(SCRL_Format fmt)
{
  switch (fmt) {
  case SCRL_RGB565:
  case SCRL_YUV422:
    return 1;
  default:
    return 0;
  }
}

static int SCRU_validate_parameters(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
{
  int i;

  /* First check for null pointer */
  if (!screen_config)
    return -1;
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (!layers_config[i])
      return -1;

  /* Also for adresses */
  if (!screen_config->address)
    return -1;
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (!layers_config[i]->address)
      return -1;

  /* All layers must have the same size ... */
  for (i = 1; i < SCRL_LAYER_NB; i++) {
    if (layers_config[i]->size.width != layers_config[0]->size.width)
      return -1;
    if (layers_config[i]->size.height != layers_config[0]->size.height)
      return -1;
  }

  /* ... and same origin */
  for (i = 1; i < SCRL_LAYER_NB; i++) {
    if (layers_config[i]->origin.x != layers_config[0]->origin.x)
      return -1;
    if (layers_config[i]->origin.y != layers_config[0]->origin.y)
      return -1;
  }

  /* layers must fully fit in screen. Only check with layer0 since layers have same size and origin */
  if (layers_config[0]->size.width + layers_config[0]->origin.x > screen_config->size.width)
    return -1;
  if (layers_config[0]->size.height + layers_config[0]->origin.y > screen_config->size.height)
    return -1;

  /* check input format */
  for (i = 0; i < SCRL_LAYER_NB; i++)
    if (!is_layer_format_valid(layers_config[i]->format))
      return -1;

  /* check output format */
  if (!is_output_format_valid(screen_config->format))
    return -1;

  return 0;
}

#ifdef SCR_LIB_USE_THREADX
static void SCRL_Init_threadx(struct scrl_usb_ctx *ctx)
{
  const UINT update_thread_priority = TX_MAX_PRIORITIES / 2 + 2;
  const ULONG time_slice = 10;
  int ret;

  ret = tx_semaphore_create(&ctx->update_sem, NULL, 0);
  assert(ret == 0);
  ret = tx_semaphore_create(&ctx->dma2d_sem, NULL, 0);
  assert(ret == 0);
  ret = tx_thread_create(&ctx->update_thread, "ud", update_thread_fct, (ULONG) ctx, update_thread_stack,
                         sizeof(update_thread_stack), update_thread_priority, update_thread_priority, time_slice,
                         TX_AUTO_START);
  assert(ret == TX_SUCCESS);
}
#elif defined(SCR_LIB_USE_FREERTOS)
static void SCRL_Init_freertos(struct scrl_usb_ctx *ctx)
{
  const int update_thread_priority = tskIDLE_PRIORITY + 20 - 2;
  TaskHandle_t hdl;

  ctx->update_sem = xSemaphoreCreateCountingStatic(1, 0, &ctx->update_sem_buffer);
  assert(ctx->update_sem);
  ctx->dma2d_sem = xSemaphoreCreateCountingStatic(1, 0, &ctx->dma2d_sem_buffer);
  assert(ctx->dma2d_sem);
  hdl = xTaskCreateStatic(update_thread_fct, "ud", configMINIMAL_STACK_SIZE, ctx, update_thread_priority,
                          update_thread_stack, &ctx->update_thread);
  assert(hdl);
}
#endif

static void SCRL_Init_Lut()
{
  int i;

  for (i = 0; i <= 255; i++)
  {
    SCRU_RED_Y_LUT[i]           = ((  ((int32_t) ((0.299 )  * (1L << 16)))  * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_GREEN_Y_LUT[i]         = ((  ((int32_t) ((0.587 )  * (1L << 16)))  * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_BLUE_Y_LUT[i]          = ((  ((int32_t) ((0.114 )  * (1L << 16)))  * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_RED_CB_LUT[i]          = (((-((int32_t) ((0.1687 ) * (1L << 16)))) * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_GREEN_CB_LUT[i]        = (((-((int32_t) ((0.3313 ) * (1L << 16)))) * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    /* BLUE_CB_LUT and RED_CR_LUT are identical */
    SCRU_BLUE_CB_RED_CR_LUT[i]  = ((  ((int32_t) ((0.5 )    * (1L << 16)))  * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_GREEN_CR_LUT[i]        = (((-((int32_t) ((0.4187 ) * (1L << 16)))) * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
    SCRU_BLUE_CR_LUT[i]         = (((-((int32_t) ((0.0813 ) * (1L << 16)))) * i) + ((int32_t) 1 << (16 - 1))) >> 16 ;
  }
}

int SCRL_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
{
  struct scrl_usb_ctx *ctx = &scrl_ctx;
  int ret;

  ret = SCRU_validate_parameters(layers_config, screen_config);
  if (ret)
    return ret;

  SCRL_Init_Lut();

  ret = SCRC_Init(layers_config, screen_config, &ctx->common);
  if (ret)
    return ret;

  ctx->usb_cbs.frame_release = usb_frame_release_cb;
  ctx->is_screen_ready_to_update = 1;

  SCRU_usb_init(USB1_OTG_HS, ctx);

  UTIL_LCD_SetFuncDriver(&Common_Driver);
  UTIL_LCD_SetDevice((uint32_t) &ctx->common);

#ifdef SCR_LIB_USE_THREADX
  SCRL_Init_threadx(ctx);
#elif defined(SCR_LIB_USE_FREERTOS)
  SCRL_Init_freertos(ctx);
#endif

  return 0;
}

int SCRL_SetAddress_NoReload(void *address, SCRL_Layer layer)
{
  struct scrl_usb_ctx *ctx = &scrl_ctx;

  ctx->common.layers[layer].address = address;

  return 0;
}

int SCRL_ReloadLayer(SCRL_Layer layer)
{
  struct scrl_usb_ctx *ctx = &scrl_ctx;

  ctx->common.display_address[layer] = ctx->common.layers[layer].address;

  return 0;
}

int SRCL_Update(void)
{
  struct scrl_usb_ctx *ctx = &scrl_ctx;
  int ret = 0;

#ifdef SCR_LIB_USE_THREADX
  tx_semaphore_ceiling_put(&ctx->update_sem, 1);
#elif defined(SCR_LIB_USE_FREERTOS)
  xSemaphoreGive(ctx->update_sem);
#else
  ret = start_composition(ctx);
#endif

  return ret;
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  assert(hpcd == &uvcl_pcd_handle);
  assert(hpcd->Instance == USB1_OTG_HS);

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_SLEEP_ENABLE();

  /* Enable the VDD33USB independent USB 33 voltage monitor */
  HAL_PWREx_EnableVddUSBVMEN();

  /* Wait until VDD33USB is ready */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY) == 0U);

  /* Enable VDDUSB supply */
  HAL_PWREx_EnableVddUSB();

  /* Enable USB1 OTG clock */
  __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_CLK_SLEEP_ENABLE();

  /* Set FSEL to 24 Mhz */
  USB1_HS_PHYC->USBPHYC_CR &= ~(0x7U << 0x4U);
  USB1_HS_PHYC->USBPHYC_CR |= (0x2U << 0x4U);

  /* Enable USB1 OTG PHY clock */
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_SLEEP_ENABLE();

  HAL_NVIC_SetPriority(USB1_OTG_HS_IRQn, 6U, 0U);

  /* Enable USB OTG interrupt */
  HAL_NVIC_EnableIRQ(USB1_OTG_HS_IRQn);
}

void USB1_OTG_HS_IRQHandler(void)
{
  UVCL_IRQHandler();
}
