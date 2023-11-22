#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ESP32Servo.h>

#define LED 13
#define RELAY1 12
#define RELAY2 15

#define TOPIC "esp32/IoT"
#define TOPICM "message/send"

// Coloque na variavel ssid o nome do seu wifi e no password a senha
const char* ssid = "Visitantes";
const char* password = "Guest20.2";
const char* mqtt_server = "172.16.5.37"; // Ip do computador ao qual foi ativado o servidor mosquitto

// Configuração Servo motor
Servo servoMotor;       // Cria um objeto do tipo Servo
int servoPin = 4;      // Pino digital conectado ao servo motor
int anguloAberto = 79;  // Ângulo de abertura da cancela
int anguloFechado = 1;  // Ângulo de fechamento da cancela

// Configuração MQTT
WiFiClient espClient;           
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];


int ligaLED = 1;

// Estrutura de variaveis para ser gravado na eeprom do esp32
struct parameter {
  char r_msg[50];
};

struct parameter parameter_cam;

parameter dataToWriteEEPROM;

const int EEPROM_SIZE = sizeof(parameter_cam);
const int EEPROM_ADDR = 0;  //  Endereço para inicialização da eeprom

void writeStructtoEEPROM(int addr, const parameter& data) {
  for (int i = 0; i < sizeof(data); i++) {
    EEPROM.write(addr + i, *((unsigned char*)&data + i));
  }
  EEPROM.commit();
}

parameter readStructFromEEPROM(int addr) {
  parameter data;
  for (int i = 0; i < sizeof(data); i++) {
    *((unsigned char*)&data + i) = EEPROM.read(addr + i);
  }
  return data;
}

void closeGate() {
  for(int i = anguloAberto; i > anguloFechado; i--){
    servoMotor.write(i);
    delay(10);
  }
  Serial.println("Porta Fechando");
}

void openGate() {
  for(int i = 1; i < anguloAberto; i++){
    servoMotor.write(i);
    delay(10);
  }
  Serial.println("Passagem Liberada");
}

void executCMD(String messageTemp) {
  if (messageTemp == "open") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    openGate();
  } else if (messageTemp == "close") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    closeGate();
  } else if (messageTemp == "R1ON") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    digitalWrite(RELAY1, 1);
  } else if (messageTemp == "R1OFF") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    digitalWrite(RELAY1, 0);
  } else if (messageTemp == "R2ON") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    digitalWrite(RELAY2, 1);
  } else if (messageTemp == "R2OFF") {
    client.publish(TOPICM, "OK");
    sprintf(parameter_cam.r_msg, "%s", messageTemp);
    writeStructtoEEPROM(EEPROM_ADDR, parameter_cam);
    Serial.println("Chegou: " + messageTemp);
    digitalWrite(RELAY2, 0);
  } else {
    client.publish(TOPICM, "Erro Comando Invalido");
  }
}

void setup() {
  Serial.begin(115200);
  servoMotor.attach(servoPin);  // Anexa o servo ao pino especificado
  servoMotor.write(anguloFechado);
  pinMode(LED, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  parameter_cam = readStructFromEEPROM(EEPROM_ADDR);
  delay(200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(LED, ligaLED);
    Serial.print(".");
    ligaLED = !ligaLED;
  }
  digitalWrite(LED, 1);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Changing output to ");
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == TOPIC) {
    if (strcmp(messageTemp.c_str(), parameter_cam.r_msg) == 0) {
      client.publish(TOPICM, "Erro Comando ja enviado");
    } else {
      executCMD(messageTemp);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("espClient")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
  }
}