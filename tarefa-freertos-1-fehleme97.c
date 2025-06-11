#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

// Definições dos pinos utilizados
const uint BUZZER_GPIO = 21;
const uint BOTAO_A_GPIO = 5;
const uint BOTAO_B_GPIO = 6;

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

    while (true) {

        // Se o botão A for pressionado, alterna o estado da tarefa do LED
        if (botao_pressionado(BOTAO_A_GPIO)) {
            if (!led_suspenso) {
                vTaskSuspend(handle_led); // Suspende a tarefa do LED
                led_suspenso = true;
            } else {
                vTaskResume(handle_led); // Retoma a tarefa do LED
                led_suspenso = false;
            }
        }

        // Se o botão B for pressionado, alterna o estado da tarefa do buzzer
        if (botao_pressionado(BOTAO_B_GPIO)) {
            if (!buzzer_suspenso) {
                vTaskSuspend(handle_buzzer); // Suspende a tarefa do buzzer
                buzzer_suspenso = true;
            } else {
                vTaskResume(handle_buzzer); // Retoma a tarefa do buzzer
                buzzer_suspenso = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para diminuir a verificação do estado do botão
    }
}

// Função principal
int main() {
    stdio_init_all(); 

    // Criação das tarefas e armazenamento dos seus handles
    xTaskCreate(led_task, "LED_Task", 256, NULL, 2, &handle_led);
    xTaskCreate(task_buzzer, "Buzzer_Task", 256, NULL, 1, &handle_buzzer);
    xTaskCreate(monitor_button_task, "Botao_Task", 256, NULL, 3, NULL);

    // Inicia o agendador do FreeRTOS
    vTaskStartScheduler();

    while(1){};
}
