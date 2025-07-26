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
#include "lib/buzzer/buzzer.h"

#define DEBOUNCE_TIME_US 200000  // 200ms em microssegundos (corrigido)

void gpio_irq_callback(uint gpio, uint32_t events);
void update_led_state();
void update_display(ssd1306_t *ssd);
void beep_start_capture();
void beep_stop_capture();

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

volatile static bool showing_temp_message = false;
volatile static int64_t message_start_time = 0;
#define MESSAGE_DURATION_US 2000000  // 2 segundos em microssegundos
volatile static int message_state = 0;  // 0: normal, 1: "Dados salvos", 2: "X amostras salvas"
volatile static bool should_beep_start = false;
volatile static bool should_beep_stop = false;

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
    init_buzzer(BUZZER_A_PIN, 4.0f);  // Inicializa o buzzer com divisor de clock de 4.0
    init_buzzer(BUZZER_B_PIN, 4.0f);  // Inicializa o buzzer com divisor de clock de 4.0

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

        // Verifica se a mensagem de temperatura deve ser exibida
        if (showing_temp_message) {
            int64_t current_time = to_us_since_boot(get_absolute_time());
            if (current_time - message_start_time > MESSAGE_DURATION_US) {
                if (message_state == 1) {
                    // Muda para a segunda mensagem
                    message_state = 2;
                    message_start_time = current_time;
                } else {
                    // Finaliza a sequência de mensagens
                    showing_temp_message = false;
                    message_state = 0;
                }
                // Force a atualização do display
                last_num_samples = -1;  // Isso forçará update_display na próxima iteração
            }
        }

        // Verifica se deve tocar o som de início de captura
        if (should_beep_start) {
            should_beep_start = false;
            beep_start_capture();
        }

        // Verifica se deve tocar o som de fim da captura
        if (should_beep_stop) {
            should_beep_stop = false;
            beep_stop_capture();
        }

        update_led_state();
        sleep_ms(500);
    }
}

// Callback para interrupções dos botões
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

            // Adicione estas linhas para mostrar a mensagem no display
            message_state = 3;  // Novo estado para mensagem de erro
            showing_temp_message = true;
            message_start_time = to_us_since_boot(get_absolute_time());

            // Force uma atualização imediata do display
            last_num_samples = -1;  // Isso forçará update_display na próxima iteração

            // Som de erro
            should_beep_stop = true;
        } else {
            // Salva o estado anterior
            bool was_capturing = is_capture_mode;

            // Muda o estado
            is_capture_mode = !is_capture_mode;

            // Se estiver iniciando captura
            if (is_capture_mode) {
                num_samples = 0;
                printf("Modo de captura ativado\n");
                should_beep_start = true;  // Define flag para tocar som
            }
            // Se estiver finalizando captura e tiver amostras
            else if (was_capturing && num_samples > 0) {
                printf("Modo de captura desativado. %d amostras salvas.\n", num_samples);

                // Inicia sequência de mensagens
                message_state = 1;  // Primeira mensagem: "Dados salvos"
                showing_temp_message = true;
                message_start_time = to_us_since_boot(get_absolute_time());

                should_beep_stop = true;  // Define flag para tocar som
            }
        }
    } else if (gpio == BTN_SW_PIN && current_time - last_time_btn_sw_pressed > DEBOUNCE_TIME_US) {
        reset_usb_boot(0, 0);
    }
}

// Atualiza o estado dos LEDs com base no estado atual
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

// Atualiza o display com o estado atual
void update_display(ssd1306_t *ssd) {
    bool color = true;
    char status_str[20];
    char message[30];

    // Exibição no display
    ssd1306_fill(ssd, !color);
    ssd1306_rect(ssd, 3, 3, 122, 60, color, !color);
    ssd1306_line(ssd, 3, 15, 123, 15, color);  // Após título
    draw_centered_text(ssd, "DATALOGGER", 6);
    ssd1306_line(ssd, 3, 48, 123, 48, color);  // Antes do status

    // Verifica se deve mostrar mensagem temporária
    if (showing_temp_message) {
        if (message_state == 1) {
            // Primeira mensagem
            draw_centered_text(ssd, "Dados salvos", 30);
        } else if (message_state == 2) {
            // Segunda mensagem
            snprintf(message, sizeof(message), "%d amos.", num_samples);
            draw_centered_text(ssd, message, 22);
            draw_centered_text(ssd, "salvas", 32);
        } else if (message_state == 3) {
            // Mensagem de erro - SD não montado
            draw_centered_text(ssd, "ERRO!", 20);
            draw_centered_text(ssd, "SD nao montado.", 30);
        }

        // Status continua sendo exibido na parte inferior
        if (is_mounted) {
            snprintf(status_str, sizeof(status_str), "SD Montado");
        } else {
            snprintf(status_str, sizeof(status_str), "SD Desmontado");
        }
    }
    // Exibição normal quando não há mensagem temporária
    else {
        if (is_mounted) {
            if (is_capture_mode) {
                // No modo de captura, mostra contagem de amostras
                char sample_str[20];
                snprintf(status_str, sizeof(status_str), "Capturando...");
                draw_centered_text(ssd, "Amostras:", 20);
                snprintf(sample_str, sizeof(sample_str), "%d", num_samples);
                draw_centered_text(ssd, sample_str, 30);
            } else {
                // Quando não está capturando, só mostra "Aguardando..."
                snprintf(status_str, sizeof(status_str), "SD Montado");
                draw_centered_text(ssd, "Aguardando...", 30);
                // NÃO mostrar número de amostras aqui
            }
        } else {
            snprintf(status_str, sizeof(status_str), "SD Desmontado");
        }
    }

    draw_centered_text(ssd, status_str, 52);
    ssd1306_send_data(ssd);
}

// Funções para controle do buzzer
void beep_start_capture() {
    play_tone(BUZZER_A_PIN, 450);  // 450 Hz
    sleep_ms(100);  // Som curto de 100ms
    stop_tone(BUZZER_A_PIN);  // Para o som
}

// Função para tocar som de fim de captura
void beep_stop_capture() {
    play_tone(BUZZER_B_PIN, 900);  // 900 Hz
    sleep_ms(100);
    stop_tone(BUZZER_B_PIN);  // Para o som
    sleep_ms(50);  // Pequena pausa entre os beeps
    play_tone(BUZZER_B_PIN, 900);  // Reproduz novamente
    sleep_ms(100);
    stop_tone(BUZZER_B_PIN);  // Para o som
}
