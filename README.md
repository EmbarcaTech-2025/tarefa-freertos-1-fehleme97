# 🔔 PROJETO: CONTROLE DE TAREFAS COM SIRENE E LED ATRAVÉS DO FreeRTOS - BITDOGLAB (PICO W + FreeRTOS)

**Autor:**  
- FELIPE LEME CORREA DA SILVA

**Instituição:**  
- EmbarcaTech - HBr - Campinas

---

# 📦 SOBRE O PROJETO

Este projeto demonstra o uso do **FreeRTOS** com o **BitDogLab (Raspberry Pi Pico W)** para executar múltiplas tarefas simultaneamente, utilizando **suspensão e retomada de tarefas** por meio de botões físicos.

O sistema é composto por duas tarefas principais:

- Uma tarefa que reproduz uma **sirene progressiva** utilizando o PWM.
- Uma tarefa que alterna as cores do **LED RGB**.
- Ambas podem ser **suspensas ou retomadas** por botões físicos distintos.

---

# 🗂️ ESTRUTURA DO PROJETO

```
/projeto_sirene_led
│── include/            # Arquivos auxiliares
│── tarefa-freertos-1-fehleme97.c        # Código principal com FreeRTOS
│── CMakeLists.txt      # Script de build do projeto
│── README.md           # Este documento
│── LICENSE.txt         # Licença do projeto
```

# 🛠️ FUNCIONALIDADES IMPLEMENTADAS

- ✅ Execução simultânea de tarefas com FreeRTOS  
- ✅ Reproduz som de sirene progressiva no buzzer  
- ✅ Alternância de cores no LED RGB (vermelho, verde, azul)  
- ✅ Controle de execução por botões físicos:  
  - Botão A (GPIO 5): Liga/desliga o LED RGB  
  - Botão B (GPIO 6): Liga/desliga o buzzer (sirene)  
- ✅ Debounce de botão implementado  
- ✅ Suspensão de tarefa com `vTaskSuspend()` e retomada com `vTaskResume()`  
- ✅ Comentários explicativos no código para aprendizado    

# 🌐 CONFIGURAÇÕES DE HARDWARE

| Componente  | GPIO          | Função              |
|-------------|---------------|---------------------|
| Buzzer      | GPIO 21       | Saída PWM da sirene |
| Botão A     | GPIO 5        | Suspender LED       |
| Botão B     | GPIO 6        | Suspender Buzzer    |
| LED RGB     | GPIO 11,12,13 | Verde, Azul, Vermelho |

# 🔄 FLUXO DE FUNCIONAMENTO

1. Ao ligar o sistema, LED RGB e sirene já estão **ativos por padrão**.
2. Pressione o **botão A** (GPIO 5) para alternar a execução do LED RGB.
3. Pressione o **botão B** (GPIO 6) para alternar a execução da sirene.
4. O sistema continua operando em tempo real com **gerenciamento multitarefa via FreeRTOS**.

# 🖥️ Execução e Gravação no Pico

1. Compilar com:

```
mkdir build
cd build
cmake ..
make
``` 

2. Copiar o ".uf2" gerado para o Raspberry Pi Pico W.

# ⚙️ TECNOLOGIAS UTILIZADAS

- C com Pico SDK
- FreeRTOS (multitarefa cooperativa)
- PWM da RP2040
- GPIO para controle digital
- BitDogLab (Raspberry Pi Pico W com periféricos integrados)

# 📜 Licença

- GNU GENERAL PUBLIC LICENSE
