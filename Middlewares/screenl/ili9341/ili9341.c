/**
  ******************************************************************************
  * @file    ili9341.c
  * @author  MCD Application Team
  * @brief   This file includes the drivers for ILI9341 LCD controller
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ili9341.h"
#include "lcd_conf.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup ILI9341
  * @brief      This file provides a set of functions needed to drive the
  *             ILI9341 LCD controller.
  * @{
  */

/** @defgroup ILI9341_Private_Types Private Types
  * @{
  */
typedef struct
{
  uint32_t        Width;
  uint32_t        Height;
  uint32_t        Orientation;
} ILI9341_Ctx_t;
/**
  * @}
  */

/** @defgroup ILI9341_Private_Variables Private Variables
  * @{
  */
ILI9341_LCD_Drv_t   ILI9341_LCD_Driver =
{
  ILI9341_Init,
  ILI9341_DeInit,
  ILI9341_ReadID,
  ILI9341_DisplayOn,
  ILI9341_DisplayOff,
  ILI9341_SetDisplayWindow,
  NULL, // ILI9341_SetBrightness,
  NULL, // ILI9341_GetBrightness,
  ILI9341_SetOrientation,
  ILI9341_GetOrientation,
  NULL, // ILI9341_SetCursor,
  NULL, // ILI9341_DrawBitmap,
  NULL, // ILI9341_FillRGBRect,
  NULL, // ILI9341_DrawHLine,
  NULL, // ILI9341_DrawVLine,
  NULL, // ILI9341_FillRect,
  NULL, // ILI9341_GetPixel,
  NULL, // ILI9341_SetPixel,
  ILI9341_GetXSize,
  ILI9341_GetYSize,
};

/* The below table handle the different values to be set to Memory Data Access Control
   depending on the orientation and pbm image writing where the data order is inverted
*/
static uint32_t OrientationTab[4] =
{
  0x00U , /* Portrait orientation choice of LCD screen               */
  0xC0U , /* Portrait rotated 180° orientation choice of LCD screen  */
  0xA0U , /* Landscape orientation choice of LCD screen              */
  0x60U , /* Landscape rotated 180° orientation choice of LCD screen */
};

static ILI9341_Ctx_t ILI9341Ctx;
/**
  * @}
  */

/** @defgroup ILI9341_Private_FunctionsPrototypes Private Functions Prototypes
  * @{
  */
static int32_t ILI9341_ReadRegWrap(void *Handle, volatile uint8_t* Reg, uint32_t Length);
static int32_t ILI9341_WriteRegWrap(void *Handle, volatile uint8_t* Reg, uint32_t Length);
static int32_t ILI9341_SendDataWrap(void *Handle, uint8_t *pData, uint32_t Length);
static int32_t ILI9341_SendDataDMAWrap(void *Handle, uint8_t *pData, uint32_t Length);
static int32_t ILI9341_RecvDataWrap(void *Handle, uint8_t *pData, uint32_t Length);
static int32_t ILI9341_RecvDataDMAWrap(void *Handle, uint8_t *pData, uint32_t Length);
static int32_t ILI9341_Delay(ILI9341_Object_t *pObj, uint32_t Delay);
/**
* @}
*/

/** @addtogroup ILI9341_Exported_Functions
  * @{
  */
/**
  * @brief  Register component IO bus
  * @param  pObj Component object pointer
  * @param  pIO  Component IO structure pointer
  * @retval Component status
  */
int32_t ILI9341_RegisterBusIO (ILI9341_Object_t *pObj, ILI9341_IO_t *pIO)
{
  int32_t ret;

  if(pObj == NULL)
  {
    ret = ILI9341_ERROR;
  }
  else
  {
    pObj->IO.Init         = pIO->Init;
    pObj->IO.DeInit       = pIO->DeInit;
    pObj->IO.Address      = pIO->Address;
    pObj->IO.WriteReg     = pIO->WriteReg;
    pObj->IO.ReadReg      = pIO->ReadReg;
    pObj->IO.SendData     = pIO->SendData;
    pObj->IO.SendDataDMA  = pIO->SendDataDMA;
    pObj->IO.RecvData     = pIO->RecvData;
    pObj->IO.RecvDataDMA  = pIO->RecvDataDMA;
    pObj->IO.GetTick      = pIO->GetTick;

    pObj->Ctx.ReadReg     = ILI9341_ReadRegWrap;
    pObj->Ctx.WriteReg    = ILI9341_WriteRegWrap;
    pObj->Ctx.SendData    = ILI9341_SendDataWrap;
    pObj->Ctx.SendDataDMA = ILI9341_SendDataDMAWrap;
    pObj->Ctx.RecvData    = ILI9341_RecvDataWrap;
    pObj->Ctx.RecvDataDMA = ILI9341_RecvDataDMAWrap;
    pObj->Ctx.handle      = pObj;

    if(pObj->IO.Init != NULL)
    {
      ret = pObj->IO.Init();
    }
    else
    {
      ret = ILI9341_ERROR;
    }
  }

  return ret;
}

/**
  * @brief  Initialize the ili9341 LCD Component.
  * @param  pObj Component object
  * @param  pInitParams Initialization Parameters
  * @retval Component status
  */
int32_t ILI9341_Init(ILI9341_Object_t *pObj, ILI9341_InitParams_t* pInitParams)
{
  uint8_t i;
  uint8_t __IO parameter[4];
  uint8_t __IO reg;
  int32_t ret = ILI9341_OK;

  if((pObj == NULL) || (pInitParams->Orientation > ILI9341_ORIENTATION_LANDSCAPE_ROT180))
  {
    ret = ILI9341_ERROR;
  }
  else if (pObj->IsInitialized == 0)
  {
    pObj->IsInitialized = 1;

    /* Sleep In Command */
    reg = ILI9341_SPLIN;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    /* Wait for 10ms */
    ILI9341_Delay(pObj, 10);

    /* SW Reset Command */
    reg = ILI9341_SWRESET;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    /* Wait for 200ms */
    ILI9341_Delay(pObj, 200);

    /* Sleep Out Command */
    reg = ILI9341_SLEEP_OUT;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    ILI9341_Delay(pObj, 200);

    if(pInitParams->SwapRB)
    {
      for(i = 0; i < 4; i++)
      {
        OrientationTab[i] |= 0x08;
      }
    }

    /* Set Display Orientation */
    ret += ILI9341_SetOrientation(pObj, pInitParams->Orientation);

    /* Set Full Screen Display Window */
    ret += ILI9341_SetDisplayWindow(pObj, 0U, 0U, ILI9341_WIDTH, ILI9341_HEIGHT);

    /* Set Pixel Format */
    reg = ILI9341_PIXEL_FORMAT;
    parameter[0] = (uint8_t) pInitParams->ColorCoding;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 1);

    /* Frame Rate Control in normal mode */
    reg = ILI9341_FRMCTR1;
    parameter[0] = 0x00; /* fosc/1 */
    parameter[1] = 0x1F; /* Default is 61 Hz */
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 2);

    // Display Normal mode
    reg = ILI9341_NORMAL_MODE_ON;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);

    // Update Interface control
    reg = ILI9341_INTERFACE;
    ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
    if(pInitParams->SwapRB)
    {
      parameter[0] = 0x49;
    }
    else
    {
      parameter[0] = 0x48;
    }
    parameter[1] = 0;
    if(pInitParams->Endian == ILI9341_ENDIAN_LITTLE)
    {
      parameter[2] = 0x20;
    }
    else
    {
      parameter[2] = 0x00;
    }
    ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 3);

    if(pInitParams->TEMode == ILI9341_TE_MODE_1)
    {
      // Set TE scan line
      reg = ILI9341_SET_TEAR_SCANLINE;
      ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
      parameter[0] = (pInitParams->TEScanline & 0x100);
      parameter[1] = (pInitParams->TEScanline & 0x0FF);
      ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 2);

      // Enable TE
      reg = ILI9341_TEON;
      ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
      parameter[0] = 0; /* VSYNC Only */
      ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 1);
    }
    else if(pInitParams->TEMode == ILI9341_TE_MODE_2)
    {
      // Reset TE scan line
      reg = ILI9341_SET_TEAR_SCANLINE;
      ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
      parameter[0] = 0;
      parameter[1] = 0;
      ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 2);

      // Enable TE
      reg = ILI9341_TEON;
      ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
      parameter[0] = 1; /* VSYNC + HSYNC */
      ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 1);
    }
  }

  if(ret != ILI9341_OK)
  {
    ret = ILI9341_ERROR;
  }

  return ret;
}

/**
  * @brief  De-Initialize the ili9341 LCD Component.
  * @param  pObj Component object
  * @retval Component status
  */
int32_t ILI9341_DeInit(ILI9341_Object_t *pObj)
{
  return pObj->IO.DeInit();
}

/**
  * @brief  Get the ili9341 ID.
  * @param  pObj Component object
  * @param  Id Component ID
  * @retval The component status
  */
int32_t ILI9341_ReadID(ILI9341_Object_t *pObj, uint32_t *Id)
{
  int32_t ret;
  uint8_t __IO data[4] = {ILI9341_READ_ID4, 0, 0, 0 };

  if(ili9341_recv_data(&pObj->Ctx, (uint8_t *)data, 4) != ILI9341_OK)
  {
    ret = ILI9341_ERROR;
  }
  else
  {
    *Id = ((*(uint8_t *)&data[1] << 16) | (*(uint8_t *)&data[2] << 8) | *(uint8_t *)&data[3]);
    ret = ILI9341_OK;
  }

  return ret;
}

/**
  * @brief  Enables the Display.
  * @param  pObj Component object
  * @retval The component status
  */
int32_t ILI9341_DisplayOn(ILI9341_Object_t *pObj)
{
  uint8_t __IO reg = ILI9341_DISPLAY_ON;

  return ili9341_write_reg(&pObj->Ctx, &reg, 1);
}

/**
  * @brief  Disables the Display.
  * @param  pObj Component object
  * @retval The component status
  */
int32_t ILI9341_DisplayOff(ILI9341_Object_t *pObj)
{
  uint8_t __IO reg = ILI9341_DISPLAY_OFF;

  return ili9341_write_reg(&pObj->Ctx, &reg, 1);
}

/**
  * @brief  Set the Display Orientation.
  * @param  pObj Component object
  * @param  Orientation ILI9341_ORIENTATION_PORTRAIT, ILI9341_ORIENTATION_PORTRAIT_ROT180
  *                     ILI9341_ORIENTATION_LANDSCAPE or ILI9341_ORIENTATION_LANDSCAPE_ROT180
  * @retval The component status
  */
int32_t ILI9341_SetOrientation(ILI9341_Object_t *pObj, uint32_t Orientation)
{
  int32_t ret;
  uint8_t __IO parameter;
  uint8_t __IO reg;

  parameter = (uint8_t)OrientationTab[Orientation];
  reg = ILI9341_MAC;
  ret = ili9341_write_reg(&pObj->Ctx, &reg, 1);
  ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)&parameter, 1);

  if(ret == ILI9341_OK)
  {
    ILI9341Ctx.Orientation = Orientation;

    switch (Orientation)
    {
      case ILI9341_ORIENTATION_LANDSCAPE:
      case ILI9341_ORIENTATION_LANDSCAPE_ROT180:
        ILI9341Ctx.Width  = ILI9341_HEIGHT;
        ILI9341Ctx.Height = ILI9341_WIDTH;
      break;
      default:
      case ILI9341_ORIENTATION_PORTRAIT_ROT180:
      case ILI9341_ORIENTATION_PORTRAIT:
        ILI9341Ctx.Width  = ILI9341_WIDTH;
        ILI9341Ctx.Height = ILI9341_HEIGHT;
      break;
    }

    pObj->XSize = ILI9341Ctx.Width;
    pObj->YSize = ILI9341Ctx.Height;
  }
  else
  {
    ret = ILI9341_ERROR;
  }

  return ret;
}

/**
  * @brief  Set the Display Orientation.
  * @param  pObj Component object
  * @param  Orientation pointer to the LCD orientation
  * @retval The component status
  */
int32_t ILI9341_GetOrientation(ILI9341_Object_t *pObj, uint32_t *Orientation)
{
  (void)pObj;

  *Orientation = ILI9341Ctx.Orientation;

  return ILI9341_OK;
}

/**
  * @brief  Get the LCD pixel Width.
  * @param  pObj Component object
  * @retval The Lcd Pixel Width
  */
int32_t ILI9341_GetXSize(ILI9341_Object_t *pObj, uint32_t *XSize)
{
  (void)pObj;

  *XSize = ILI9341Ctx.Width;

  return ILI9341_OK;
}

/**
  * @brief  Get the LCD pixel Height.
  * @param  pObj Component object
  * @retval The Lcd Pixel Height
  */
int32_t ILI9341_GetYSize(ILI9341_Object_t *pObj, uint32_t *YSize)
{
  (void)pObj;

  *YSize = ILI9341Ctx.Height;

  return ILI9341_OK;
}

/**
  * @brief  Sets a display window
  * @param  Xpos   specifies the X bottom left position.
  * @param  Ypos   specifies the Y bottom left position.
  * @param  Height display window height.
  * @param  Width  display window width.
  * @retval Component status
  */
int32_t ILI9341_SetDisplayWindow(ILI9341_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height)
{
  int32_t ret;
  uint8_t __IO parameter[4];
  uint8_t __IO reg = ILI9341_CASET;

  /* Column addr set, 4 args, no delay: XSTART = Xpos, XEND = (Xpos + Width - 1) */
  ret = ili9341_write_reg(&pObj->Ctx, &reg, 1);
  parameter [0] = (uint8_t)(Xpos >> 8U);
  parameter [1] = (uint8_t)(Xpos & 0xFFU);
  parameter [2] = (uint8_t)((Xpos + Width - 1U) >> 8U);
  parameter [3] = (uint8_t)((Xpos + Width - 1U) & 0xFFU);
  ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 4);

  /* Row addr set, 4 args, no delay: YSTART = Ypos, YEND = (Ypos + Height - 1) */
  reg = ILI9341_RASET;
  ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);
  parameter [0] = (uint8_t)(Ypos >> 8U);
  parameter [1] = (uint8_t)(Ypos & 0xFFU);
  parameter [2] = (uint8_t)((Ypos + Height - 1U) >> 8U);
  parameter [3] = (uint8_t)((Ypos + Height - 1U) & 0xFFU);
  ret += ili9341_send_data(&pObj->Ctx, (uint8_t *)parameter, 4);

  reg = ILI9341_GRAM;
  ret += ili9341_write_reg(&pObj->Ctx, &reg, 1);

  if(ret != ILI9341_OK)
  {
    ret = ILI9341_ERROR;
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup ILI9341_Private_Functions  Private Functions
  * @{
  */
/**
  * @brief  Wrap component ReadReg to Bus Read function
  * @param  Handle  Component object handle
  * @param  Reg  The target register address to write
  * @param  pData  The target register value to be written
  * @retval Component error status
  */
static int32_t ILI9341_ReadRegWrap(void *Handle, volatile uint8_t* Reg, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.ReadReg(Reg, Length);
}

/**
  * @brief  Wrap component WriteReg to Bus Write function
  * @param  handle  Component object handle
  * @param  Reg  The target register address to write
  * @param  pData  The target register value to be written
  * @param  Length  buffer size to be written
  * @retval Component error status
  */
static int32_t ILI9341_WriteRegWrap(void *Handle, volatile uint8_t* Reg, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.WriteReg(Reg, Length);
}


/**
  * @brief  Wrap component SendData to Bus Write function
  * @param  handle  Component object handle
  * @param  pData  The target register value to be written
  * @retval Component error status
  */
static int32_t ILI9341_SendDataWrap(void *Handle, uint8_t *pData, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.SendData(pData, Length);
}

/**
  * @brief  Wrap component SendDataDMA to Bus Write function
  * @param  handle  Component object handle
  * @param  pData  The target register value to be written
  * @retval Component error status
  */
static int32_t ILI9341_SendDataDMAWrap(void *Handle, uint8_t *pData, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.SendDataDMA(pData, Length);
}

/**
  * @brief  Wrap component SendData to Bus Write function
  * @param  handle  Component object handle
  * @param  pData  The target register value to be written
  * @retval Component error status
  */
static int32_t ILI9341_RecvDataWrap(void *Handle, uint8_t *pData, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.RecvData(pData, Length);
}

/**
  * @brief  Wrap component SendDataDMA to Bus Write function
  * @param  handle  Component object handle
  * @param  pData  The target register value to be written
  * @retval Component error status
  */
static int32_t ILI9341_RecvDataDMAWrap(void *Handle, uint8_t *pData, uint32_t Length)
{
  ILI9341_Object_t *pObj = (ILI9341_Object_t *)Handle;

  return pObj->IO.RecvDataDMA(pData, Length);
}

/**
  * @brief  ILI9341 delay
  * @param  Delay  Delay in ms
  * @retval Component error status
  */
static int32_t ILI9341_Delay(ILI9341_Object_t *pObj, uint32_t Delay)
{
  uint32_t tickstart;
  tickstart = pObj->IO.GetTick();
  while((pObj->IO.GetTick() - tickstart) < Delay)
  {
  }
  return ILI9341_OK;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
