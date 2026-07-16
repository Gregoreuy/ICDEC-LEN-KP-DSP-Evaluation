/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32u5xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_GREEN_Pin GPIO_PIN_0
#define LED_GREEN_GPIO_Port GPIOE
#define LED_RED_Pin GPIO_PIN_1
#define LED_RED_GPIO_Port GPIOE
#define LCD_RESET_Pin GPIO_PIN_5
#define LCD_RESET_GPIO_Port GPIOD
#define DSI_PWR_ON_Pin GPIO_PIN_5
#define DSI_PWR_ON_GPIO_Port GPIOI
#define VSYNC_FREQ_Pin GPIO_PIN_1
#define VSYNC_FREQ_GPIO_Port GPIOD
#define DSI_BL_CTRL_Pin GPIO_PIN_6
#define DSI_BL_CTRL_GPIO_Port GPIOI
#define RENDER_TIME_Pin GPIO_PIN_0
#define RENDER_TIME_GPIO_Port GPIOD
#define USER_BUTTON_Pin GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define OSPI_CLK_Pin GPIO_PIN_10
#define OSPI_CLK_GPIO_Port GPIOF
#define OSPI_D2_Pin GPIO_PIN_7
#define OSPI_D2_GPIO_Port GPIOF
#define OSPI_D1_Pin GPIO_PIN_9
#define OSPI_D1_GPIO_Port GPIOF
#define OSPI_D4_Pin GPIO_PIN_1
#define OSPI_D4_GPIO_Port GPIOC
#define OSPI_D3_Pin GPIO_PIN_6
#define OSPI_D3_GPIO_Port GPIOF
#define OSPI_D0_Pin GPIO_PIN_8
#define OSPI_D0_GPIO_Port GPIOF
#define OSPI_CS_Pin GPIO_PIN_2
#define OSPI_CS_GPIO_Port GPIOA
#define DSI_TOUCH_INT_Pin GPIO_PIN_8
#define DSI_TOUCH_INT_GPIO_Port GPIOE
#define DSI_TOUCH_INT_EXTI_IRQn EXTI8_IRQn
#define OSPI_D6_Pin GPIO_PIN_3
#define OSPI_D6_GPIO_Port GPIOC
#define MCU_ACTIVE_Pin GPIO_PIN_12
#define MCU_ACTIVE_GPIO_Port GPIOF
#define OSPI_D5_Pin GPIO_PIN_2
#define OSPI_D5_GPIO_Port GPIOC
#define OSPI_D7_Pin GPIO_PIN_0
#define OSPI_D7_GPIO_Port GPIOC
#define OSPI_DQS_Pin GPIO_PIN_1
#define OSPI_DQS_GPIO_Port GPIOA
#define FRAME_RATE_Pin GPIO_PIN_14
#define FRAME_RATE_GPIO_Port GPIOF

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
