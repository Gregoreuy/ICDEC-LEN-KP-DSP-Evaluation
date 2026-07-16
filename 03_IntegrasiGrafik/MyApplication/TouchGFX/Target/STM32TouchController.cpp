/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.21.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
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

/* USER CODE BEGIN STM32TouchController */
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/Types.hpp>
#include <STM32TouchController.hpp>
#include "stm32u5x9j_discovery_ts.h"

static bool _initialized = false;
static TS_State_t state = { 0 };

using namespace touchgfx;

void STM32TouchController::init()
{
    if (!_initialized)
    {
        /* Initialize the TouchScreen */
        TS_Init_t TsInit;
        TsInit.Width = HAL::DISPLAY_WIDTH;
        TsInit.Height = HAL::DISPLAY_HEIGHT;
        TsInit.Orientation = TS_ORIENTATION_PORTRAIT;
        TsInit.Accuracy = 0;
        if (BSP_TS_Init(0, &TsInit) == BSP_ERROR_NONE)
        {
            _initialized = true;
        }
    }
}

extern "C" void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TS_INT_PIN)
    {
        if (_initialized)
        {
            if (BSP_TS_GetState(0, &state) != BSP_ERROR_NONE)
            {
                assert(0 && "Failed to read TS state");
            }
        }
    }
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    if (_initialized)
    {
        if (state.TouchDetected)
        {
            x = state.TouchX;
            y = state.TouchY;
            return true;
        }
    }

    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
