#include <string.h>
#include <wavparser.h>

uint32_t get_uint32(const uint8_t* buf)
{
    int res = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
    return res;
}

uint16_t get_uint16(const uint8_t* buf)
{
    int res = buf[0] + (buf[1] << 8);
    return res;
}

int parse_wav_header(const uint8_t* buffer, WavData* data)
{
    int r = strncmp((const char*)buffer, "RIFF", 4);
    if (r != 0) {
        return -1;
    }

    r = strncmp((const char*)&buffer[8], "WAVE", 4);
    if (r != 0) {
        return -1;
    }

    uint32_t file_size = get_uint32(&buffer[4]);

    r = strncmp((const char*)&buffer[12], "fmt", 3);
    if (r != 0) {
        return -1;
    }

    data->fmt_chunk_size = get_uint32(&buffer[16]);
    data->format = get_uint16(&buffer[20]);
    data->n_channels = get_uint16(&buffer[22]);
    data->sample_rate = get_uint32(&buffer[24]);
    data->bits_per_sample = get_uint16(&buffer[34]);
    data->data_size = get_uint32(&buffer[40]);

    return 1;
}
