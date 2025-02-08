#include <Wire.h>               // Biblioteca para comunicação I2C
#include <Adafruit_PN532.h>     // Biblioteca para o módulo PN532
#include <WiFi.h>               // Biblioteca para conexão Wi-Fi
#include <HTTPClient.h>
#include <UrlEncode.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define LED1RFID 27
#define LEDVERDE 13

#define BUZZER 18
#define PIR 25
#define DHT 4

#define LDR1 32
#define LDR2 35
#define LDR3 34

const int RESOLUTION = 12;

// Inicializa o objeto para o módulo NFC PN532 usando I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
// Credenciais Wi-Fi
const char* ssid = "seu wifi";
const char* password = "Ufrr@2024Cit";

String phoneNumber = "+seu numero";
String apiKey = "sua api";

bool flagAlteracao = 0;

int leituraLDR1 = 0;
int leituraLDR2 = 0;
int leituraLDR3 = 0;

bool salaBloqueada = true;

String authorizedTags[] = { "0x04 0x7A 0x6C 0x1B", "0x93 0x82 0xa7 0xd" };

void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi conectado.");
}

void sendMessage(String phoneNumber, String apiKey, String message) {
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&text=" + urlEncode(message) + "&apikey=" + apiKey;
  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.println("Mensagem enviada com sucesso");
  } else {
    Serial.println("Erro no envio da mensagem");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

bool isAuthorized(String uid) {
  for (String tag : authorizedTags) {
    if (tag == uid) {
      return true;
    }
  }
  return false;
}

// Função para verificar a tag RFID detectada
void verificarRFID() {
  uint8_t success = nfc.inListPassiveTarget();

  if (success > 0) {
    uint8_t uid[7];
    uint8_t uidLength;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      String uidString = "";
      for (uint8_t i = 0; i < uidLength; i++) {
        uidString += "0x";
        uidString += String(uid[i], HEX);
        if (i < uidLength - 1) uidString += " ";
      }

      // Processa a tag detectada
      Serial.print("UID da tag: ");
      Serial.println(uidString);

      String message;
      if (isAuthorized(uidString)) {
        if(salaBloqueada){
          message = "Acesso autorizado para " + uidString;
          digitalWrite(LED1RFID, LOW);
          digitalWrite(LEDVERDE, HIGH);
          digitalWrite(BUZZER, LOW);
          flagAlteracao = 0;
          salaBloqueada = !salaBloqueada;
        } else {
          message = "Sala bloqueada por " + uidString;
          digitalWrite(LED1RFID, HIGH);
          digitalWrite(LEDVERDE, LOW);
          salaBloqueada = !salaBloqueada;
        }
      } else {
        message = "Acesso negado para " + uidString;
        digitalWrite(LED1RFID, HIGH);
        digitalWrite(LEDVERDE, LOW);
      }
      Serial.println(message);
      delay(1000); // Adiciona um pequeno delay para não detectar a tag repetidamente
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1RFID, OUTPUT);
  pinMode(LEDVERDE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PIR, INPUT);
  setup_wifi();

  // Inicializando o sensor RFID
  nfc.begin();

  // Obtém a versão do firmware do PN532 para garantir que está funcionando corretamente
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    // Se não houver resposta do módulo, exibe uma mensagem de erro
    Serial.println("Não foi possível encontrar o leitor NFC.");
    while (1); // Trava o código se o leitor não for encontrado
  }

  // Exibe as informações do firmware no monitor serial
  Serial.print("Firmware do leitor NFC: 0x");
  Serial.println(versiondata, HEX); // Exibe a versão do firmware em hexadecimal

  // Configura o leitor NFC para modo passivo (para ler tags RFID/NFC)
  nfc.SAMConfig();
}

void loop() {
  // Chama a função para verificar a tag RFID
  verificarRFID();
  if(salaBloqueada){
    digitalWrite(LED1RFID, HIGH);
  }

  // flagAlteracao = digitalRead(PIR);
  flagAlteracao = digitalRead(PIR);
  Serial.println(flagAlteracao);

  analogReadResolution(RESOLUTION);
  leituraLDR1 = analogRead(LDR1);
  leituraLDR2 = analogRead(LDR2);
  leituraLDR3 = analogRead(LDR3);
  delay(1000);

  // Serial.println(leituraLDR1);
  if(leituraLDR1 >= 600 || leituraLDR2 >= 600 || ){
    
  }

  if(flagAlteracao){
    digitalWrite(BUZZER, HIGH);
    sendMessage(phoneNumber, apiKey, "Alteração Detectada ");
  } else {
    digitalWrite(BUZZER, LOW);
  }
}
