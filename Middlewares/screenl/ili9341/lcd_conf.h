#ifndef __LCD_CONF_H__
#define __LCD_CONF_H__

#include "stm32n6xx_hal.h"

#define LCD_RST_LOW()                       WRITE_REG(GPIOC->BRR, GPIO_PIN_10)
#define LCD_RST_HIGH()                      WRITE_REG(GPIOC->BSRR, GPIO_PIN_10)

#define LCD_CS_LOW()                        WRITE_REG(GPIOD->BRR, GPIO_PIN_12)
#define LCD_CS_HIGH()                       WRITE_REG(GPIOD->BSRR, GPIO_PIN_12)

#define LCD_DC_LOW()                        WRITE_REG(GPIOD->BSRR, GPIO_PIN_5)
#define LCD_DC_HIGH()                       WRITE_REG(GPIOD->BRR, GPIO_PIN_5)

#endif
