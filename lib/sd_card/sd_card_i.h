#ifndef SD_CARD_I_H
#define SD_CARD_I_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include "hardware/rtc.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

sd_card_t *sd_get_by_name(const char *const name);
FATFS *sd_get_fs_by_name(const char *name);
bool run_setrtc(const char *datetime_str);
void run_format();
void run_mount();
void run_unmount();
void run_getfree();
void run_ls();
void run_cat();

// Função para capturar dados do ADC e salvar no arquivo *.txt
void save_data(const char *filename, int16_t aceleracao[3], int16_t gyro[3], float temp_celsius);

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *filename);

#endif  // SD_CARD_I_H
