#include "main.h"
#include <stdint.h>

enum PlayerStates {
    PLAYING,
    STOPPED,
    READY,
};

void player_init(uint32_t (*source)(uint8_t*, uint32_t), uint32_t len, DAC_HandleTypeDef* dac_handle);
void player_play();
void player_dac_dma_callback();
