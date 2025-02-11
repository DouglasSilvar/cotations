#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// ========================
// Configurações do Display
// ========================
TFT_eSPI tft = TFT_eSPI();   // Objeto do display
#define DISPLAY_ROTATION 1   // Rotaciona 90° no sentido horário

// ========================
// Configurações do WiFi
// ========================
const char* ssid = "WIFI_NETWORK";
const char* password = "WIFI_PASSWORD";

// ========================
// Configurações dos Botões
// ========================
const int buttonBackPin = 25;    // Botão para voltar
const int buttonManualPin = 26;  // Botão para atualizar manualmente
const int buttonForwardPin = 27; // Botão para avançar
bool lastButtonBackState    = HIGH;
bool lastButtonForwardState = HIGH;
bool lastButtonManualState = HIGH;

// ========================
// Intervalos (em milissegundos)
// ========================
const unsigned long updateInterval   = 600000; // 10 minutos para atualizar a API
const unsigned long autoCycleInterval = 60000;  // 1 minuto para auto-ciclo das telas
unsigned long lastUpdateTime    = 0;
unsigned long lastAutoCycleTime = 0;

// ========================
// Variável para controlar a tela atual
// ========================
int currentScreen = 0;  // Variável global para a tela atual (0 a 3)

// ========================
// Estrutura para armazenar os dados de cada moeda
// ========================
struct Currency {
  const char* name;
  float values[3];  // values[0]: 20 min atrás, values[1]: 10 min atrás, values[2]: atual
};

// Ordem na API (ao atualizar):  
//   índice 0 → DOLAR  
//   índice 1 → BITCOIN  
//   índice 2 → ETHEREUM  
//   índice 3 → EURO  
// Mas a exibição (telas) será nesta ordem:
//   Tela 1: BITCOIN  
//   Tela 2: ETHEREUM  
//   Tela 3: DOLAR  
//   Tela 4: EURO
Currency currencies[4] = {
  { "Dolar",    { -1, -1, -1 } },
  { "BitCoin",  { -1, -1, -1 } },
  { "Ethereum", { -1, -1, -1 } },
  { "Euro",     { -1, -1, -1 } }
};
// Mapeamento das telas para os índices do array currencies:
int screenMapping[4] = { 1, 2, 0, 3 };


// ====================================================
// Função auxiliar para formatar o valor com separador
// Insere um espaço antes dos últimos 3 dígitos, se necessário.
// Se useSeparator for false, retorna o valor sem formatação extra.
void formatValue(float val, bool useSeparator, char* outStr, int outSize) {
  char temp[20];
  sprintf(temp, "%.2f", val); // converte para string com 2 casas decimais
  if (!useSeparator) {
    strncpy(outStr, temp, outSize);
    outStr[outSize - 1] = '\0';
    return;
  }
  // Procura o ponto decimal
  int dotPos = -1;
  for (int i = 0; temp[i] != '\0'; i++) {
    if (temp[i] == '.') { 
      dotPos = i; 
      break; 
    }
  }
  if (dotPos < 0) { // se não encontrou ponto, copia direto
    strncpy(outStr, temp, outSize);
    outStr[outSize - 1] = '\0';
    return;
  }
  int intLen = dotPos;  // tamanho da parte inteira
  // Se a parte inteira tiver mais de 3 dígitos, insere um espaço antes dos últimos 3
  if (intLen > 3) {
    int prefixLen = intLen - 3;
    int pos = 0;
    // Copia o prefixo
    for (int i = 0; i < prefixLen && pos < outSize - 1; i++) {
      outStr[pos++] = temp[i];
    }
    // Insere o espaço
    if (pos < outSize - 1) {
      outStr[pos++] = ' ';
    }
    // Copia os últimos 3 dígitos da parte inteira
    for (int i = prefixLen; i < intLen && pos < outSize - 1; i++) {
      outStr[pos++] = temp[i];
    }
    // Copia a parte decimal (ponto e decimais)
    for (int i = dotPos; temp[i] != '\0' && pos < outSize - 1; i++) {
      outStr[pos++] = temp[i];
    }
    outStr[pos] = '\0';
  } else {
    strncpy(outStr, temp, outSize);
    outStr[outSize - 1] = '\0';
  }
}


// ========================
// Função: Atualiza os valores via API
// ========================
void updateCurrenciesFromAPI() {
  HTTPClient http;
  // Endpoint que retorna os valores de USD, EUR, BTC e ETH em relação ao BRL
  http.begin("https://economia.awesomeapi.com.br/json/last/USD-BRL,EUR-BRL,BTC-BRL,ETH-BRL");
  int httpCode = http.GET();
  
  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Extrai os valores (a propriedade "bid" vem como string e é convertida para float)
      float usd = doc["USDBRL"]["bid"].as<float>();
      float eur = doc["EURBRL"]["bid"].as<float>();
      float btc = doc["BTCBRL"]["bid"].as<float>();
      float eth = doc["ETHBRL"]["bid"].as<float>();
      
      // Para cada moeda, se ainda não houver histórico (values[1] < 0),
      // inicializa todas as posições com o valor atual;
      // caso contrário, desloca os históricos e insere o novo valor em [2].
      // DOLAR (índice 0)
      if (currencies[0].values[1] < 0) {
        currencies[0].values[0] = usd;
        currencies[0].values[1] = usd;
        currencies[0].values[2] = usd;
      } else {
        currencies[0].values[0] = currencies[0].values[1];
        currencies[0].values[1] = currencies[0].values[2];
        currencies[0].values[2] = usd;
      }
      
      // BITCOIN (índice 1)
      if (currencies[1].values[1] < 0) {
        currencies[1].values[0] = btc;
        currencies[1].values[1] = btc;
        currencies[1].values[2] = btc;
      } else {
        currencies[1].values[0] = currencies[1].values[1];
        currencies[1].values[1] = currencies[1].values[2];
        currencies[1].values[2] = btc;
      }
      
      // ETHEREUM (índice 2)
      if (currencies[2].values[1] < 0) {
        currencies[2].values[0] = eth;
        currencies[2].values[1] = eth;
        currencies[2].values[2] = eth;
      } else {
        currencies[2].values[0] = currencies[2].values[1];
        currencies[2].values[1] = currencies[2].values[2];
        currencies[2].values[2] = eth;
      }
      
      // EURO (índice 3)
      if (currencies[3].values[1] < 0) {
        currencies[3].values[0] = eur;
        currencies[3].values[1] = eur;
        currencies[3].values[2] = eur;
      } else {
        currencies[3].values[0] = currencies[3].values[1];
        currencies[3].values[1] = currencies[3].values[2];
        currencies[3].values[2] = eur;
      }
      
      Serial.println("Dados atualizados da API.");
    }
    else {
      Serial.print("Erro ao desserializar JSON: ");
      Serial.println(error.c_str());
    }
  }
  else {
    Serial.print("Erro na requisição, HTTP Code: ");
    Serial.println(httpCode);
  }
  http.end();
}

// ========================
// Função: Desenha a tela para a moeda (layout conforme solicitado)
// ========================
void drawScreen(int screenIndex) {
  tft.fillScreen(TFT_BLACK);
  
  // Obtemos o índice da moeda a partir do mapeamento
  int currencyIndex = screenMapping[screenIndex];
  Currency cur = currencies[currencyIndex];
  
  // Para a rotação 90°, considere resolução efetiva 240 x 135.
  // Layout:
  //   Cabeçalho (nome da moeda) – size 3
  //   Linha 1: cotação de 20 min atrás – size 2
  //   Linha 2: cotação de 10 min atrás – size 2 + seta (comparando com o valor atual)
  //   Linha 3: cotação atual – size 3
  int x = 10;
  int yHeader = 5;
  int yLine1  = 35;
  int yLine2  = 60;
  int yLine3  = 90;
  
  char buffer[40];
  char formatted[30];
  // Para Bitcoin e Ethereum, usamos separador de milhares
  bool useSeparator = (currencyIndex == 1 || currencyIndex == 2);
  
  // Cabeçalho: Nome da moeda (size 3, cor laranja)
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString(cur.name, x, yHeader);
  
  // Definindo os prefixos:
  // Para as linhas 1 e 2: para todas as moedas, usa "R$: "
  // Para a linha 3: se for Bitcoin, usa "R$" (sem espaço); caso contrário, "R$: "
  const char* prefixLine12 = "R$: ";
  const char* prefixLine3  = (currencyIndex == 1) ? "R$" : "R$: ";
  
  // Linha 1: Cotação de 20 min atrás (size 2, cor azul)
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.setTextSize(2);
  formatValue(cur.values[0], useSeparator, formatted, sizeof(formatted));
  sprintf(buffer, "%s%s", prefixLine12, formatted);
  tft.drawString(buffer, x, yLine1);
  
  // Linha 2: Cotação de 10 min atrás (size 2, cor roxo)
  tft.setTextColor(TFT_PURPLE, TFT_BLACK);
  tft.setTextSize(2);
  formatValue(cur.values[1], useSeparator, formatted, sizeof(formatted));
  sprintf(buffer, "%s%s", prefixLine12, formatted);
  tft.drawString(buffer, x, yLine2);
  
  // Desenha a seta de tendência após a linha 2.
  // A seta é desenhada com 5 pixels a mais à direita do texto.
  bool drawArrow = false;
  bool arrowUp = false;
  if (cur.values[2] > cur.values[1]) {
    drawArrow = true;
    arrowUp = true;
  }
  else if (cur.values[2] < cur.values[1]) {
    drawArrow = true;
    arrowUp = false;
  }
  int textWidth = tft.textWidth(buffer);
  int arrowX = x + textWidth + 7; // deslocamento de 7 pixels (5 pixels adicionais)
  int arrowY = yLine2 + 5;         // ajuste vertical para centralizar com o texto
  if (drawArrow) {
    if (arrowUp) {
      // Seta para cima: cor verde
      tft.fillTriangle(arrowX, arrowY, arrowX-5, arrowY+10, arrowX+5, arrowY+10, TFT_GREEN);
    }
    else {
      // Seta para baixo: cor vermelha
      tft.fillTriangle(arrowX, arrowY+10, arrowX-5, arrowY, arrowX+5, arrowY, TFT_RED);
    }
  }
  
  // Linha 3: Cotação atual (size 3, cor branca)
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  formatValue(cur.values[2], useSeparator, formatted, sizeof(formatted));
  sprintf(buffer, "%s%s", prefixLine3, formatted);
  tft.drawString(buffer, x, yLine3);
}


// ========================
// Setup
// ========================
void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");
  
  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Inicializa o display e aplica a rotação
  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(TFT_BLACK);
  
  // Ativa o backlight (verifique seu User_Setup.h ou force um pino)
  #ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  #else
    const int BL_PIN = 4;
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, HIGH);
  #endif
  
  // Configura os botões com resistor pull-up interno
  pinMode(buttonBackPin, INPUT_PULLUP);
  pinMode(buttonForwardPin, INPUT_PULLUP);
  pinMode(buttonManualPin, INPUT_PULLUP);

  
  // Primeira atualização da API
  updateCurrenciesFromAPI();
  lastUpdateTime    = millis();
  lastAutoCycleTime = millis();
  
  // Exibe a tela inicial (tela 0 corresponde a BITCOIN conforme mapeamento)
  drawScreen(currentScreen);
}

// ========================
// Loop
// ========================
void loop() {
  unsigned long currentMillis = millis();
  
  // Atualiza os valores via API a cada 10 minutos
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    updateCurrenciesFromAPI();
    drawScreen(currentScreen);
  }
  
  // Auto-ciclo das telas a cada 1 minuto
  if (currentMillis - lastAutoCycleTime >= autoCycleInterval) {
    lastAutoCycleTime = currentMillis;
    currentScreen = (currentScreen + 1) % 4;
    drawScreen(currentScreen);
  }
  
  // Navegação manual via botões
  bool currentBackState    = digitalRead(buttonBackPin);
  bool currentForwardState = digitalRead(buttonForwardPin);
  bool currentManualState = digitalRead(buttonManualPin);
    if (lastButtonManualState == HIGH && currentManualState == LOW) {
      // Chama a atualização manualmente
      updateCurrenciesFromAPI();
      // Atualiza o timer para evitar que o update automático ocorra logo em seguida
      lastUpdateTime = millis();
      // Atualiza a tela para refletir os novos valores
      drawScreen(currentScreen);
      delay(200); // debounce
    }
lastButtonManualState = currentManualState;
  
  if (lastButtonBackState == HIGH && currentBackState == LOW) {
    currentScreen = (currentScreen - 1 + 4) % 4;
    drawScreen(currentScreen);
    delay(200); // debounce
  }
  lastButtonBackState = currentBackState;
  
  if (lastButtonForwardState == HIGH && currentForwardState == LOW) {
    currentScreen = (currentScreen + 1) % 4;
    drawScreen(currentScreen);
    delay(200); // debounce
  }
  lastButtonForwardState = currentForwardState;
  
  delay(50);
}

