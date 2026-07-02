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
#include <stdio.h>
#include "kws_inference.hpp"
#include "kws_test_sample.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HALF_BUFFER_SAMPLES   1600U                      /* ~96ms @ 16.666kHz, 16-bit mono */
#define FULL_BUFFER_SAMPLES   (HALF_BUFFER_SAMPLES * 2U)  /* total ping-pong buffer */

/* WAJIB sama dengan EI_CLASSIFIER_RAW_SAMPLE_COUNT di model_metadata.h (16000).
 * HALF_BUFFER_SAMPLES (1600) x 10 = KWS_WINDOW_SAMPLES (16000) secara PAS,
 * jadi 10 kali event half-buffer = 1 window genap tanpa sisa. Kalau model
 * diganti/retrain dan RAW_SAMPLE_COUNT berubah, angka ini HARUS disesuaikan
 * manual juga (sama seperti KWS_TEST_SAMPLE_LENGTH). */
#define KWS_WINDOW_SAMPLES    16000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static volatile uint32_t halfCpltCount = 0;
static volatile uint32_t fullCpltCount = 0;

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

/* --- Buffer akumulasi window inference KWS: DOUBLE-BUFFER ---
 * Dua slot window 16000 sample. ISR selalu punya satu buffer "aktif"
 * untuk diisi (kwsFillBuffer), sementara buffer sisi lain (kalau ada
 * yang sudah penuh & belum selesai diproses main loop) ditandai lewat
 * kwsReadyBuffer + kwsWindowReady. Ini menghilangkan blind spot dari
 * desain lama (single buffer + skip total selama run_classifier()
 * berjalan) -- ISR tidak pernah berhenti merekam.
 *
 * Asumsi yang mendasari desain ini: durasi run_classifier() (~450ms,
 * terukur konsisten) LEBIH PENDEK dari durasi akumulasi 1 window
 * (~1000ms @ 16.2kHz). Selama asumsi ini benar, buffer yang sedang
 * diproses main loop selalu sudah selesai SEBELUM buffer sisi lain
 * penuh lagi, jadi tidak akan pernah terjadi tabrakan. Kalau asumsi
 * ini ternyata dilanggar, lihat kwsDoubleBufferOverrunCount di bawah.
 */
static int16_t kwsAccumBuffer[2][KWS_WINDOW_SAMPLES];
static volatile uint32_t kwsAccumIndex = 0;
static volatile uint8_t kwsFillBuffer = 0;       /* buffer yang sedang diisi ISR: 0 atau 1 */
static volatile uint8_t kwsReadyBuffer = 0xFFU;  /* buffer siap diproses main loop; 0xFF = tidak ada */
static volatile uint8_t kwsWindowReady = 0;

/* Safety counter: SEHARUSNYA TETAP 0 selama asumsi timing di atas benar.
 * Kalau nilainya naik, berarti main loop belum selesai memproses buffer
 * lama saat buffer satunya sudah penuh lagi -- tanda asumsi timing
 * dilanggar dan desain ini perlu dievaluasi ulang (jangan diabaikan). */
static volatile uint32_t kwsDoubleBufferOverrunCount = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */
static void ExtractAndSendHalfBuffer(uint32_t *rawData, uint16_t sizeInSamples);
static void ExtractAndAccumulateForKWS(uint32_t *rawData, uint16_t sizeInSamples);
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
  MX_SDMMC1_MMC_Init();
  MX_USART1_UART_Init();
  MX_USB_OTG_HS_HCD_Init();
  MX_SAI1_Init();
  /* USER CODE BEGIN 2 */

  /* --- STATIC TEST: validasi integrasi Edge Impulse SDK, TIDAK depend ke mic ---
   * Dijalankan SEKALI di sini, sebelum audio real (SAI DMA) distart.
   * Tujuannya murni memastikan run_classifier()/signal_t/dst di
   * kws_inference.cpp sudah terhubung & jalan benar, terlepas dari
   * status mic yang sedang rusak.
   *
   * kws_test_sample saat ini masih PLACEHOLDER (isi nol semua) di
   * kws_test_sample.c -- ganti dengan "Raw Features" asli dari Edge
   * Impulse Studio (Live Classification) sebelum menyimpulkan apa pun
   * dari hasil klasifikasinya.
   *
   * Hasil klasifikasi (semua label + confidence) dicetak via ei_printf
   * -> UART1. kws_test_result berisi index label dengan confidence
   * tertinggi -- bisa juga diinspeksi langsung lewat Live Expressions
   * dengan breakpoint di baris (void)kws_test_result di bawah.
   */
  KWS_Init();
  int kws_test_result = KWS_RunInference((int16_t *)kws_test_sample, KWS_TEST_SAMPLE_LENGTH, "STATIC_TEST");
  (void)kws_test_result;

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
      /* --- Streaming audio mentah via UART: DINONAKTIFKAN SEMENTARA ---
       * Alasan: ExtractAndSendHalfBuffer pakai HAL_UART_Transmit_DMA
       * (non-blocking) di UART1, sementara ei_printf (di kws_inference.cpp)
       * pakai HAL_UART_Transmit (blocking) di UART1 YANG SAMA. Dua mode
       * transmit berbeda pada handle UART yang sama ini saling konflik
       * kalau jalan bersamaan -- hasilnya data biner audio dan teks log
       * saling bertabrakan/corrupt di level UART (ini penyebab output
       * "kotak-kotak acak" yang terlihat di Tera Term).
       *
       * Aktifkan lagi blok ini HANYA kalau memang perlu debug audio raw,
       * dan pastikan ei_printf/KWS logging dimatikan dulu saat itu supaya
       * tidak konflik lagi.
       */
#if 0
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
#else
      /* Tetap clear flag supaya tidak menumpuk tanpa arti, walau data
       * tidak dikirim ke UART. */
      firstHalfReady = 0;
      secondHalfReady = 0;
#endif

      /* Window akumulasi 16000 sample sudah penuh -> jalankan inference.
       * Desain double-buffer: ISR TIDAK PERNAH berhenti merekam selama
       * ini berjalan (beda dari desain lama yang skip total). bufIdx
       * di-copy ke variabel lokal supaya nilainya stabil selama
       * KWS_RunInference() blocking, walau secara desain kwsReadyBuffer
       * baru boleh berubah lagi setelah kwsWindowReady di-clear. */
      if (kwsWindowReady)
      {
          uint8_t bufIdx = kwsReadyBuffer;
          int kws_result_idx = KWS_RunInference(kwsAccumBuffer[bufIdx], KWS_WINDOW_SAMPLES, "REALTIME");
          kwsWindowReady = 0;

          /* Toggle LED berdasarkan label hasil inference. Urutan label
           * SUDAH terkonfirmasi dari model_metadata.h: index 0 = "no",
           * 1 = "noise", 2 = "unknown", 3 = "yes". Label "noise"/"unknown"
           * sengaja diabaikan (tidak toggle LED apa pun). */
          if (kws_result_idx == 0)      /* "no" */
          {
              HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
          }
          else if (kws_result_idx == 3) /* "yes" */
          {
              HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
          }
      }

      /* Debug: laporkan sekali tiap kali overrun counter berubah.
       * Idealnya baris ini TIDAK PERNAH muncul di UART -- kalau muncul,
       * berarti asumsi timing (run_classifier() < durasi 1 window)
       * ternyata dilanggar dan desain double-buffer ini perlu
       * dievaluasi ulang (jangan diabaikan). */
      {
          /* Catatan: pakai HAL_UART_Transmit langsung (bukan ei_printf),
           * karena main.c dikompilasi sebagai plain C sedangkan ei_printf
           * punya linkage C++ (lihat catatan lama soal fix conflicting
           * linkage) -- tidak bisa dipanggil langsung dari file C. */
          static uint32_t lastReportedOverrun = 0;
          if (kwsDoubleBufferOverrunCount != lastReportedOverrun)
          {
              char overrunMsg[64];
              int msgLen = snprintf(overrunMsg, sizeof(overrunMsg),
                  "[REALTIME] WARNING: double-buffer overrun count = %lu\r\n",
                  (unsigned long)kwsDoubleBufferOverrunCount);
              HAL_UART_Transmit(&huart1, (uint8_t *)overrunMsg, (uint16_t)msgLen, HAL_MAX_DELAY);
              lastReportedOverrun = kwsDoubleBufferOverrunCount;
          }
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
 * Fungsi bantu KWS: ekstrak 16-bit MSB dari half-buffer raw SAI (logika
 * ekstraksi SAMA seperti ExtractAndSendHalfBuffer di atas), lalu TARUH
 * hasilnya ke buffer akumulasi window (DOUBLE-BUFFER) -- BUKAN dikirim
 * ke UART.
 *
 * PENTING: fungsi ini dipanggil dari DALAM ISR (HAL_SAI_RxHalfCpltCallback /
 * HAL_SAI_RxCpltCallback), BUKAN dari main loop. Ini sengaja, supaya
 * ekstraksi & akumulasi selalu terjadi tepat waktu (~setiap 96ms) TIDAK
 * PEDULI seberapa lama KWS_RunInference()/run_classifier() blocking di
 * main loop.
 *
 * Dibuat sebagai fungsi TERPISAH dari ExtractAndSendHalfBuffer (bukan
 * digabung) dengan alasan:
 * 1. Jalur UART (ExtractAndSendHalfBuffer) tetap di main loop -- boleh
 *    di-skip kalau UART busy, tidak masalah (cuma debug stream)
 * 2. Jalur KWS ini WAJIB tidak pernah kehilangan audio -- makanya
 *    ditaruh di ISR dan sekarang pakai double-buffer: begitu satu
 *    buffer penuh, ISR langsung pindah isi ke buffer sisi lain TANPA
 *    jeda/skip, sambil buffer yang baru penuh menunggu diambil main loop
 * 3. Keduanya sama-sama hanya MEMBACA audioBufferRaw, jadi aman dipanggil
 *    dari dua tempat berbeda (ISR vs main loop) tanpa race condition
 *
 * Kondisi kwsWindowReady masih true saat buffer satunya baru penuh lagi
 * SEHARUSNYA tidak pernah terjadi selama asumsi timing (run_classifier()
 * ~450ms < durasi 1 window ~1000ms) berlaku -- kalau terjadi, dicatat di
 * kwsDoubleBufferOverrunCount, dan buffer yang belum sempat diambil main
 * loop TIDAK ditimpa (window baru yang penuh berikutnya akan menunggu).
 *
 * CATATAN: karena dipanggil dari ISR, badan fungsi ini harus tetap cepat
 * (cuma loop copy 1600 sample + sedikit logika buffer swap, tidak ada
 * panggilan blocking apa pun).
 * ============================================================================
 */
static void ExtractAndAccumulateForKWS(uint32_t *rawData, uint16_t sizeInSamples)
{
    for (uint16_t i = 0; i < sizeInSamples; i++)
    {
        kwsAccumBuffer[kwsFillBuffer][kwsAccumIndex + i] = (int16_t)(rawData[i] >> 16);
    }
    kwsAccumIndex += sizeInSamples;

    if (kwsAccumIndex >= KWS_WINDOW_SAMPLES)
    {
        kwsAccumIndex = 0;

        if (kwsWindowReady)
        {
            /* Main loop belum sempat ambil window sebelumnya. Seharusnya
             * tidak pernah terjadi -- catat kejadiannya, jangan timpa
             * buffer yang masih ditandai ready. */
            kwsDoubleBufferOverrunCount++;
        }
        else
        {
            kwsReadyBuffer = kwsFillBuffer;
            kwsWindowReady = 1;
        }

        /* Pindah ke buffer sisi lain, TANPA jeda -- ISR langsung lanjut
         * akumulasi window berikutnya di buffer yang tidak sedang dibaca
         * main loop. Ini bedanya dari desain lama (skip total). */
        kwsFillBuffer = (kwsFillBuffer == 0U) ? 1U : 0U;
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
        halfCpltCount++;
        firstHalfReady = 1;
        ExtractAndAccumulateForKWS(&audioBufferRaw[0], HALF_BUFFER_SAMPLES);
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
        fullCpltCount++;
        secondHalfReady = 1;
        ExtractAndAccumulateForKWS(&audioBufferRaw[HALF_BUFFER_SAMPLES], HALF_BUFFER_SAMPLES);
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
