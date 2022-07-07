#include "audio_player.h"
#include "stm32h7xx_hal_dac.h"
#include "stm32h7xx_hal_dac_ex.h"
#include <stdint.h>

#define DMA_MAX_TRANSFER 65535
#define BUFFER_SIZE 2048

static PlayerByteSource data_source = NULL;
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

uint32_t min(uint32_t a, uint32_t b)
{
    if (a < b) {
        return a;
    }
    return b;
}

uint32_t fetch_data(uint8_t* buffer)
{
    uint32_t bytes_read = data_source(buffer, BUFFER_SIZE);
    data_pos += bytes_read;
    return bytes_read;
}

void player_init(PlayerByteSource source, uint32_t len, DAC_HandleTypeDef* dac_handle)
{
    data_source = source;
    data_len = len;
    data_pos = 0;
    playing_pos = 0;
    bytes_to_transfer = 0;
    hdac = dac_handle;

    //The order here is important!
    fetch_data(playing_buffer);
    fetch_data(reading_buffer);

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);

    player_state = READY;
}

void player_play()
{
    if (player_state != READY) {
        return; //TODO: return some kind of error
    }

    uint32_t transfer_size = min(data_pos, BUFFER_SIZE);

    //HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size, DAC_ALIGN_8B_R);
    HAL_DACEx_DualStart_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size / 2, DAC_ALIGN_8B_R);
    bytes_to_transfer = transfer_size;
}

void player_stop()
{
    if (player_state == STOPPED) {
        return; //TODO: return some kind of error
    }

    HAL_DAC_Stop_DMA(hdac, DAC_CHANNEL_1);
    player_state = STOPPED;
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

//TODO: player stopped callback
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
        player_state = STOPPED;
        if (playback_finished_callback != NULL) {
            playback_finished_callback();
        }
        return;
    }

    uint32_t bytes_left = data_len - playing_pos;
    uint32_t transfer_size = min(bytes_left, BUFFER_SIZE);
    //HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size, DAC_ALIGN_8B_R);
    HAL_DACEx_DualStart_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size / 2, DAC_ALIGN_8B_R);
    fetch_data(reading_buffer);
    bytes_to_transfer = transfer_size;
}
