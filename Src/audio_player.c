#include "audio_player.h"
#include "stm32h7xx_hal_dac.h"
#include <stdint.h>

#define DMA_MAX_TRANSFER 65535

static uint32_t audio_len;
static uint32_t audio_pos = 0;
static uint8_t* audio_buffer;
static DAC_HandleTypeDef* hdac;

static enum PlayerStates audio_state = STOPPED;

uint32_t min(uint32_t a, uint32_t b)
{
    if (a < b) {
        return a;
    }
    return b;
}

void play(uint8_t* buffer, uint32_t len, DAC_HandleTypeDef* dac_handle)
{
    audio_buffer = buffer;
    audio_len = len;
    audio_pos = 0;
    audio_state = PLAYING;
    hdac = dac_handle;

    uint32_t transfer_size = min(audio_len, DMA_MAX_TRANSFER);

    HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)audio_buffer, transfer_size, DAC_ALIGN_8B_R);
    audio_pos += transfer_size;
}

void dac_dma_callback()
{
    if (audio_state == STOPPED) {
        return;
    }

    if (audio_pos >= audio_len - 1) {
        audio_state = STOPPED;
        return;
    }

    uint32_t bytes_left = audio_len - audio_pos;
    uint32_t transfer_size = min(bytes_left, DMA_MAX_TRANSFER);
    HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)&audio_buffer[audio_pos], transfer_size, DAC_ALIGN_8B_R);
    audio_pos += transfer_size;
}
