#include "sd_card_i.h"

// Function to get the sd_card_t structure by name
sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

// Function to get the FATFS structure by name
FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

// Função para definir a data e hora do RTC
void run_setrtc()
{
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr)
    {
        printf("Missing argument\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr)
    {
        printf("Missing argument\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr)
    {
        printf("Missing argument\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr)
    {
        printf("Missing argument\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr)
    {
        printf("Missing argument\n");
        return;
    }
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr)
    {
        printf("Missing argument\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {
        .year = (int16_t)year,
        .month = (int8_t)month,
        .day = (int8_t)date,
        .dotw = 0, // 0 is Sunday
        .hour = (int8_t)hour,
        .min = (int8_t)min,
        .sec = (int8_t)sec};
    rtc_set_datetime(&t);
}

// Função para montar o cartão SD
void run_format()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr)
        printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}

// Função para montar o cartão SD
void run_mount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD ( %s ) concluído\n", pSD->pcName);
}

// Função para desmontar o cartão SD
void run_unmount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
    printf("SD ( %s ) desmontado\n", pSD->pcName);
}

// Função para obter o espaço livre no cartão SD
void run_getfree()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr)
    {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
}

// Função para listar o conteúdo de um diretório
void run_ls()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = "";
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr;
    char const *p_dir;
    if (arg1[0])
    {
        p_dir = arg1;
    }
    else
    {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr)
        {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr)
    {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0])
    {
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        if (fno.fattrib & AM_DIR)
            pcAttrib = pcDirectory;
        else if (fno.fattrib & AM_RDO)
            pcAttrib = pcReadOnlyFile;
        else
            pcAttrib = pcWritableFile;
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}

// Função para exibir o conteúdo de um arquivo no terminal
void run_cat()
{
    char *arg1 = strtok(NULL, " ");
    if (!arg1)
    {
        printf("Missing argument\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr)
    {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil))
    {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
}

// Função para salvar dados do acelerômetro e giroscópio no cartão SD
void save_data(const char *filename, int16_t aceleracao[3], int16_t gyro[3], int16_t temp)
{
    FIL file;
    FRESULT res;
    UINT bw;

    // Tenta abrir o arquivo existente para append
    res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Não foi possível abrir o arquivo para escrita. Monte o Cartao.\n");
        return;
    }

    // Se o arquivo é novo (tamanho = 0), escreve o cabeçalho
    if (f_size(&file) == 0) {
        const char *header = "Date,Time,Acel_X,Acel_Y,Acel_Z,Gyro_X,Gyro_Y,Gyro_Z,Temp\n";
        res = f_write(&file, header, strlen(header), &bw);
        if (res != FR_OK) {
            printf("[ERRO] Não foi possível escrever cabeçalho no arquivo.\n");
            f_close(&file);
            return;
        }
    } else {
        // Se o arquivo já existe, posiciona o ponteiro no fim
        f_lseek(&file, f_size(&file));
    }

    // Obter data e hora atual para timestamp
    datetime_t dt;
    char datetime_str[30];

    if (rtc_get_datetime(&dt)) {
        sprintf(datetime_str, "%04d-%02d-%02d,%02d:%02d:%02d",
                dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    } else {
        strcpy(datetime_str, "0000-00-00,00:00:00");  // Fallback se o RTC não estiver disponível
    }

    // Formata os dados para o arquivo CSV
    char buffer[100];
    sprintf(buffer, "%s,%d,%d,%d,%d,%d,%d\n",
            datetime_str,
            aceleracao[0], aceleracao[1], aceleracao[2],
            gyro[0], gyro[1], gyro[2]);

    // Escreve no arquivo
    res = f_write(&file, buffer, strlen(buffer), &bw);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível escrever no arquivo. Monte o Cartao.\n");
        f_close(&file);
        return;
    }

    // Fecha o arquivo e confirma que os dados foram salvos
    f_close(&file);
    printf("\nDados dos sensores salvos no arquivo %s.\n", filename);
    printf("Acelerometro: X=%d, Y=%d, Z=%d\nGiroscopio: X=%d, Y=%d, Z=%d\nTemperatura: %d\n",
           aceleracao[0], aceleracao[1], aceleracao[2],
           gyro[0], gyro[1], gyro[2], temp);
}

// Função para ler o conteúdo de um arquivo e exibir no terminal de forma formatada
void read_file(const char *filename)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o Cartão está montado ou se o arquivo existe.\n");
        return;
    }

    char buffer[128];
    UINT br;
    int line_count = 0;
    bool header_processed = false;

    printf("\n==== Leitura de Dados: %s ====\n\n", filename);

    // Ler linha por linha
    while (f_gets(buffer, sizeof(buffer), &file))
    {
        line_count++;

        // Processar cabeçalho (primeira linha)
        if (line_count == 1) {
            printf("CABEÇALHO: %s", buffer);
            printf("----------------------------------------\n");
            header_processed = true;
            continue;
        }

        // Processar linhas de dados
        if (header_processed) {
            // Variáveis para armazenar os dados extraídos
            char date[11], time[9];
            int acel_x, acel_y, acel_z;
            int gyro_x, gyro_y, gyro_z;

            // Analisar a linha CSV
            // Formato esperado: yyyy-mm-dd,hh:mm:ss,acel_x,acel_y,acel_z,gyro_x,gyro_y,gyro_z
            int result = sscanf(buffer, "%10[^,],%8[^,],%d,%d,%d,%d,%d,%d",
                        date, time, &acel_x, &acel_y, &acel_z,
                        &gyro_x, &gyro_y, &gyro_z);

            if (result == 8) {
                // Exibe os dados formatados
                printf("Registro #%d - Data/Hora: %s %s\n", line_count-1, date, time);
                printf("  Acelerômetro: X=%6d, Y=%6d, Z=%6d\n", acel_x, acel_y, acel_z);
                printf("  Giroscópio:   X=%6d, Y=%6d, Z=%6d\n", gyro_x, gyro_y, gyro_z);
                printf("----------------------------------------\n");
            } else {
                // Linha não corresponde ao formato esperado
                printf("Formato inválido na linha %d: %s", line_count, buffer);
            }
        }
    }

    f_close(&file);

    // Estatísticas de leitura
    printf("\nTotal de %d registros lidos do arquivo.\n", line_count - 1);
    printf("==== Leitura concluída ====\n\n");
}
