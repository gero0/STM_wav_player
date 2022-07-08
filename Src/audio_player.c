#include <audio_player.h>
#include <ff.h>
#include <stdint.h>
#include <stm32h7xx_hal_dac.h>
#include <stm32h7xx_hal_dac_ex.h>
#include <string.h>
#include <wavparser.h>

#define DMA_MAX_TRANSFER 65535
#define BUFFER_SIZE 2048

static void (*playback_finished_callback)() = NULL;

static volatile uint32_t data_len = 0;
static volatile uint32_t data_pos = 0;
static volatile uint32_t playing_pos = 0;
static volatile uint32_t bytes_to_transfer = 0;
static volatile uint16_t n_channels = 2;

static volatile uint8_t buf1[BUFFER_SIZE];
static volatile uint8_t buf2[BUFFER_SIZE];

static volatile uint8_t* playing_buffer = buf1;
static volatile uint8_t* reading_buffer = buf2;

static volatile enum PlayerStates player_state = PSTATUS_STOPPED;

static volatile DAC_HandleTypeDef* hdac;
static volatile TIM_HandleTypeDef* htim;

static volatile FIL current_file;
static uint32_t timer_freq = 0;

uint32_t min(uint32_t a, uint32_t b)
{
    if (a < b) {
        return a;
    }
    return b;
}

int load_bytes_stereo(uint8_t* buffer, uint32_t buflen)
{
    unsigned int bytes_read_total = 0;

    //I don't know if it's just my SD card, but it REALLY does not want to allow
    //single read ops longer than 512 bytes
    for (int i = 0; i < buflen; i += 512) {
        unsigned int bytes_r = 0;
        FRESULT res = f_read(&current_file, &buffer[i], 512, &bytes_r);
        if (res != FR_OK) {
            return res;
        }
        if (bytes_r == 0) {
            break;
        }
        bytes_read_total += bytes_r;
    }

    data_pos += bytes_read_total;

    return FR_OK;
}

int load_bytes_mono(uint8_t* buffer, uint32_t buflen)
{
    uint8_t temp_buf[buflen / 2];
    unsigned int bytes_read_total = 0;

    //I don't know if it's just my SD card, but it REALLY does not want to allow
    //single read ops longer than 512 bytes
    for (int i = 0; i < buflen / 2; i += 512) {
        unsigned int bytes_r = 0;
        FRESULT res = f_read(&current_file, &temp_buf[i], 512, &bytes_r);
        if (res != FR_OK) {
            return res;
        }
        if (bytes_r == 0) {
            break;
        }
        bytes_read_total += bytes_r;
    }

    //duplicate the sample, so identical samples play in left and right channels
    for (int i = 0; i < buflen / 2; i++) {
        buffer[2 * i] = temp_buf[i];
        buffer[(2 * i) + 1] = temp_buf[i];
    }

    data_pos += bytes_read_total;

    return FR_OK;
}

int load_bytes(uint8_t* buffer, uint32_t buflen)
{
    if (n_channels == 2) {
        return load_bytes_stereo(buffer, buflen);
    }

    return load_bytes_mono(buffer, buflen);
}

int parse_wav(WavData* wav_data)
{
    FRESULT fres = FR_OK;
    uint8_t buffer[44];

    unsigned int bytes_read = 0;
    fres = f_read(&current_file, buffer, 44, &bytes_read);

    if (fres != FR_OK) {
        return PLAYER_FS_ERROR;
    }

    if (bytes_read != 44) {
        return PLAYER_PARSE_ERR;
    }

    int res = parse_wav_header(buffer, wav_data);
    if (res != 1) {
        return PLAYER_PARSE_ERR;
    }

    //seek to the start of PCM data
    fres = f_lseek(&current_file, 20 + wav_data->fmt_chunk_size);
    if (fres != FR_OK) {
        return PLAYER_FS_ERROR;
    }

    fres = f_read(&current_file, buffer, 4, &bytes_read);
    if (fres != FR_OK) {
        return PLAYER_FS_ERROR;
    }

    if (bytes_read != 4) {
        return PLAYER_PARSE_ERR;
    }

    //make sure it's a data chunk - we don't support any extensions for now
    int r = strncmp((const char*)buffer, "data", 4);
    if (r != 0) {
        return PLAYER_PARSE_ERR;
    }

    return PLAYER_OK;
}

void reset()
{
    data_len = 0;
    data_pos = 0;
    playing_pos = 0;
    bytes_to_transfer = 0;
    n_channels = 2;
}

void player_init(DAC_HandleTypeDef* dac_handle, TIM_HandleTypeDef* timer_handle, uint32_t timer_frequency)
{
    reset();
    hdac = dac_handle;
    htim = timer_handle;
    timer_freq = timer_frequency;

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

int player_loadfile(FILINFO fileinfo)
{
    reset();

    FRESULT fres = FR_OK;

    fres = f_open(&current_file, fileinfo.fname, FA_READ);
    if (fres != FR_OK) {
        return PLAYER_FS_ERROR;
    }

    WavData wav_data;
    int r = parse_wav(&wav_data);
    if (r != PLAYER_OK) {
        return r;
    }

    if (wav_data.format != WAVE_FORMAT_PCM) {
        return PLAYER_UNSUPP_FMT;
    }

    if (wav_data.n_channels != 1 && wav_data.n_channels != 2) {
        return PLAYER_UNSUPP_CHNL;
    }

    if (wav_data.sample_rate < 8000 || wav_data.sample_rate > 48000) {
        return PLAYER_UNSUPP_SMPLRATE;
    }

    if (wav_data.bits_per_sample != 8) {
        return PLAYER_UNSUPP_BITRATE;
    }

    n_channels = wav_data.n_channels;
    data_len = wav_data.data_size;

    htim->Instance->ARR = ((timer_freq / wav_data.sample_rate) - 1);

    uint32_t bytes_read;

    //The order here is important!
    fres = load_bytes(playing_buffer, BUFFER_SIZE);
    if(fres != FR_OK){
        return PLAYER_FS_ERROR;
    }
    fres = load_bytes(reading_buffer, BUFFER_SIZE);
    if(fres != FR_OK){
        return PLAYER_FS_ERROR;
    }

    player_state = PSTATUS_READY;

    return 1;
}

void player_play()
{
    if (player_state != PSTATUS_READY) {
        return; //TODO: return some kind of error
    }

    uint32_t transfer_size = min(data_pos, BUFFER_SIZE);

    HAL_DACEx_DualStart_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)playing_buffer, transfer_size / 2, DAC_ALIGN_8B_R);
    HAL_TIM_Base_Start(htim); // DAC Timer
    bytes_to_transfer = transfer_size;
}

void player_stop()
{
    if (player_state == PSTATUS_STOPPED) {
        return; //TODO: return some kind of error
    }

    HAL_DAC_Stop_DMA(hdac, DAC_CHANNEL_1);
    HAL_TIM_Base_Stop(htim); // DAC Timer
    player_state = PSTATUS_STOPPED;

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
    if (player_state == PSTATUS_STOPPED) {
        return;
    }

    //swap buffers
    uint8_t* temp = playing_buffer;
    playing_buffer = reading_buffer;
    reading_buffer = temp;

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);

    if (n_channels == 1) {
        playing_pos += bytes_to_transfer / 2;
    } else {
        playing_pos += bytes_to_transfer;
    }

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

    FRESULT fres = load_bytes(reading_buffer, BUFFER_SIZE);
    if(fres != FR_OK){
        player_stop();
        //TODO: Add an error callback?
    }
    bytes_to_transfer = transfer_size;
}
