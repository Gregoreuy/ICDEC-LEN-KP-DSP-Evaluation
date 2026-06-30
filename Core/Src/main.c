/* USER CODE BEGIN Header */
/* ============================================================================
 * Voice Activation Project - Audio Acquisition Stage
 * Board   : STM32U5A9J-DK
 * Mic     : INMP441 (I2S digital MEMS microphone)
 * Path    : SAI1 (I2S RX, 32-bit DMA Circular) -> extract 16-bit MSB ->
 *           USART1 (TX, DMA Normal) -> PC
 * ============================================================================
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "gpdma.h"
#include "hspi.h"
#include "i2c.h"
#include "icache.h"
#include "octospi.h"
#include "sai.h"
#include "sdmmc.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HALF_BUFFER_SAMPLES   1600U                      /* ~96ms @ 16.666kHz, 16-bit mono */
#define FULL_BUFFER_SAMPLES   (HALF_BUFFER_SAMPLES * 2U)  /* total ping-pong buffer */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Buffer RAW dari SAI1 RX DMA, 32-bit per sample (sesuai SAI Data Size 32 Bits).
 * Tiap word berisi: 24-bit data audio INMP441 (MSB-first) + 8-bit padding nol di LSB.
 * Index 0 .. HALF_BUFFER_SAMPLES-1   -> "half pertama"
 * Index HALF_BUFFER_SAMPLES .. end   -> "half kedua"
 */
static uint32_t audioBufferRaw[FULL_BUFFER_SAMPLES];

/* Buffer hasil ekstraksi 16-bit MSB dari audioBufferRaw, yang akan dikirim
 * ke UART. Dipakai ulang (reuse) untuk half pertama maupun half kedua,
 * jadi cukup berukuran HALF_BUFFER_SAMPLES saja (bukan FULL). */
static uint16_t audioBuffer16[HALF_BUFFER_SAMPLES];

/* Flag untuk menandai buffer mana yang siap diproses & dikirim.
 * Diset di callback ISR, dibaca/diproses di main loop. */
static volatile uint8_t firstHalfReady  = 0;
static volatile uint8_t secondHalfReady = 0;

/* Flag status pengiriman UART, supaya kita tidak menumpuk
 * panggilan HAL_UART_Transmit_DMA sebelum transfer sebelumnya selesai. */
static volatile uint8_t uartTxBusy = 0;

/* Counter untuk deteksi overrun (buffer berikutnya siap sebelum
 * buffer sebelumnya selesai dikirim -> data akan ke-skip) */
static volatile uint32_t overrunCount = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */
static void ExtractAndSendHalfBuffer(uint32_t *rawData, uint16_t sizeInSamples);
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

  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ADC4_Init();
  MX_HSPI1_Init();
  MX_I2C3_Init();
  MX_I2C5_Init();
  MX_ICACHE_Init();
  MX_OCTOSPI1_Init();
  MX_SAI1_Init();
  MX_SDMMC1_MMC_Init();
  MX_USART1_UART_Init();
  MX_USB_OTG_HS_HCD_Init();
  /* USER CODE BEGIN 2 */
  /* Mulai akuisisi audio dari INMP441 via SAI1, mode DMA circular, 32-bit.
   * Karena Circular Mode sudah diaktifkan di CubeMX, panggilan ini
   * HANYA SEKALI di awal -- DMA akan otomatis looping terus menerus.
   * Catatan: parameter ketiga HAL_SAI_Receive_DMA tetap dalam satuan SAMPLE
   * (bukan byte), HAL akan otomatis kalikan dengan lebar data (32-bit/4 byte)
   * sesuai konfigurasi SAI Data Size. */
  if (HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t *)audioBufferRaw, FULL_BUFFER_SAMPLES) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* Cek flag dari callback ISR, proses di main loop (bukan di ISR)
       * supaya ISR tetap ringan dan cepat. */
      if (firstHalfReady)
      {
          firstHalfReady = 0;
          ExtractAndSendHalfBuffer(&audioBufferRaw[0], HALF_BUFFER_SAMPLES);
      }

      if (secondHalfReady)
      {
          secondHalfReady = 0;
          ExtractAndSendHalfBuffer(&audioBufferRaw[HALF_BUFFER_SAMPLES], HALF_BUFFER_SAMPLES);
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = 8;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
  HAL_PWREx_DisableUCPDDeadBattery();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

/* ============================================================================
 * Fungsi bantu: ekstrak 16-bit MSB dari tiap sample 32-bit raw SAI,
 * lalu kirim hasil ekstraksi (16-bit) ke USART1 via DMA.
 *
 * Kenapa >> 16: data INMP441 24-bit MSB-first ditempatkan di bit [31:8]
 * dari word 32-bit (bit [7:0] adalah padding nol). Mengambil 16 bit
 * TERATAS (bit [31:16]) memberi kita representasi 16-bit yang paling
 * signifikan dari data audio asli -- setara dengan "downsample" presisi
 * dari 24-bit ke 16-bit dengan cara membuang bit-bit paling tidak
 * signifikan (LSB), bukan memotong di posisi sembarang.
 *
 * Jika UART masih sibuk mengirim buffer sebelumnya, data ini akan
 * di-skip dan overrunCount bertambah.
 * ============================================================================
 */
static void ExtractAndSendHalfBuffer(uint32_t *rawData, uint16_t sizeInSamples)
{
    if (uartTxBusy)
    {
        overrunCount++;
        return;
    }

    /* Ekstraksi: ambil 16-bit MSB dari tiap word 32-bit */
    for (uint16_t i = 0; i < sizeInSamples; i++)
    {
        audioBuffer16[i] = (uint16_t)(rawData[i] >> 16);
    }

    uartTxBusy = 1;
    /* sizeInSamples * 2 karena tiap sample HASIL EKSTRAKSI 16-bit = 2 byte,
     * dan HAL_UART_Transmit_DMA bekerja dalam satuan byte. Buffer yang
     * dikirim adalah audioBuffer16 (hasil ekstraksi), BUKAN rawData. */
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)audioBuffer16, sizeInSamples * 2U) != HAL_OK)
    {
        uartTxBusy = 0;
        Error_Handler();
    }
}

/* ============================================================================
 * Callback: dipanggil otomatis oleh HAL saat separuh PERTAMA buffer
 * SAI RX selesai diisi DMA (event "Half Transfer").
 * ============================================================================
 */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance == hsai_BlockA1.Instance)
    {
        firstHalfReady = 1;
    }
}

/* ============================================================================
 * Callback: dipanggil otomatis oleh HAL saat buffer SAI RX selesai
 * penuh terisi (event "Transfer Complete"). Karena Circular Mode,
 * DMA otomatis lanjut menulis ulang dari awal buffer (half pertama)
 * setelah ini.
 * ============================================================================
 */
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance == hsai_BlockA1.Instance)
    {
        secondHalfReady = 1;
    }
}

/* ============================================================================
 * Callback: dipanggil otomatis oleh HAL saat transfer USART1 TX (DMA)
 * selesai sepenuhnya. Reset flag busy supaya half-buffer berikutnya
 * boleh diekstrak & dikirim.
 * ============================================================================
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == huart1.Instance)
    {
        uartTxBusy = 0;
    }
}

/* ============================================================================
 * Callback error handler untuk SAI. Berguna untuk debugging jika ada
 * overrun di sisi SAI/DMA RX sendiri (berbeda dari overrunCount di atas
 * yang menghitung sisi UART TX).
 * ============================================================================
 */
void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai->Instance == hsai_BlockA1.Instance)
    {
        /* Letakkan breakpoint di sini saat debugging untuk
         * mengetahui apakah SAI mengalami overrun/error lain. */
        __NOP();
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
