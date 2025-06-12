#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <string.h>

// Definições dos pinos utilizados
const uint BUZZER_GPIO = 21;
const uint BOTAO_A_GPIO = 5;
const uint BOTAO_B_GPIO = 6;
const uint I2C_SDA = 14;              // Pino SDA do OLED
const uint I2C_SCL = 15;              // Pino SCL do OLED

// Parâmetros do sistema
#define SCREEN_WIDTH 128              // Largura da tela OLED
#define SCREEN_HEIGHT 64              // Altura da tela OLED

// Instâncias do display
ssd1306_t oled;
uint8_t oled_frame[1024];

// Tornar globais:
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1
};


// Handles das tarefas para controle posterior (suspensão/retomada)
TaskHandle_t handle_led = NULL;
TaskHandle_t handle_buzzer = NULL;

// Configura o PWM no pino do buzzer
void configurar_pwm_buzzer() {

    gpio_set_function(BUZZER_GPIO, GPIO_FUNC_PWM); // Define pino como saída PWM
    uint slice = pwm_gpio_to_slice_num(BUZZER_GPIO); // Obtém o "slice" do PWM associado ao pino

    pwm_set_clkdiv(slice, 4.0f); // Divide o clock do sistema (125 MHz / 4 = 31.25 MHz)
    pwm_set_enabled(slice, true); // Habilita o PWM no slice selecionado

}

// Define a frequência e o duty cycle do buzzer
void buzzer_set_freq(uint freq, float duty_percent) {

    uint slice = pwm_gpio_to_slice_num(BUZZER_GPIO);
    uint32_t clk = 125000000 / 4; // Clock já dividido
    uint32_t wrap = clk / freq;  // Calcula valor do "topo" do contador para a frequência desejada

    pwm_set_wrap(slice, wrap); // Define valor de topo
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_GPIO), wrap * duty_percent); // Define nível (duty cycle)

}

// Função utilitária para detectar botão pressionado com debounce
bool botao_pressionado(uint gpio) {

    if (gpio_get(gpio) == 0) { // Pressionado (nível baixo)
        sleep_ms(50); // Espera um pouco para eliminar ruídos (debounce)
        if (gpio_get(gpio) == 0) {
            while (gpio_get(gpio) == 0); // Aguarda o botão ser solto
            return true;
        }
    }
    return false;

}

// Tarefa responsável por piscar o LED RGB de forma sequencial
void led_task(void *p) {
    
    const uint LED_R_PIN = 13;
    const uint LED_G_PIN = 11;
    const uint LED_B_PIN = 12;

    // Inicializa os pinos como saída
    gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN); gpio_set_dir(LED_B_PIN, GPIO_OUT);

    // Loop infinito alternando os LEDs com delay
    while (true) {

        gpio_put(LED_R_PIN, 1); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_put(LED_R_PIN, 0); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_put(LED_G_PIN, 1); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_put(LED_G_PIN, 0); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_put(LED_B_PIN, 1); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_put(LED_B_PIN, 0); vTaskDelay(pdMS_TO_TICKS(300));

    }
}

// Tarefa responsável por gerar o som da sirene no buzzer
void task_buzzer(void *p) {

    configurar_pwm_buzzer(); // Inicializa o PWM no pino do buzzer

    while (true) {
        
        // Frequência crescente (sirene sobe)
        for (int f = 400; f <= 1000; f += 10) {
            buzzer_set_freq(f, 0.5f); // 50% duty cycle
            vTaskDelay(pdMS_TO_TICKS(10)); // Pequeno delay para suavizar a variação
        }

        // Frequência decrescente (sirene desce)
        for (int f = 1000; f >= 400; f -= 10) {
            buzzer_set_freq(f, 0.5f);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Pausa entre os ciclos de sirene
    }
}

// Tarefa responsável por monitorar os botões e suspender/retomar as tarefas
void monitor_button_task() {

    // Configura os pinos dos botões como entrada com pull-up interno
    gpio_init(BOTAO_A_GPIO); gpio_set_dir(BOTAO_A_GPIO, GPIO_IN); gpio_pull_up(BOTAO_A_GPIO);
    gpio_init(BOTAO_B_GPIO); gpio_set_dir(BOTAO_B_GPIO, GPIO_IN); gpio_pull_up(BOTAO_B_GPIO);

    static bool led_suspenso = false;
    static bool buzzer_suspenso = false;

    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    calculate_render_area_buffer_length(&frame_area);


    while (true) {

        // Se o botão A for pressionado, alterna o estado da tarefa do LED
        if (botao_pressionado(BOTAO_A_GPIO)) {
            if (!led_suspenso) {
                vTaskSuspend(handle_led); // Suspende a tarefa do LED
                led_suspenso = true;

                // Limpa tela e escreve
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 32, "  LED SUSPENSO");
                render_on_display(ssd, &frame_area);
            } else {
                vTaskResume(handle_led); // Retoma a tarefa do LED
                led_suspenso = false;
                // Limpa tela e escreve
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 32, "  LED RETOMADO");
                render_on_display(ssd, &frame_area);
            }
        }

        // Se o botão B for pressionado, alterna o estado da tarefa do buzzer
        if (botao_pressionado(BOTAO_B_GPIO)) {
            if (!buzzer_suspenso) {
                vTaskSuspend(handle_buzzer); // Suspende a tarefa do buzzer
                buzzer_suspenso = true;

                // Limpa tela e escreve
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 32, " BUZZER SUSPENSO");
                render_on_display(ssd, &frame_area);
            } else {
                vTaskResume(handle_buzzer); // Retoma a tarefa do buzzer
                buzzer_suspenso = false;
                // Limpa tela e escreve
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 32, " BUZZER RETOMADO");
                render_on_display(ssd, &frame_area);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para diminuir a verificação do estado do botão
    }
}


// Função principal
int main() {
    stdio_init_all(); 

    // Inicialização do display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    calculate_render_area_buffer_length(&frame_area);

    // Mensagem inicial no display
    ssd1306_draw_string(ssd, 0, 16, "    SISTEMA  ");
    ssd1306_draw_string(ssd, 0, 32, "   INICIANDO.. ");
    render_on_display(ssd, &frame_area);
    sleep_ms(3000);

    // Criação das tarefas e armazenamento dos seus handles
    xTaskCreate(led_task, "LED_Task", 512, NULL, 2, &handle_led);
    xTaskCreate(task_buzzer, "Buzzer_Task", 512, NULL, 1, &handle_buzzer);
    xTaskCreate(monitor_button_task, "Botao_Task", 1024, NULL, 3, NULL);


    // Inicia o agendador do FreeRTOS
    vTaskStartScheduler();

    while(1){};
}
