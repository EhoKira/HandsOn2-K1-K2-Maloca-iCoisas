#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>  // Certifique-se de incluir a biblioteca para conexão segura
#include <Adafruit_PN532.h>
#include <PubSubClient.h>

// Configuração do Wi-Fi
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Configuração do sensor PN532 via I2C
#define led_vermelho 32
#define led_verde 33
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Configuração do Broker MQTT
const char* mqtt_server = "URL_SERVER";  // Broker MQTT
const int mqtt_port = 8883;  // Porta MQTT com SSL/TLS
const char* mqtt_username = "USER";  // Nome de usuário MQTT
const char* mqtt_password = "password";  // Senha do MQTT
const char* mqtt_topic = "ativos/cadastrar";  // Tópico para cadastro

WiFiClientSecure espClient;  // Conexão segura com o MQTT
PubSubClient client(espClient);  // Cliente MQTT

// Função para conectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao Broker MQTT!");
    } else {
      Serial.print("Falha na conexão. Status: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(led_vermelho, OUTPUT);
  pinMode(led_verde, OUTPUT);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.print("IP_Address: ");
  Serial.println(WiFi.localIP());

  // Inicializa PN532
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 nao detectado");
    while (1);
  }
  nfc.SAMConfig();

  // Configuração MQTT
  espClient.setInsecure();  // Usa conexão sem verificação de certificado (se necessário, configure o certificado)
  client.setServer(mqtt_server, mqtt_port);
}

void cartao_valido_aproximado(){
  digitalWrite(led_vermelho, LOW);
  digitalWrite(led_verde, HIGH);
  delay(2000);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_vermelho, HIGH);
}

void loop() {
  // Verifica se a conexão MQTT está ativa
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  digitalWrite(led_vermelho, HIGH);
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String tag = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      tag += String(uid[i], HEX);
    }
    Serial.println("Tag detectada: " + tag);

    // Preparar os dados em formato JSON para enviar ao MQTT
    String postData;
    if (tag == "83593810") {
      cartao_valido_aproximado();
      postData = "{\"tag\":\"" + tag + "\", \"descricao\":\"Maquina de pressao arterial\", \"localizacao\":\"Leito 1\"}";
    } else if (tag == "73752d10") { // Cartão RFID
      cartao_valido_aproximado();
      postData = "{\"tag\":\"" + tag + "\", \"descricao\":\"Maquina de Anestesia\", \"localizacao\":\"Leito 2\"}";
    } else {
      cartao_valido_aproximado();
      postData = "{\"tag\":\"" + tag + "\", \"descricao\":\"Ativo desconhecido\", \"localizacao\":\"Setor nao informado\"}";
    }

    // Enviar os dados para o broker MQTT
    Serial.println("Enviando para o MQTT: " + postData);
    client.publish(mqtt_topic, postData.c_str());

    delay(5000);  // Aguarde antes de nova leitura
  }
}
