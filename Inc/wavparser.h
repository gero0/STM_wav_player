#include <stdint.h>
#include <stdio.h>

typedef enum {
    WAVE_FORMAT_PCM = 1,
    WAVE_FORMAT_IEEE_FLOAT = 3,
    WAVE_FORMAT_ALAW = 6,
    WAVE_FORMAT_MULAW = 7,
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
} WaveFormat;

typedef struct {
    uint32_t fmt_chunk_size;
    WaveFormat format;
    uint16_t n_channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint32_t data_size;
} WavData;

int parse_wav_header(const uint8_t* buffer, WavData* data);
