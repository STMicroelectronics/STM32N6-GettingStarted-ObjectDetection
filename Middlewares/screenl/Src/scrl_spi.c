#include "scrl.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"
#include "stm32_lcd.h"
#include "stm32n6xx_hal_def.h"
#ifdef SCR_LIB_USE_THREADX
#include "tx_api.h"
#endif
#ifdef SCR_LIB_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif
#include "ili9341.h"
#include "lcd_conf.h"
#include "scrl_common.h"

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

struct scrl_spi_ctx {
  struct scrl_common_ctx common;
#ifdef SCR_LIB_USE_THREADX
  TX_THREAD update_thread;
  TX_SEMAPHORE update_sem;
  TX_SEMAPHORE dma2d_sem;
  TX_SEMAPHORE dma_sem;
#elif defined(SCR_LIB_USE_FREERTOS)
  StaticTask_t update_thread;
  SemaphoreHandle_t update_sem;
  StaticSemaphore_t update_sem_buffer;
  SemaphoreHandle_t dma2d_sem;
  StaticSemaphore_t dma2d_sem_buffer;
  SemaphoreHandle_t dma_sem;
  StaticSemaphore_t dma_sem_buffer;
#else
  int is_screen_ready_to_update;
#endif
  /* spi tx context */
  uint8_t *data;
  int len;
};

static struct scrl_spi_ctx scrl_ctx;
static SPI_HandleTypeDef hspi;
static DMA_HandleTypeDef hdma;
#ifdef SCR_LIB_USE_THREADX
static uint8_t update_thread_stack[4096];
#endif
#ifdef SCR_LIB_USE_FREERTOS
static StackType_t update_thread_stack[configMINIMAL_STACK_SIZE];
#endif
static ILI9341_Object_t ObjCtx;
static ILI9341_IO_t IOCtx;

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
    return 1;
  default:
    return 0;
  }
}

static int SCRS_validate_parameters(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
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

static void spi_transfert_data_init(struct scrl_spi_ctx *ctx)
{
  uint32_t w = ctx->common.screen.size.width;
  uint32_t h = ctx->common.screen.size.height;

  ILI9341_SetDisplayWindow(&ObjCtx, 0, 0, w, h);

  ctx->data = ctx->common.screen.address;
  /* Only 2bpp supported */
  ctx->len = w * h * 2;
  LCD_CS_LOW();
}

static void spi_transfert_data_deinit(struct scrl_spi_ctx *ctx)
{
  LCD_CS_HIGH();
}

static void spi_transfert_data_chunck_start(struct scrl_spi_ctx *ctx)
{
  uint16_t size = MIN(ctx->len, 63 * 1024);
  int ret;

  ret = HAL_SPI_Transmit_DMA(&hspi, ctx->data, size);
  assert(ret == HAL_OK);

  ctx->len -= size;
  ctx->data += size;
}

#ifdef SCR_LIB_USE_THREADX
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_spi_ctx *ctx = container_of(ctx_common, struct scrl_spi_ctx, common);
  int ret;

  ret = tx_semaphore_put(&ctx->dma2d_sem);
  assert(ret == 0);
}
#elif defined(SCR_LIB_USE_FREERTOS)
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_spi_ctx *ctx = container_of(ctx_common, struct scrl_spi_ctx, common);
  int ret;

  ret = xSemaphoreGive(ctx->dma2d_sem);
  assert(ret == pdTRUE);
}
#else
void SCRC_Composition_Done_Callback(struct scrl_common_ctx *ctx_common)
{
  struct scrl_spi_ctx *ctx = container_of(ctx_common, struct scrl_spi_ctx, common);

  spi_transfert_data_init(ctx);
  spi_transfert_data_chunck_start(ctx);
}
#endif

#ifdef SCR_LIB_USE_THREADX
static void spi_transfert_data(struct scrl_spi_ctx *ctx)
{
  int ret;

  spi_transfert_data_init(ctx);
  /* Send Data */
  while (ctx->len) {
    spi_transfert_data_chunck_start(ctx);
    ret = tx_semaphore_get(&ctx->dma_sem, TX_WAIT_FOREVER);
    assert(ret == 0);
  }
  spi_transfert_data_deinit(ctx);
}

static void update_thread_fct(ULONG arg)
{
  struct scrl_spi_ctx *ctx = (struct scrl_spi_ctx *) arg;
  int ret;

  while (1)
  {
    ret = tx_semaphore_get(&ctx->update_sem, TX_WAIT_FOREVER);
    assert(ret == 0);

    SCRC_Composition_Start(&ctx->common, 1);
    ret = tx_semaphore_get(&ctx->dma2d_sem, TX_WAIT_FOREVER);
    assert(ret == 0);
    spi_transfert_data(ctx);
  }
}

static void SCRL_Init_threadx(struct scrl_spi_ctx *ctx)
{
  const UINT update_thread_priority = TX_MAX_PRIORITIES / 2 + 2;
  const ULONG time_slice = 10;
  int ret;

  ret = tx_semaphore_create(&ctx->update_sem, NULL, 0);
  assert(ret == 0);
  ret = tx_semaphore_create(&ctx->dma2d_sem, NULL, 0);
  assert(ret == 0);
  ret = tx_semaphore_create(&ctx->dma_sem, NULL, 0);
  assert(ret == 0);
  ret = tx_thread_create(&ctx->update_thread, "ud", update_thread_fct, (ULONG) ctx, update_thread_stack,
                         sizeof(update_thread_stack), update_thread_priority, update_thread_priority, time_slice,
                         TX_AUTO_START);
  assert(ret == TX_SUCCESS);
}
#elif defined(SCR_LIB_USE_FREERTOS)
static void spi_transfert_data(struct scrl_spi_ctx *ctx)
{
  int ret;

  spi_transfert_data_init(ctx);
  /* Send Data */
  while (ctx->len) {
    spi_transfert_data_chunck_start(ctx);
    ret = xSemaphoreTake(ctx->dma_sem, portMAX_DELAY);
    assert(ret == pdTRUE);
  }
  spi_transfert_data_deinit(ctx);
}

static void update_thread_fct(void *arg)
{
  struct scrl_spi_ctx *ctx = (struct scrl_spi_ctx *) arg;
  int ret;

  while (1)
  {
    ret = xSemaphoreTake(ctx->update_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    SCRC_Composition_Start(&ctx->common, 1);
    ret = xSemaphoreTake(ctx->dma2d_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    spi_transfert_data(ctx);
  }
}

static void SCRL_Init_freertos(struct scrl_spi_ctx *ctx)
{
  const int update_thread_priority = tskIDLE_PRIORITY + 20 - 2;
  TaskHandle_t hdl;

  ctx->update_sem = xSemaphoreCreateCountingStatic(1, 0, &ctx->update_sem_buffer);
  assert(ctx->update_sem);
  ctx->dma2d_sem = xSemaphoreCreateCountingStatic(1, 0, &ctx->dma2d_sem_buffer);
  assert(ctx->dma2d_sem);
  ctx->dma_sem = xSemaphoreCreateCountingStatic(1, 0, &ctx->dma_sem_buffer);
  assert(ctx->dma_sem);

  hdl = xTaskCreateStatic(update_thread_fct, "ud", configMINIMAL_STACK_SIZE, ctx, update_thread_priority,
                          update_thread_stack, &ctx->update_thread);
  assert(hdl);
}
#else
static int start_composition(struct scrl_spi_ctx *ctx)
{
  if (!ctx->is_screen_ready_to_update)
      return 0;

  ctx->is_screen_ready_to_update = 0;
  SCRC_Composition_Start(&ctx->common, 1);

  return 0;
}
#endif

static int32_t SPI_IO_Init(void)
{
  LCD_RST_LOW();
  HAL_Delay(100);
  LCD_RST_HIGH();
  HAL_Delay(100);

  /* turn LCD on = drive pin low (active low) */
  /* Set or Reset the control line */
  LCD_CS_HIGH();
  LCD_DC_HIGH();

  return 0;
}

static int32_t SPI_IO_DeInit(void)
{
  return 0;
}

static int32_t SPI_IO_ReadReg(volatile uint8_t *Reg, uint32_t Length)
{
  assert(0);

  return -1;
}

static int32_t SPI_IO_WriteReg(volatile uint8_t *Reg, uint32_t Length)
{
  int ret;

  /* Select : Chip Select low */
  LCD_CS_LOW();

  LCD_DC_HIGH();
  ret = HAL_SPI_Transmit(&hspi, (uint8_t *)Reg, Length, HAL_MAX_DELAY);
  LCD_DC_LOW();

  /* Deselect : Chip Select high */
  LCD_CS_HIGH();

  return ret;
}

static int32_t SPI_IO_SendData(uint8_t *pData, uint32_t Length)
{
  int32_t ret;

  /* Select : Chip Select low */
  LCD_CS_LOW();

  /* Send Data */
  while (Length) {
    uint16_t size = MIN(Length, 63 * 1024);

    ret = HAL_SPI_Transmit(&hspi, (uint8_t *)pData, size, HAL_MAX_DELAY);
    if (ret)
      break;
    Length -= size;
    pData += size;
  }

  /* Deselect : Chip Select high */
  LCD_CS_HIGH();

  return ret;
}

static int32_t SPI_IO_SendDataDMA(uint8_t *pData, uint32_t Length)
{
  assert(0);

  return -1;
}

static int32_t SPI_IO_RecvData(uint8_t *pData, uint32_t Length)
{
  assert(0);

  return -1;
}

static int32_t SPI_IO_RecvDataDMA(uint8_t *pData, uint32_t Length)
{
  assert(0);

  return -1;
}

static int32_t SPI_IO_GetTick(void)
{
  return (int32_t) HAL_GetTick();
}

static int32_t SPI_IO_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);

  return 0;
}

static void SCRL_Init_spi(struct scrl_spi_ctx *ctx)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  ILI9341_InitParams_t ILI9341_InitParams = {0};
  GPIO_InitTypeDef gpio_init = {0};
  int ret;

  /* Power */
  /* For PC10 */
  HAL_PWREx_EnableVddIO4();

  /* clocks */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI5;
  PeriphClkInitStruct.Spi5ClockSelection = RCC_SPI5CLKSOURCE_PCLK2;
  ret = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  assert(ret == HAL_OK);
  __HAL_RCC_HPDMA1_CLK_ENABLE();
  __HAL_RCC_HPDMA1_CLK_SLEEP_ENABLE();
  __HAL_RCC_SPI5_CLK_ENABLE();
  __HAL_RCC_SPI5_CLK_SLEEP_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_SLEEP_ENABLE();

  /* gpios */
    /* reset */
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOC, &gpio_init);

   /* cs */
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Pin = GPIO_PIN_12;
  HAL_GPIO_Init(GPIOD, &gpio_init);

    /* dc */
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Pin = GPIO_PIN_5;
  HAL_GPIO_Init(GPIOD, &gpio_init);

    /* spi clock */
  gpio_init.Pin = GPIO_PIN_15;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF5_SPI5;
  HAL_GPIO_Init(GPIOE, &gpio_init);

    /* spi mosi */
  gpio_init.Pin = GPIO_PIN_2;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = GPIO_AF5_SPI5;
  HAL_GPIO_Init(GPIOG, &gpio_init);

  /* dma for spi tx */
  hdma.Instance = HPDMA1_Channel12;
  hdma.Init.Request = HPDMA1_REQUEST_SPI5_TX;
  hdma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
  hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma.Init.SrcInc = DMA_SINC_INCREMENTED;
  hdma.Init.DestInc = DMA_DINC_FIXED;
  hdma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
  hdma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
  hdma.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
  hdma.Init.SrcBurstLength = 1;
  hdma.Init.DestBurstLength = 1;
  hdma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
  hdma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
  hdma.Init.Mode = DMA_NORMAL;
  ret = HAL_DMA_Init(&hdma);
  assert(ret == 0);

  __HAL_LINKDMA(&hspi, hdmatx, hdma);

  ret = HAL_DMA_ConfigChannelAttributes(&hdma, DMA_CHANNEL_PRIV|DMA_CHANNEL_SEC|DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC);
  assert(ret == 0);

  DMA_IsolationConfigTypeDef IsolationConfiginput;
  IsolationConfiginput.CidFiltering = DMA_ISOLATION_ON;
  IsolationConfiginput.StaticCid = DMA_CHANNEL_STATIC_CID_1;
  ret = HAL_DMA_SetIsolationAttributes(&hdma, &IsolationConfiginput);
  assert(ret == 0);

  /* spi @50Mbit/s */
  hspi.Instance = SPI5;
  hspi.Init.Mode = SPI_MODE_MASTER;
  hspi.Init.Direction = SPI_DIRECTION_1LINE;
  hspi.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi.Init.NSS = SPI_NSS_SOFT;
  hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi.Init.CRCPolynomial = 0x7;
  hspi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  ret = HAL_SPI_Init(&hspi);
  assert(ret == 0);

  /* turn on IRQs */
  HAL_NVIC_SetPriority(HPDMA1_Channel12_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(HPDMA1_Channel12_IRQn);
  HAL_NVIC_SetPriority(SPI5_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(SPI5_IRQn);

  /* display init */
  IOCtx.Init             = SPI_IO_Init;
  IOCtx.DeInit           = SPI_IO_DeInit;
  IOCtx.ReadReg          = SPI_IO_ReadReg;
  IOCtx.WriteReg         = SPI_IO_WriteReg;
  IOCtx.SendData         = SPI_IO_SendData;
  IOCtx.SendDataDMA      = SPI_IO_SendDataDMA;
  IOCtx.RecvData         = SPI_IO_RecvData;
  IOCtx.RecvDataDMA      = SPI_IO_RecvDataDMA;
  IOCtx.GetTick          = SPI_IO_GetTick;
  IOCtx.Delay            = SPI_IO_Delay;
  ret = ILI9341_RegisterBusIO(&ObjCtx, &IOCtx);
  assert(ret == 0);

  ILI9341_InitParams.Endian         = ILI9341_ENDIAN_LITTLE;
  ILI9341_InitParams.SwapRB         = 0;
  ILI9341_InitParams.ColorCoding    = ILI9341_FORMAT_RBG565;
  ILI9341_InitParams.Orientation    = ILI9341_ORIENTATION_LANDSCAPE;
  ILI9341_InitParams.TEMode         = ILI9341_TE_DISABLED;
  ret = ILI9341_Init(&ObjCtx, &ILI9341_InitParams);
  assert(ret == 0);

  ret = ILI9341_DisplayOn(&ObjCtx);
  assert(ret == 0);
}

int SCRL_Init(SCRL_LayerConfig *layers_config[SCRL_LAYER_NB], SCRL_ScreenConfig *screen_config)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;
  int ret;

  ret = SCRS_validate_parameters(layers_config, screen_config);
  if (ret)
    return ret;

  ret = SCRC_Init(layers_config, screen_config, &ctx->common);
  if (ret)
    return ret;

  SCRL_Init_spi(ctx);

  UTIL_LCD_SetFuncDriver(&Common_Driver);
  UTIL_LCD_SetDevice((uint32_t) &ctx->common);

#ifdef SCR_LIB_USE_THREADX
  SCRL_Init_threadx(ctx);
#elif defined(SCR_LIB_USE_FREERTOS)
  SCRL_Init_freertos(ctx);
#else
  ctx->is_screen_ready_to_update = 1;
#endif

  return 0;
}

int SCRL_SetAddress_NoReload(void *address, SCRL_Layer layer)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;

  ctx->common.layers[layer].address = address;

  return 0;
}

int SCRL_ReloadLayer(SCRL_Layer layer)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;

  ctx->common.display_address[layer] = ctx->common.layers[layer].address;

  return 0;
}

int SRCL_Update()
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;
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

#ifdef SCR_LIB_USE_THREADX
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;
  int ret;

  ret = tx_semaphore_put(&ctx->dma_sem);
  assert(ret == 0);
}
#elif defined(SCR_LIB_USE_FREERTOS)
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;
  int ret;

  ret = xSemaphoreGive(ctx->dma_sem);
  assert(ret == pdTRUE);
}
#else
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  struct scrl_spi_ctx *ctx = &scrl_ctx;

  if (ctx->len) {
    spi_transfert_data_chunck_start(ctx);
  } else {
    spi_transfert_data_deinit(ctx);
    ctx->is_screen_ready_to_update = 1;
  }
}
#endif

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  assert(0);
}

void SPI5_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
}

void HPDMA1_Channel12_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma);
}
