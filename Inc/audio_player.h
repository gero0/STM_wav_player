#include <stdint.h>
#include "main.h"

enum PlayerStates {
    PLAYING,
    STOPPED
};

void play(uint8_t* buffer, uint32_t len, DAC_HandleTypeDef* dac_handle);
void dac_dma_callback();
