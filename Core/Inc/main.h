/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"
#include "stm32h5xx_ll_adc.h"
#include "stm32h5xx_ll_icache.h"
#include "stm32h5xx_ll_pwr.h"
#include "stm32h5xx_ll_crs.h"
#include "stm32h5xx_ll_rcc.h"
#include "stm32h5xx_ll_bus.h"
#include "stm32h5xx_ll_system.h"
#include "stm32h5xx_ll_exti.h"
#include "stm32h5xx_ll_cortex.h"
#include "stm32h5xx_ll_utils.h"
#include "stm32h5xx_ll_dma.h"
#include "stm32h5xx_ll_spi.h"
#include "stm32h5xx_ll_tim.h"
#include "stm32h5xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPIO_EXTI13_Pin LL_GPIO_PIN_13
#define GPIO_EXTI13_GPIO_Port GPIOC
#define Sensor_Mux1_Pin LL_GPIO_PIN_1
#define Sensor_Mux1_GPIO_Port GPIOC
#define Senosor_Mux2_Pin LL_GPIO_PIN_2
#define Senosor_Mux2_GPIO_Port GPIOC
#define Sensor_MuxX_Pin LL_GPIO_PIN_3
#define Sensor_MuxX_GPIO_Port GPIOC
#define LED2_Pin LL_GPIO_PIN_5
#define LED2_GPIO_Port GPIOA
#define USART3_RX_Pin LL_GPIO_PIN_4
#define USART3_RX_GPIO_Port GPIOC
#define Motor_R6_Pin LL_GPIO_PIN_1
#define Motor_R6_GPIO_Port GPIOB
#define Motor_L6_Pin LL_GPIO_PIN_2
#define Motor_L6_GPIO_Port GPIOB
#define OLED_CS_Pin LL_GPIO_PIN_13
#define OLED_CS_GPIO_Port GPIOB
#define OLED_DC_Pin LL_GPIO_PIN_14
#define OLED_DC_GPIO_Port GPIOB
#define Motor_L1_Pin LL_GPIO_PIN_6
#define Motor_L1_GPIO_Port GPIOC
#define Motor_L2_Pin LL_GPIO_PIN_7
#define Motor_L2_GPIO_Port GPIOC
#define Motor_L4_Pin LL_GPIO_PIN_8
#define Motor_L4_GPIO_Port GPIOC
#define Motor_L3_Pin LL_GPIO_PIN_9
#define Motor_L3_GPIO_Port GPIOC
#define Motor_L5_Pin LL_GPIO_PIN_8
#define Motor_L5_GPIO_Port GPIOA
#define Motor_R5_Pin LL_GPIO_PIN_9
#define Motor_R5_GPIO_Port GPIOA
#define Motor_R4_Pin LL_GPIO_PIN_2
#define Motor_R4_GPIO_Port GPIOD
#define SWL_Pin LL_GPIO_PIN_4
#define SWL_GPIO_Port GPIOB
#define SWR_Pin LL_GPIO_PIN_5
#define SWR_GPIO_Port GPIOB
#define Motor_R1_Pin LL_GPIO_PIN_6
#define Motor_R1_GPIO_Port GPIOB
#define Motor_R2_Pin LL_GPIO_PIN_7
#define Motor_R2_GPIO_Port GPIOB
#define Motor_R3_Pin LL_GPIO_PIN_8
#define Motor_R3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
