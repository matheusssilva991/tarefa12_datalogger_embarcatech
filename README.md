# 📊 InertiaSense Datalogger

Um sistema avançado de captura de dados inerciais desenvolvido para **Raspberry Pi Pico** com interface OLED e armazenamento em cartão SD.

## 📋 Descrição do Projeto

Este projeto implementa um datalogger embarcado utilizando **Raspberry Pi Pico** que coleta dados de aceleração e giroscópio do sensor MPU6050, armazena-os em formato CSV no cartão microSD e fornece feedback visual através de display OLED e LEDs indicadores. O sistema oferece uma interface intuitiva controlada por botões, com feedback sonoro para melhor experiência do usuário.

## ⚡ Funcionalidades

### 📊 **Captura de Dados**
- **Aceleração** nos três eixos (X, Y, Z)
- **Giroscópio** nos três eixos (X, Y, Z)
- **Temperatura** do sensor integrado
- **Timestamp** para cada amostra coletada

### 🖥️ **Interface Visual**
- Display OLED com status do sistema
- Contagem em tempo real de amostras
- Mensagens de estado e feedback
- Indicadores LED para estados diferentes

### 🔊 **Feedback Sonoro**
- Som de início de captura
- Som de finalização de captura
- Alertas para operações inválidas

### 💾 **Armazenamento e Leitura**
- Formato CSV estruturado para fácil análise
- Cabeçalho de dados claro e organizado
- Leitura de arquivos salvos
- Listagem de dados no terminal para cópia

## 🛠️ Hardware Utilizado

- **Microcontrolador**: Raspberry Pi Pico
- **Sensores**:
  - MPU6050 (acelerômetro e giroscópio de 3 eixos com sensor de temperatura)
- **Armazenamento**:
  - Módulo de cartão microSD
- **Interface de Usuário**:
  - Display OLED SSD1306
  - LEDs indicadores (vermelho, verde, amarelo e azul)
  - Buzzers para feedback sonoro
  - 3 Botões de controle (Botão A, Botão B e joystick)

## 🚀 Como Rodar

### **Pré-requisitos**
- Raspberry Pi Pico
- SDK C/C++ do Raspberry Pi Pico instalado
- CMake e ferramentas de build
- VSCode com extensão PlatformIO (recomendado)

### **1. Clone o Repositório**
```bash
git clone https://github.com/matheusssilva991/tarefa12_datalogger_embarcatech.git
cd tarefa12_datalogger_embarcatech
```

### **2. Configuração do Hardware**
```
Pico  →  MPU6050
VCC     →  3.3V
GND     →  GND
SDA     →  GP4 (I2C SDA)
SCL     →  GP5 (I2C SCL)

Pico  →  Cartão microSD
VCC     →  3.3V
GND     →  GND
MOSI   →  GP15
MISO   →  GP14
SCK    →  GP13
CS     →  GP12
```

### **3. Compilação e Upload**
```bash
mkdir build
cd build
cmake ..
make
# Conecte o Pico em modo BOOTSEL
cp main.uf2 /media/RPI-RP2/
```

### **4. Acesso à Interface**
1. Abra o monitor serial para ver o status
2. Acesse o terminal para visualizar dados
3. Interaja com o sistema através dos botões


## 🎥 Vídeo de Demonstração

[![Demonstração do DataLogger EmbarcaTech]()]()

*Clique na imagem acima para assistir ao vídeo de demonstração completo*

### **O que você verá no vídeo:**
- ✅ Inicialização do sistema
- ✅ Conexão com o sensor MPU6050
- ✅ Interface OLED exibindo status
- ✅ Captura de dados em tempo real
- ✅ Armazenamento em cartão microSD
- ✅ Leitura e listagem de dados salvos
- ✅ Feedback sonoro para ações

## 🏗️ Arquitetura do Sistema

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Sensores      │    │   Raspberry      │    │   Interface     │
│                 │◄──►│   Pico           │◄──►│   OLED/SD       │
│ • MPU6050       │    │                  │    │ • Display OLED  │
│                 │    │ • Leitura I2C    │    │ • Cartão microSD│
│                 │    │ • Armazenamento  │    │ • LEDs          │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 📁 Estrutura do Projeto

```
tarefa12_datalogger_embarcatech/
│
├── 📁 lib/                      # Bibliotecas utilizadas no projeto
│   ├── button/                  # Controle dos botões
│   ├── buzzer/                  # Controle dos buzzers
│   ├── led/                     # Controle dos LEDs
│   ├── mpu6050/                 # Driver do sensor MPU6050
│   ├── sd_card/                 # Interface com cartão SD
│   └── ssd1306/                 # Driver do display OLED
│
├── main.c                       # Código principal do projeto
├── CMakeLists.txt               # Configuração do CMake
└── README.md                    # Documentação do projeto
```

## 🏆 Desenvolvedores

<table>
  <tr>
    <td align="center">
      <a href="https://github.com/matheusssilva991">
        <img src="https://avatars.githubusercontent.com/matheusssilva991" width="100px;" alt="Foto do Matheus"/><br>
        <sub>
          <b>Matheus Santos Silva</b>
        </sub>
      </a><br>
      <span title="Código">💻</span>
      <span title="Hardware">🔧</span>
      <span title="Interface">🎨</span>
    </td>
  </tr>
</table>
