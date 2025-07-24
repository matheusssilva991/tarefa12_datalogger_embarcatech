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
#include "lib/led/led.h"

void gpio_irq_callback(uint gpio, uint32_t events);
void update_led_state();

static char filename[20] = "data.txt";
volatile static int64_t last_time_btn_a_pressed = 0;
volatile static int64_t last_time_btn_b_pressed = 0;
volatile static int64_t last_time_btn_sw_pressed = 0;
bool is_mounted = false;
bool is_capture_mode = false;


int main() {
    stdio_init_all();
    time_init();

    int16_t aceleracao[3], gyro[3], temp;
    bool color = true;
    ssd1306_t ssd;
    bool current_mounted = false;

    init_btns();
    init_btn(BTN_SW_PIN);
    init_leds();
    mpu6050_init();
    init_display(&ssd);

    gpio_set_irq_enabled_with_callback(
        BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_SW_PIN, GPIO_IRQ_EDGE_FALL, true);

    char status_str[20];

    while (true) {
        if (current_mounted != is_mounted) {
            current_mounted = is_mounted;
            if (is_mounted) {
                printf("Montando o cartão SD...\n");
                run_mount();  // Agora correto
            } else {
                printf("Desmontando o cartão SD...\n");
                run_unmount();  // Agora correto
            }
        }

        // Leitura dos dados do MPU6050
        mpu6050_read_raw(aceleracao, gyro, &temp);

        // Exibição no display
        ssd1306_fill(&ssd, !color);
        ssd1306_rect(&ssd, 3, 3, 122, 60, color, !color);
        ssd1306_line(&ssd, 3, 15, 123, 15, color);
        ssd1306_line(&ssd, 3, 37, 123, 37, color);
        ssd1306_line(&ssd, 3, 49, 123, 49, color);

        ssd1306_draw_string(&ssd, "DATALOGGER", 20, 6);

        if (is_mounted) {
            if (is_capture_mode) {
                snprintf(status_str, sizeof(status_str), "Capturando...");
            } else {
                snprintf(status_str, sizeof(status_str), "SD Montado");
            }
        } else {
            snprintf(status_str, sizeof(status_str), "SD Desmontado");
        }
        ssd1306_draw_string(&ssd, status_str, 10, 40);
        ssd1306_send_data(&ssd);

        // Captura e salva os dados no cartão SD
        if (is_capture_mode && is_mounted) {
            save_data(filename, aceleracao, gyro, temp);
        }


        update_led_state();
        sleep_ms(500);
    }
}

void gpio_irq_callback(uint gpio, uint32_t events)
{
    int64_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == BTN_A_PIN && current_time - last_time_btn_a_pressed > 290) {
        last_time_btn_a_pressed = current_time;
        is_mounted = !is_mounted;
        printf("Estado do cartão SD: %s\n", is_mounted ? "Montado" : "Desmontado");
    } else if (gpio == BTN_B_PIN && current_time - last_time_btn_b_pressed > 290) {
        last_time_btn_b_pressed = current_time;
        if (!is_mounted) {
            printf("O cartão SD não está montado. Não é possível capturar dados.\n");
        } else {
            is_capture_mode = !is_capture_mode;
            printf("Modo de captura %s\n", is_capture_mode ? "ativado" : "desativado");
        }
    } else if (gpio == BTN_SW_PIN && current_time - last_time_btn_sw_pressed > 290) {
        reset_usb_boot(0, 0);
    }
}

void update_led_state() {
    if (!is_mounted) {
        set_led_yellow();
    } else if (is_mounted) {
        if (!is_capture_mode) {
            set_led_green();
        } else {
            set_led_red();
        }
    } else {
        turn_off_leds();
    }
}
