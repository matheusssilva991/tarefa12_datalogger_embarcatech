#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/irq.h"
#include <math.h>                    // Para as funções trigonométricas

#include "lib/button/button.h"
#include "lib/mpu6050/mpu6050.h"
#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"

void gpio_irq_callback(uint gpio, uint32_t events);

int main()
{
    stdio_init_all();
    init_btns();
    init_btn(BTN_SW_PIN);
    mpu6050_init();

    ssd1306_t ssd;
    init_display(&ssd); // Inicializa o display SSD1306

    gpio_set_irq_enabled_with_callback(
        BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);

    int16_t aceleracao[3], gyro[3], temp;
    bool color = true;

    while (true) {
        // Leitura dos dados de aceleração, giroscópio e temperatura
        mpu6050_read_raw(aceleracao, gyro, &temp);

        // Conversão para unidade de 'g'
        float ax = aceleracao[0] / 16384.0f;
        float ay = aceleracao[1] / 16384.0f;
        float az = aceleracao[2] / 16384.0f;

        // Cálculo dos ângulos em graus (Roll e Pitch)
        float roll  = atan2(ay, az) * 180.0f / M_PI;
        float pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0f / M_PI;

        // Montagem das strings para o display
        char str_roll[20];
        char str_pitch[20];


        snprintf(str_roll,  sizeof(str_roll),  "%5.1f", roll);
        snprintf(str_pitch, sizeof(str_pitch), "%5.1f", pitch);


        // Exibição no display
        ssd1306_fill(&ssd, !color);                            // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, color, !color);        // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, color);             // Desenha uma linha horizontal
        ssd1306_line(&ssd, 3, 37, 123, 37, color);             // Desenha outra linha horizontal
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);   // Escreve texto no display
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);    // Escreve texto no display
        ssd1306_draw_string(&ssd, "IMU    MPU6050", 10, 28); // Escreve texto no display
        ssd1306_line(&ssd, 63, 35, 63, 60, color);             // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, "roll", 14, 41);           // Escreve texto no display
        ssd1306_draw_string(&ssd, str_roll, 14, 52);         // Exibe Roll
        ssd1306_draw_string(&ssd, "pitch", 73, 41);           // Escreve texto no display
        ssd1306_draw_string(&ssd, str_pitch, 73, 52);        // Exibe Pitch
        ssd1306_send_data(&ssd);
        sleep_ms(500);
    }
}

void gpio_irq_callback(uint gpio, uint32_t events)
{
    if (gpio == BTN_A_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        reset_usb_boot(0, 0); // Reset the Pico to bootloader mode
    }
}
