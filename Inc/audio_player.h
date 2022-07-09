#include "main.h"
#include <fatfs.h>
#include <stdint.h>

enum PlayerStates {
    PSTATUS_PLAYING,
    PSTATUS_STOPPED,
    PSTATUS_READY,
};

enum PlayerLoadResult {
    PLAYER_OK = 1,
    PLAYER_UNSUPP_CHNL = 0,
    PLAYER_ERR_GENERIC = -1,
    PLAYER_UNSUPP_FMT = -2,
    PLAYER_UNSUPP_SMPLRATE = -3,
    PLAYER_UNSUPP_BITRATE = -4,
    PLAYER_PARSE_ERR = -5,
    PLAYER_FS_ERROR = -6,
};

void player_init(DAC_HandleTypeDef* dac_handle, TIM_HandleTypeDef* timer_handle, uint32_t timer_frequency);
int player_loadfile(FILINFO fileinfo);
void player_play();
void player_stop();
enum PlayerStates player_get_state();
double player_get_progress();
void player_register_stop_callback(void (*callback)());
void player_unregister_stop_callback();
void player_dac_dma_callback();
