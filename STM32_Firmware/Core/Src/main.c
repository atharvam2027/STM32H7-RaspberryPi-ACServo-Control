/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

char rxBuffer[64];
uint8_t rxChar;
uint8_t rxIndex = 0;
volatile uint8_t commandReady = 0;

#define PULSES_PER_REV 10000UL
#define TIMER_CLOCK_HZ 280000000UL

volatile uint32_t motor1_target_pulses = 0;
volatile uint32_t motor2_target_pulses = 0;

volatile uint32_t motor1_pulse_count = 0;
volatile uint32_t motor2_pulse_count = 0;

volatile uint8_t motor1_done = 1;
volatile uint8_t motor2_done = 1;

uint8_t uart_rx_byte;
char uart_rx_buffer[64];

volatile uint8_t uart_rx_index = 0;
volatile uint8_t uart_command_ready = 0;

volatile uint8_t both_report_pending = 0;
volatile uint8_t motor1_report_pending = 0;
volatile uint8_t motor2_report_pending = 0;

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void ProcessCommand(void);
void MoveMotor1(float angle, uint32_t rpm, uint8_t direction);
void MoveMotor2(float angle, uint32_t rpm, uint8_t direction);
void MoveBothMotors(float angle1, uint32_t rpm1,
                    uint8_t direction1,
                    float angle2, uint32_t rpm2,
                    uint8_t direction2);
void SetMotorSpeed(TIM_HandleTypeDef *htim, uint32_t rpm);
void ProcessUARTCommand(void);
void UART_SendMessage(const char *message);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  /* USER CODE END 2 */

  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      ProcessUARTCommand();


      if (both_report_pending && motor1_done && motor2_done)
      {
          both_report_pending = 0;
          UART_SendMessage("DONE\r\n");
      }
      else
      {
          if (motor1_report_pending && motor1_done)
          {
              motor1_report_pending = 0;
              UART_SendMessage("DONE\r\n");
          }

          if (motor2_report_pending && motor2_done)
          {
              motor2_report_pending = 0;
              UART_SendMessage("DONE\r\n");
          }
      }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /*AXI clock gating */
  RCC->CKGAENR = 0xFFFFFFFF;

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 35;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 685;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 343;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 3;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 41999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 21000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*AnalogSwitch Config */
  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA1, SYSCFG_SWITCH_PA1_CLOSE);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void UART_SendMessage(const char *message)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)message,
                      strlen(message), 1000);
}



void ProcessUARTCommand(void)
{
    char command[64];
    float angle1, angle2;
    unsigned long rpm1, rpm2;
    int parsed = 0;

    __disable_irq();

    if (!uart_command_ready)
    {
        __enable_irq();
        return;
    }

    strncpy(command, uart_rx_buffer, sizeof(command));
    command[sizeof(command) - 1] = '\0';
    uart_command_ready = 0;

    __enable_irq();

    /* BOTH: different angles and RPMs - check first */
    if (sscanf(command, "BOTH,%f,%lu,%f,%lu",
               &angle1, &rpm1, &angle2, &rpm2) == 4)
    {
        if (angle1 > 0 && angle1 <= 25200 &&
            angle2 > 0 && angle2 <= 25200 &&
            rpm1 >= 1 && rpm1 <= 100 &&
            rpm2 >= 1 && rpm2 <= 100 &&
            motor1_done && motor2_done)
        {
            both_report_pending = 1;
            motor1_report_pending = 0;
            motor2_report_pending = 0;

            MoveBothMotors(angle1, (uint32_t)rpm1, 1,
                           angle2, (uint32_t)rpm2, 1);

            parsed = (!motor1_done && !motor2_done);

            if (!parsed)
                both_report_pending = 0;
        }
    }

    /* BOTH: same angle and RPM */
    else if (sscanf(command, "BOTH,%f,%lu",
                    &angle1, &rpm1) == 2)
    {
        angle2 = angle1;
        rpm2 = rpm1;

        if (angle1 > 0 && angle1 <= 25200 &&
            rpm1 >= 1 && rpm1 <= 100 &&
            motor1_done && motor2_done)
        {
            both_report_pending = 1;
            motor1_report_pending = 0;
            motor2_report_pending = 0;

            MoveBothMotors(angle1, (uint32_t)rpm1, 1,
                           angle2, (uint32_t)rpm2, 1);

            parsed = (!motor1_done && !motor2_done);

            if (!parsed)
                both_report_pending = 0;
        }
    }

    /* Motor 1 only */
    else if (sscanf(command, "M1,%f,%lu",
                    &angle1, &rpm1) == 2)
    {
        if (angle1 > 0 && angle1 <= 25200 &&
            rpm1 >= 1 && rpm1 <= 100 && motor1_done)
        {
            motor1_report_pending = 1;
            MoveMotor1(angle1, (uint32_t)rpm1, 1);
            parsed = !motor1_done;
        }
    }

    /* Motor 2 only */
    else if (sscanf(command, "M2,%f,%lu",
                    &angle1, &rpm1) == 2)
    {
        if (angle1 > 0 && angle1 <= 25200 &&
            rpm1 >= 1 && rpm1 <= 100 && motor2_done)
        {
            motor2_report_pending = 1;
            MoveMotor2(angle1, (uint32_t)rpm1, 1);
            parsed = !motor2_done;
        }
    }

    if (parsed)
        UART_SendMessage("OK\r\n");
    else
        UART_SendMessage("ERR\r\n");
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (!uart_command_ready)
        {
            if (uart_rx_byte == '\n')
            {
                uart_rx_buffer[uart_rx_index] = '\0';
                uart_command_ready = 1;
                uart_rx_index = 0;
            }
            else if (uart_rx_byte != '\r')
            {
                if (uart_rx_index < sizeof(uart_rx_buffer) - 1)
                {
                	uart_rx_buffer[uart_rx_index++] = (char)uart_rx_byte;
                }
                else
                {
                    uart_rx_index = 0;
                }
            }
        }

        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
}


void SetMotorSpeed(TIM_HandleTypeDef *htim, uint32_t rpm)
{
    if (rpm == 0 || rpm > 100)
        return;

    uint64_t pulse_frequency =
        ((uint64_t)rpm * PULSES_PER_REV) / 60ULL;

    uint32_t prescaler_divisor = 1U;

    if (htim->Instance == TIM3)
    {
        prescaler_divisor = 4U;  // TIM3 PSC = 3
    }

    uint32_t arr =
        (uint32_t)((TIMER_CLOCK_HZ /
        ((uint64_t)prescaler_divisor * pulse_frequency)) - 1ULL);

    uint32_t ccr = (arr + 1U) / 2U;

    __HAL_TIM_DISABLE(htim);

    __HAL_TIM_SET_COUNTER(htim, 0);
    __HAL_TIM_SET_AUTORELOAD(htim, arr);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, ccr);

    htim->Instance->EGR = TIM_EGR_UG;

    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE(htim);
}

void MoveMotor1(float angle, uint32_t rpm, uint8_t direction)
{
    if (angle <= 0.0f || rpm == 0 || rpm > 100 || !motor1_done)
        return;

    motor1_target_pulses =
        (uint32_t)((angle * PULSES_PER_REV / 360.0f) + 0.5f);

    motor1_pulse_count = 0;
    motor1_done = 0;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1,
                      direction ? GPIO_PIN_RESET : GPIO_PIN_SET);

    SetMotorSpeed(&htim2, rpm);

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        motor1_done = 1;
        Error_Handler();
    }

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        motor1_done = 1;
        Error_Handler();
    }
}


void MoveMotor2(float angle, uint32_t rpm, uint8_t direction)
{
    if (angle <= 0.0f || rpm == 0 || rpm > 100 || !motor2_done)
        return;

    motor2_target_pulses =
        (uint32_t)((angle * PULSES_PER_REV / 360.0f) + 0.5f);

    motor2_pulse_count = 0;
    motor2_done = 0;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,
                      direction ? GPIO_PIN_RESET : GPIO_PIN_SET);
    SetMotorSpeed(&htim3, rpm);

    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);

    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        motor2_done = 1;
        Error_Handler();
    }

    if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
    {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        motor2_done = 1;
        Error_Handler();
    }
}


void MoveBothMotors(float angle1, uint32_t rpm1,
                    uint8_t direction1,
                    float angle2, uint32_t rpm2,
                    uint8_t direction2)
{
    if (!motor1_done || !motor2_done)
    {
        return;
    }

    if (angle1 <= 0.0f || angle1 > 25200.0f ||
        angle2 <= 0.0f || angle2 > 25200.0f ||
        rpm1 < 1 || rpm1 > 100 ||
        rpm2 < 1 || rpm2 > 100)
    {
        return;
    }

    MoveMotor1(angle1, rpm1, direction1);
    MoveMotor2(angle2, rpm2, direction2);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 && !motor1_done)
    {
        motor1_pulse_count++;

        if (motor1_pulse_count >= motor1_target_pulses)
        {
        	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        	HAL_TIM_Base_Stop_IT(&htim2);
        	motor1_done = 1;
        }
    }

    if (htim->Instance == TIM3 && !motor2_done)
    {
        motor2_pulse_count++;

        if (motor2_pulse_count >= motor2_target_pulses)
        {
            HAL_TIM_Base_Stop_IT(&htim3);
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);

            motor2_done = 1;
        }
    }
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        /* Stay here if a fatal error occurs */
    }
}
#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
