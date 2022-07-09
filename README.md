# STM32 WAV player

Software for STM32H743 based audio player.
Supports 2-channel 8-bit PCM WAV files with max 48000Hz sampling rate.
Files are loaded from SD card connected via SPI1, and internal DAC and DMA are used to play the samples.

Uses [kiwih's SD SPI driver](https://github.com/kiwih/cubeide-sd-card) and [FATFS by ChaN](http://elm-chan.org/fsw/ff/00index_e.html).

## Compilation

```
cmake -DCMAKE_BUILD_TYPE=Release -B ./build 
cmake --build ./build
```

## Flashing

Flash ./build/DAC_DMA.elf file to your microcontroller using your favourite flashing/debugging software.
E.g. openocd+gdb.
