#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/irq.h"

#include <math.h>
#include <string.h>

#include "lib/button/button.h"
#include "lib/mpu6050/mpu6050.h"
#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/sd_card/sd_card_i.h"

void gpio_irq_callback(uint gpio, uint32_t events);

static char filename[20] = "data.txt";
volatile static int last_time_btn_a_pressed = 0;
volatile static int last_time_btn_b_pressed = 0;
volatile static int last_time_btn_sw_pressed = 0;
bool is_mounted = false;
bool is_sd_card_initialized = false;
bool is_capture_mode = false;


int main() {
    stdio_init_all();
    sleep_ms(5000);
    time_init();

    int16_t aceleracao[3], gyro[3], temp;
    bool color = true;
    ssd1306_t ssd;

    init_btns();
    init_btn(BTN_SW_PIN);
    mpu6050_init();
    init_display(&ssd);

    gpio_set_irq_enabled_with_callback(
        BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);

    while (true) {
        // Leitura dos dados do MPU6050
        mpu6050_read_raw(aceleracao, gyro, &temp);

        printf("Aceleracao: X=%d, Y=%d, Z=%d\n", aceleracao[0], aceleracao[1], aceleracao[2]);
        printf("Giroscopio: X=%d, Y=%d, Z=%d\n", gyro[0], gyro[1], gyro[2]);
        printf("Temperatura: %d\n", temp);

        // Exibição no display
        ssd1306_fill(&ssd, !color);
        ssd1306_rect(&ssd, 3, 3, 122, 60, color, !color);
        ssd1306_line(&ssd, 3, 15, 123, 15, color);
        ssd1306_line(&ssd, 3, 37, 123, 37, color);
        ssd1306_line(&ssd, 3, 49, 123, 49, color);

        ssd1306_draw_string(&ssd, "DATALOGGER", 20, 6);

        ssd1306_send_data(&ssd);
        sleep_ms(1000);
    }
}

void gpio_irq_callback(uint gpio, uint32_t events)
{
    int current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A_PIN && current_time - last_time_btn_a_pressed > 250) {
        last_time_btn_a_pressed = current_time;
        is_mounted = !is_mounted;
        if (is_mounted) {
            run_mount();
            printf("Cartão SD montado.\n");
        } else {
            run_unmount();
            printf("Cartão SD desmontado.\n");
        }
    } else if (gpio == BTN_B_PIN && current_time - last_time_btn_b_pressed > 250) {
        // Ação para o botão B
        printf("Botão B pressionado\n");
    } else if (gpio == BTN_SW_PIN && current_time - last_time_btn_sw_pressed > 250) {
        reset_usb_boot(0, 0);
    }
}
