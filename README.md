# ttgoCotation.ino

Este projeto é um firmware para a placa **LilyGO TTGO** equipado com display OLED e conexão Wi-Fi. Ele exibe as cotações do **Dólar (USD), Bitcoin (BTC), Ethereum (ETH) e Euro (EUR)** em relação ao Real (BRL), obtendo os dados de uma API online. O projeto apresenta um layout otimizado para facilitar a visualização das informações e permite a navegação entre as diferentes telas através de botões físicos.

## 📌 **Principais Funcionalidades**

- **Conexão via Wi-Fi**: Obtém os valores atualizados de uma API online.
- **Exibição em Display OLED**: Mostra as cotações em telas individuais.
- **Atualização Automática**: A cada **10 minutos**, os valores são atualizados.
- **Troca de Telas**: O sistema troca automaticamente de tela a cada **1 minuto**.
- **Navegação Manual**: Permite avançar ou retroceder entre as telas usando botões físicos.
- **Histórico de Variação**: Armazena os últimos três valores de cada moeda e exibe uma seta para indicar aumento ou queda no valor.

## 🛠 **Componentes e Bibliotecas Utilizadas**

- **Placa:** LilyGO TTGO com ESP32
- **Display:** Tela OLED via **TFT_eSPI**
- **Conexão Wi-Fi:** Biblioteca **WiFi.h**
- **Requisições HTTP:** Biblioteca **HTTPClient.h**
- **Parsing de JSON:** Biblioteca **ArduinoJson.h**

## 🔧 **Configuração e Uso**

### 1️⃣ **Configurar Wi-Fi**
No código, altere as credenciais de Wi-Fi para sua rede:
```cpp
const char* ssid = "WIFI_NETWORK";
const char* password = "WIFI_PASSWORD";
```

### 2️⃣ **Carregar o Código**
Utilize a **Arduino IDE** ou **PlatformIO** para compilar e carregar o código na placa.

### 3️⃣ **Operação**
- Ao ligar, o dispositivo se conecta à rede Wi-Fi e obtém as cotações.
- A cada 10 minutos, os valores são atualizados automaticamente.
- As telas mudam automaticamente a cada 1 minuto.
- Use os botões físicos:
  - **Pino 25**: Voltar tela
  - **Pino 27**: Avançar tela

### 4️⃣ **Layout das Telas**
As cotações são exibidas em ordem pré-definida:
1. **Bitcoin**
2. **Ethereum**
3. **Dólar**
4. **Euro**

Cada tela mostra:
- Cotação atual (maior destaque)
- Cotação de 10 minutos atrás
- Cotação de 20 minutos atrás
- **Seta de tendência** 📈 ou 📉 (indicando alta ou queda)

## 🌐 **API Utilizada**
Os valores são obtidos da API **AwesomeAPI**:
```
https://economia.awesomeapi.com.br/json/last/USD-BRL,EUR-BRL,BTC-BRL,ETH-BRL
```

## 📌 **Observações**
- O código já implementa um sistema de debounce para evitar leituras falsas dos botões.
- A exibição do valor é formatada para incluir separação de milhares quando necessário.
- O display é inicializado e ajustado para rotação adequada.

## 📢 **Licença**
Este projeto é de código aberto. Sinta-se à vontade para modificar e melhorar conforme necessário.

---

🔧 **Desenvolvido para facilitar o monitoramento de cotações em tempo real!** 🚀

![ttgos](https://github.com/user-attachments/assets/856a4183-a918-4d2b-85d3-168ceb54f309)

