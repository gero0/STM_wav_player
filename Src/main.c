/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "gpio.h"
#include "spi.h"
#include "stm32h7xx_hal.h"
#include "tim.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <audio_player.h>
#include <lcd.h>
#include <stdarg.h>
#include <stdint.h>
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

/* USER CODE BEGIN PV */
#define max_files 1024

static FILINFO files[max_files];
static uint16_t last_encoder_val = 0;
static unsigned int selected_file = 0;
static unsigned int file_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void myprintf(const char* fmt, ...)
{
    static char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    unsigned int len = strlen(buffer);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, -1);
}

void display_msg(const char* line_1, const char* line_2)
{
    LCD_clear();
    LCD_position(1, 1);
    LCD_write_text(line_1, strlen(line_1));
    LCD_position(1, 2);
    LCD_write_text(line_2, strlen(line_2));
}

void display_ui()
{
    FILINFO file = files[selected_file];
    char line_1[16];
    char line_2[16];

    snprintf(line_1, 16, "%s", file.fname);

    if (player_get_state() == PSTATUS_PLAYING) {
        uint32_t filled_squares = (uint32_t)(player_get_progress() * 16);
        int i = 0;

        for (int i; i < 16; i++) {
            if (i <= filled_squares) {
                line_2[i] = '#';
            } else {
                line_2[i] = '-';
            }
        }
    } else {
        line_2[0] = '\0';
    }
    display_msg(line_1, line_2);
}

int mount_sd(FATFS* FatFs, DIR* dir)
{
    FRESULT fres = FR_OK; //Result after operations

    fres = f_mount(FatFs, "", 1); //1=mount now
    if (fres != FR_OK) {
        myprintf("f_mount error (%i)\r\n", fres);
        return 0;
    }

    fres = f_opendir(dir, "/");
    if (fres != FR_OK) {
        myprintf("Error opening directory\r\n");
        return 0;
    }

    return 1;
}

uint32_t enumerate_files(DIR* dir)
{
    FRESULT fres = FR_OK;

    file_count = 0;
    while (file_count < max_files) {
        FILINFO fnfo;
        fres = f_readdir(
            dir,
            &fnfo);

        if (fres != FR_OK || fnfo.fname[0] == 0) {
            break;
        }
        //filter to save only wav files
        char* pos = strstr(fnfo.fname, ".wav");

        if (pos == NULL) {
            continue;
        }

        files[file_count] = fnfo;
        file_count++;
    }

    return file_count;
}

void enc_up()
{
    selected_file++;
    if (selected_file >= file_count) {
        selected_file = file_count - 1;
    }
}

void enc_down()
{
    selected_file--;
    if (selected_file >= file_count) {
        selected_file = 0;
    }
}

void handle_encoder_input()
{
    int enc_val = TIM4->CNT;
    if (enc_val > last_encoder_val) {
        enc_up();
    } else if (enc_val < last_encoder_val) {
        enc_down();
    }

    last_encoder_val = enc_val;
    display_ui();
}

void start_player(FILINFO file)
{
    FRESULT fres = FR_OK;

    myprintf("Opening audio file...");
    int result = player_loadfile(file);
    if (result != PLAYER_OK) {
        if (result == PLAYER_FS_ERROR) {
            display_msg("FS ERR RESTART", "DEVICE");
            Error_Handler();
        }
        display_msg("ERR OPEN.FILE", "UNSUPP FMT");
        return;
    }
    player_play();
    display_ui();
}

void button_handler()
{
    if (player_get_state() != PSTATUS_STOPPED) {
        player_stop();
        return;
    }

    start_player(files[selected_file]);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13) {
        button_handler();
    }
}

void delay_us(uint16_t us)
{
    htim7.Instance->CNT = 0;
    while (htim7.Instance->CNT <= us) {
        //wait
    }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
    player_dac_dma_callback();
}

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

    /* Configure the system clock */
    SystemClock_Config();

    /* Configure the peripherals common clocks */
    PeriphCommonClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM6_Init();
    MX_DMA_Init();
    MX_DAC1_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();
    MX_FATFS_Init();
    MX_TIM4_Init();
    MX_TIM7_Init();
    /* USER CODE BEGIN 2 */

    HAL_TIM_Base_Start(&htim7); // Delay Timer
    HAL_TIM_Encoder_Start_IT(&htim4, TIM_CHANNEL_ALL);

    LCD_init(delay_us);

    display_msg("WAV PLAYER", "loading...");

    //DAC timer is clocked at 240MHz
    player_init(&hdac1, &htim6, 240000000);

    FATFS FatFs; //Fatfs handle
    FIL fil; //File handle
    DIR dir;

    //Give SD card some time to settle
    HAL_Delay(2000);

    int res = mount_sd(&FatFs, &dir);
    if (!res) {
        display_msg("SD ERROR", "RESTART DEVICE");
        Error_Handler();
    }

    enumerate_files(&dir);
    if (file_count == 0) {
        display_msg("No WAV files", "found on SD");
        Error_Handler();
    }

    display_ui();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        display_ui();
        HAL_Delay(1000);
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
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /** Supply configuration update enable
  */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    /** Configure the main internal regulator output voltage
  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) { }
    /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 60;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 16;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
        | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

    /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CKPER;
    PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

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
    while (1) {
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
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
