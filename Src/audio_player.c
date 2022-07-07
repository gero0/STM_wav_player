#include "audio_player.h"
#include "ff.h"
#include "stm32h7xx_hal_dac.h"
#include "stm32h7xx_hal_dac_ex.h"
#include <stdint.h>

#define DMA_MAX_TRANSFER 65535
#define BUFFER_SIZE 2048

static void (*playback_finished_callback)() = NULL;

static volatile uint32_t data_len = 0;
static volatile uint32_t data_pos = 0;
static volatile uint32_t playing_pos = 0;
static volatile uint32_t bytes_to_transfer = 0;

static volatile uint8_t buf1[BUFFER_SIZE];
static volatile uint8_t buf2[BUFFER_SIZE];

static volatile uint8_t* playing_buffer = buf1;
static volatile uint8_t* reading_buffer = buf2;

static volatile enum PlayerStates player_state = STOPPED;

static volatile DAC_HandleTypeDef* hdac;
static volatile TIM_HandleTypeDef* htim;

static volatile FIL current_file;

uint32_t min(uint32_t a, uint32_t b)
{
    if (a < b) {
        return a;
    }
    return b;
}

uint32_t load_bytes(uint8_t* buffer)
{
    unsigned int bytes_read_total = 0;

    for (int i = 0; i < BUFFER_SIZE; i += 512) {
        unsigned int bytes_read = 0;
        FRESULT res = f_read(&current_file, &buffer[i], 512, &bytes_read);
        if (bytes_read == 0) {
            break;
        }
        bytes_read_total += bytes_read;
    }

    data_pos += bytes_read_total;
    return bytes_read_total;
}

void reset()
{
    data_len = 0;
    data_pos = 0;
    playing_pos = 0;
    bytes_to_transfer = 0;
}

void player_init(DAC_HandleTypeDef* dac_handle, TIM_HandleTypeDef* timer_handle)
{
    reset();
    hdac = dac_handle;
    htim = timer_handle;

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

int player_loadfile(FILINFO fileinfo)
{

    reset();
    data_len = fileinfo.fsize;

    FRESULT fres = FR_OK;

    fres = f_open(&current_file, fileinfo.fname, FA_READ);
    if (fres != FR_OK) {
        return 0;
    }

    //The order here is important!
    load_bytes(playing_buffer);
    load_bytes(reading_buffer);

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);

    player_state = READY;

    return 1;
}

void player_play()
{
    if (player_state != READY) {
        return; //TODO: return some kind of error
    }

    uint32_t transfer_size = min(data_pos, BUFFER_SIZE);

    HAL_DACEx_DualStart_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size / 2, DAC_ALIGN_8B_R);
    HAL_TIM_Base_Start(htim); // DAC Timer
    bytes_to_transfer = transfer_size;
}

void player_stop()
{
    if (player_state == STOPPED) {
        return; //TODO: return some kind of error
    }

    HAL_DAC_Stop_DMA(hdac, DAC_CHANNEL_1);
    HAL_TIM_Base_Stop(htim); // DAC Timer
    player_state = STOPPED;

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);

    f_close(&current_file);
}

enum PlayerStates player_get_state()
{
    return player_state;
}

void player_register_stop_callback(void (*callback)())
{
    playback_finished_callback = callback;
}

void player_unregister_stop_callback()
{
    playback_finished_callback = NULL;
}

void player_dac_dma_callback()
{
    if (player_state == STOPPED) {
        return;
    }

    //swap buffers
    uint8_t* temp = playing_buffer;
    playing_buffer = reading_buffer;
    reading_buffer = temp;

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);

    playing_pos += bytes_to_transfer;

    if (playing_pos >= data_len - 1) {
        player_stop();
        if (playback_finished_callback != NULL) {
            playback_finished_callback();
        }
        return;
    }

    uint32_t bytes_left = data_len - playing_pos;
    uint32_t transfer_size = min(bytes_left, BUFFER_SIZE);

    HAL_DACEx_DualStart_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size / 2, DAC_ALIGN_8B_R);
    load_bytes(reading_buffer);
    bytes_to_transfer = transfer_size;
}
