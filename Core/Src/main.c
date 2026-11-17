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
#include "pdm2pcm.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
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
CRC_HandleTypeDef hcrc;

I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_rx;

/* USER CODE BEGIN PV */
#define PDM_BUFFER_SIZE 2048

uint16_t pdm_buffer[PDM_BUFFER_SIZE];

volatile uint32_t dma_half_count = 0;
volatile uint32_t dma_full_count = 0;

#define PCM_BUFFER_SIZE 512

int16_t pcm_buffer[PCM_BUFFER_SIZE];


volatile uint8_t pdm_half_ready = 0;
volatile uint8_t pdm_full_ready = 0;

volatile uint32_t usb_tx_count = 0;
volatile uint32_t usb_busy_count = 0;
volatile uint32_t usb_error_count = 0;

extern volatile uint8_t usb_tx_ready;

// debug variables
volatile uint16_t pdm_debug[64];
volatile uint32_t debug_i2scfgr;
volatile uint32_t debug_i2spr;
volatile uint32_t debug_standard;
volatile uint32_t debug_register;

// clap detection variables

#define CLAP_WINDOW_SIZE       64
#define CLAP_PEAK_THRESHOLD    5898
#define CLAP_CREST_THRESHOLD  3.0f

#define CLAP_REFRACTORY_MS       100

#define PATTERN_PAUSE_MIN_MS     400
#define PATTERN_PAUSE_MAX_MS     800

#define PATTERN_QUICK_MAX_MS     400
#define PATTERN_RESET_MS        1000

uint8_t led_state = 0;
uint8_t first_clap_detected = 0;

uint32_t first_clap_time = 0;
uint32_t last_clap_time = 0;

uint32_t clap_detect_count = 0;
uint32_t double_clap_count = 0;

uint8_t clap_sequence_active = 0;
uint8_t clap_pattern_state = 0;
uint32_t pattern_last_clap_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
void Process_PDM_To_PCM(uint16_t *pdm, int16_t *pcm)
{


	for (uint32_t i = 0; i < 64; i++)
	{
	    pdm_debug[i] = pdm[i];
	}


    for (uint32_t i = 0; i < 16; i++)
    {
        PDM_Filter(&pdm[i * 64],
                   &pcm[i * 16],
                   &PDM1_filter_handler);
    }
}

uint8_t USB_Send_PCM(int16_t *pcm, uint16_t sample_count)
{
    if (usb_tx_ready == 0)
    {
        usb_busy_count++;
        return USBD_BUSY;
    }

    usb_tx_ready = 0;

    uint8_t status = CDC_Transmit_FS(
        (uint8_t *)pcm,
        sample_count * sizeof(int16_t)
    );

    if (status == USBD_OK)
    {
        usb_tx_count++;
    }
    else
    {
        usb_error_count++;
        usb_tx_ready = 1;
    }

    return status;
}

void Detect_Claps(int16_t *pcm, uint16_t sample_count);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  __HAL_RCC_CRC_CLK_ENABLE();
  CRC->CR = CRC_CR_RESET;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_CRC_Init();
  MX_PDM2PCM_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_I2S_Receive_DMA(&hi2s2,
                          pdm_buffer,
                          PDM_BUFFER_SIZE) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

      if (pdm_half_ready)
      {
          pdm_half_ready = 0;

          Process_PDM_To_PCM(&pdm_buffer[0],
                             &pcm_buffer[0]);

          Detect_Claps(&pcm_buffer[0], 256);

          USB_Send_PCM(&pcm_buffer[0], 256);
      }

      if (pdm_full_ready)
      {
          pdm_full_ready = 0;

          Process_PDM_To_PCM(&pdm_buffer[1024],
                             &pcm_buffer[256]);

          Detect_Claps(&pcm_buffer[256], 256);

          USB_Send_PCM(&pcm_buffer[256], 256);
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_CRC_DR_RESET(&hcrc);
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s2.Init.Standard = I2S_STANDARD_MSB;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_16K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */
  debug_i2scfgr = SPI2->I2SCFGR;
  debug_i2spr = SPI2->I2SPR;
  debug_standard = hi2s2.Init.Standard;
  debug_register = SPI2->I2SCFGR;
  /* USER CODE END I2S2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2)
    {
        dma_half_count++;
        pdm_half_ready = 1;
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2)
    {
        dma_full_count++;
        pdm_full_ready = 1;
    }
}

// function to detect claps


void Detect_Claps(int16_t *pcm, uint16_t sample_count)
{
    static int16_t window[CLAP_WINDOW_SIZE];
    static uint16_t fill_count = 0;

    for (uint16_t i = 0; i < sample_count; i++)
    {
        window[fill_count++] = pcm[i];

        if (fill_count == CLAP_WINDOW_SIZE)
        {
            uint32_t peak = 0;
            uint64_t sum_squares = 0;

            /* Calculate peak and sum of squares */
            for (uint16_t j = 0; j < CLAP_WINDOW_SIZE; j++)
            {
                int32_t sample = window[j];

                uint32_t magnitude =
                    (sample < 0) ? -sample : sample;

                if (magnitude > peak)
                    peak = magnitude;

                sum_squares += (uint64_t)(sample * sample);
            }

            uint64_t mean_square =
                sum_squares / CLAP_WINDOW_SIZE;

            /* Check clap thresholds */
            if ((peak >= CLAP_PEAK_THRESHOLD) &&
                ((uint64_t)peak * peak > 9ULL * mean_square))
            {
                uint32_t now = HAL_GetTick();

                /* Reject repeated detections within 100 ms */
                if ((last_clap_time == 0) ||
                    (now - last_clap_time >= CLAP_REFRACTORY_MS))
                {
                    last_clap_time = now;
                    clap_detect_count++;

                    uint32_t gap = now - pattern_last_clap_time;

                    if (clap_pattern_state == 0)
                    {
                        /* First clap */
                        clap_pattern_state = 1;
                    }
                    else if (clap_pattern_state == 1)
                    {
                        /* Second clap: must follow the pause */
                        if (gap >= PATTERN_PAUSE_MIN_MS &&
                            gap <= PATTERN_PAUSE_MAX_MS)
                        {
                            clap_pattern_state = 2;
                        }
                        else
                        {
                            /* Incorrect timing: restart */
                            clap_pattern_state = 0;
                        }
                    }
                    else if (clap_pattern_state == 2)
                    {
                        /* Third clap: must be quick */
                        if (gap <= PATTERN_QUICK_MAX_MS)
                        {
                            HAL_GPIO_TogglePin(
                                LED_GREEN_GPIO_Port,
                                LED_GREEN_Pin
                            );

                            led_state ^= 1;
                            double_clap_count++;
                        }

                        /* Pattern completed or failed */
                        clap_pattern_state = 0;
                    }

                    pattern_last_clap_time = now;
                }
            }

            /* Reset if no clap arrives for 1 second */
            if ((clap_pattern_state != 0) &&
                (HAL_GetTick() - pattern_last_clap_time >=
                 PATTERN_RESET_MS))
            {
                clap_pattern_state = 0;
            }

            /* Maintain overlapping 64-sample windows */
            for (uint16_t j = 0; j < 32; j++)
            {
                window[j] = window[j + 32];
            }

            fill_count = 32;
        }
    }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
