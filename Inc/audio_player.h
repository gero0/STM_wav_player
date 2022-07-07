#include "main.h"
#include <fatfs.h>
#include <stdint.h>

enum PlayerStates {
    PLAYING,
    STOPPED,
    READY,
};

void player_init(DAC_HandleTypeDef* dac_handle, TIM_HandleTypeDef* timer_handle);
int player_loadfile(FILINFO fileinfo);
void player_play();
void player_stop();
enum PlayerStates player_get_state();
void player_register_stop_callback(void (*callback)());
void player_unregister_stop_callback();
void player_dac_dma_callback();
