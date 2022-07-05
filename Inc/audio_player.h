#include "main.h"
#include <stdint.h>

enum PlayerStates {
    PLAYING,
    STOPPED,
    READY,
};

typedef uint32_t (*PlayerByteSource)(uint8_t*, uint32_t);

void player_init(PlayerByteSource source, uint32_t len, DAC_HandleTypeDef* dac_handle);
void player_play();
void player_stop();
enum PlayerStates player_get_state();
void player_dac_dma_callback();
