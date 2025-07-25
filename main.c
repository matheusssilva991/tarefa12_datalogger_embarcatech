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

#define DEBOUNCE_TIME_US 50000  // 50ms em microssegundos

void gpio_irq_callback(uint gpio, uint32_t events);
void update_led_state();
void update_display(ssd1306_t *ssd);

static char filename[20] = "data.txt";
volatile static int64_t last_time_btn_a_pressed = 0;
volatile static int64_t last_time_btn_b_pressed = 0;
volatile static int64_t last_time_btn_sw_pressed = 0;
volatile static bool is_mounted = false;
volatile static bool is_capture_mode = false;
volatile static int num_samples = 0;
volatile static bool current_mounted = false;
volatile static bool current_capture_mode = false;

// Evite redesenhar o display quando não há mudanças
static int last_num_samples = -1;
static bool last_is_mounted = false;
static bool last_is_capturing = false;

int main() {
    stdio_init_all();
    time_init();

    int16_t aceleracao[3], gyro[3], temp;
    ssd1306_t ssd;


    init_btns();
    init_btn(BTN_SW_PIN);
    init_leds();
    mpu6050_init();
    init_display(&ssd);

    gpio_set_irq_enabled_with_callback(
        BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_SW_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        // Verifica se houve mudanças no estado do cartão SD
        if (current_mounted != is_mounted) {
            current_mounted = is_mounted;
            if (is_mounted) {
                printf("Montando o cartão SD...\n");
                run_mount();
            } else {
                printf("Desmontando o cartão SD...\n");
                run_unmount();
            }
        }

        // Leitura dos dados do MPU6050
        mpu6050_read_raw(aceleracao, gyro, &temp);

        // Conversão do valor bruto para graus Celsius
        float temp_celsius = (temp / 340.0f) + 36.53f;

        // Captura e salva os dados no cartão SD
        if (is_capture_mode && is_mounted) {
            save_data(filename, aceleracao, gyro, temp_celsius);
            num_samples++;
        }

        // No loop principal, antes de update_display()
        bool display_needs_update = (last_num_samples != num_samples) ||
                                  (last_is_mounted != is_mounted) ||
                                  (last_is_capturing != is_capture_mode);

        if (display_needs_update) {
            update_display(&ssd);
            last_num_samples = num_samples;
            last_is_mounted = is_mounted;
            last_is_capturing = is_capture_mode;
        }

        // Verifica se o modo de captura foi ativado/desativado
        if (current_capture_mode != is_capture_mode) {
            current_capture_mode = is_capture_mode;
            if (!is_capture_mode) {
                draw_centered_text(&ssd, "Dados salvos.", 37);
                ssd1306_send_data(&ssd);
            }
        }

        update_led_state();
        sleep_ms(500);
    }
}

void gpio_irq_callback(uint gpio, uint32_t events)
{
    int64_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == BTN_A_PIN && current_time - last_time_btn_a_pressed > DEBOUNCE_TIME_US) {
        last_time_btn_a_pressed = current_time;
        is_mounted = !is_mounted;
        printf("Estado do cartão SD: %s\n", is_mounted ? "Montado" : "Desmontado");
    } else if (gpio == BTN_B_PIN && current_time - last_time_btn_b_pressed > DEBOUNCE_TIME_US) {
        last_time_btn_b_pressed = current_time;
        if (!is_mounted) {
            printf("O cartão SD não está montado. Não é possível capturar dados.\n");
        } else {
            is_capture_mode = !is_capture_mode;

            printf("Modo de captura %s\n", is_capture_mode ? "ativado" : "desativado");
        }
    } else if (gpio == BTN_SW_PIN && current_time - last_time_btn_sw_pressed > DEBOUNCE_TIME_US) {
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

void update_display(ssd1306_t *ssd) {
    bool color = true;
    char status_str[20];

    // Exibição no display
    ssd1306_fill(ssd, !color);
    ssd1306_rect(ssd, 3, 3, 122, 60, color, !color);
    ssd1306_line(ssd, 3, 15, 123, 15, color);  // Após título
    draw_centered_text(ssd, "DATALOGGER", 6);
    ssd1306_line(ssd, 3, 48, 123, 48, color);  // Antes do status

    if (is_mounted) {
        // Mostrar amostras tanto em modo de captura quanto quando finalizado
        if (is_capture_mode || num_samples > 0) {
            char sample_str[20];

            if (is_capture_mode) {
                snprintf(status_str, sizeof(status_str), "Capturando...");
                draw_centered_text(ssd, "Amostras:", 18);
            } else {
                snprintf(status_str, sizeof(status_str), "SD Montado");
                draw_centered_text(ssd, "Amos. salvas:", 18);
            }

            snprintf(sample_str, sizeof(sample_str), "%d", num_samples);
            draw_centered_text(ssd, sample_str, 28);
        } else {
            snprintf(status_str, sizeof(status_str), "SD Montado");
        }
    } else {
        snprintf(status_str, sizeof(status_str), "SD Desmontado");
    }

    draw_centered_text(ssd, status_str, 52);
    ssd1306_send_data(ssd);
}
