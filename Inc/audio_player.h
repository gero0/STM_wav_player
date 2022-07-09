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

/**
 * @brief Initialize audio player. Note, dac_dma_callback needs to be added to DMA IRQ.
 * 
 * @param dac_handle Handle to DAC peripheral
 * @param timer_handle Handle to Timer Peripheral used to trigger the DAC
 * @param timer_frequency Frequency of Timer usued to trigger the DAC in Hz.
 */
void player_init(DAC_HandleTypeDef* dac_handle, TIM_HandleTypeDef* timer_handle, uint32_t timer_frequency);

/**
 * @brief Load file and prepare the player to play it
 * 
 * @param fileinfo Fileinfo struct containing information about the file
 * @return int PLAYER_OK is file is ready to play, or an error. See PlayerLoadResult enum.
 */
int player_loadfile(FILINFO fileinfo);

/**
 * @brief Starts the playback of audio file
 * 
 */
void player_play();

/**
 * @brief Stops the playback of audio file
 * 
 */
void player_stop();

/**
 * @brief Returns the current state of the player. See PlayerStates enum.
 * 
 * @return enum Player state
 */
enum PlayerStates player_get_state();

/**
 * @brief Returns the position of playback. Its calculated by dividing bytes played by total number of bytes in file.
 * 
 * @return Position of playback (0.0 - 1.0 range).
 */
double player_get_progress();

/**
 * @brief Register a callback to be called when player reaches EOF
 * 
 * @param callback Pointer to callback function
 */
void player_register_stop_callback(void (*callback)());

/**
 * @brief Unegister a callback to be called when player reaches EOF
 * 
 */
void player_unregister_stop_callback();

/**
 * @brief This function MUST be called in DAC CH1 DMA Transfer Complete IRQ. in order for player to work correctly
 * 
 */
void player_dac_dma_callback();
