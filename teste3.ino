#include <arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Ultrasonic.h>

// Definições do DHT11
#define DHTPIN 4
#define DHTTYPE DHT11

#define LED1 25
#define LED2 26
#define LED3 27

#define trig 5
#define echo 18

/* Definicoes para o MQTT */
#define TOPICO_PUBLISH_DISTANCIA   "topico_publica_distancia"
#define TOPICO_PUBLISH_TEMPERATURA  "topico_publica_temp"
#define TOPICO_PUBLISH_UMIDADE  "topico_publica_umidade"

#define ID_MQTT  "Smart_Walker_mqtt"     //id mqtt (para identificação de sessão)

//Casa Amanda
const char* SSID     = "BLFIBRA VALERIA"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "295017AVL"; // Senha da rede WI-FI que deseja se conectar
const char* BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

Ultrasonic ultrasonic(trig,echo);
DHT dht(DHTPIN, DHTTYPE);

int distance;
unsigned long startTime; // Armazena o tempo de início

/* Prototypes */
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);
void valorUmidadeETemperatura();
void ultrassonico();
void printElapsedTime();

void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnectWiFi();
}

void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String msg;

  /* obtem a string do payload recebido */
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("Chegou a seguinte string via MQTT: ");
  Serial.println(msg);
}

void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }  
}

void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
  reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

void reconnectWiFi(void)
{
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("\nIP obtido: ");
  Serial.println(WiFi.localIP());
}

void ultrassonico(){
  distance = ultrasonic.read();
  Serial.print("Distância em CM: ");
  Serial.println(distance);

  if(distance <30){
    MQTT.publish(TOPICO_PUBLISH_DISTANCIA, "CUIDADO, MUITO PERTO");
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  } else if(distance > 30 && distance <50){
    MQTT.publish(TOPICO_PUBLISH_DISTANCIA, "SE APROXIMANDO...");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
  }else if(distance >= 50){
    MQTT.publish(TOPICO_PUBLISH_DISTANCIA, "DISTÂNCIA SEGURA");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH);
  }
}

void valorUmidadeETemperatura(){
  long h = dht.readHumidity();
  long t = dht.readTemperature();

  if (isnan(t)||isnan(h)) {
    Serial.println("Falha na leitura do DHT");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println("°C");
    Serial.print("Umidade: ");
    Serial.print(h);
    Serial.println("%");

    char temperatura_str[13] = {0};
    char umidade_str[13] = {0};
    sprintf(temperatura_str, "%ld°C", t);
    sprintf(umidade_str, "%ld%%", h);

    MQTT.publish(TOPICO_PUBLISH_TEMPERATURA, temperatura_str);
    MQTT.publish(TOPICO_PUBLISH_UMIDADE, umidade_str);
  }
}

void printElapsedTime() {
  unsigned long elapsedTime = millis() - startTime;
  unsigned long seconds = (elapsedTime / 1000) % 60;
  unsigned long minutes = (elapsedTime / 60000) % 60;
  unsigned long hours = elapsedTime / 3600000;

  Serial.printf("Tempo decorrido: %02lu:%02lu:%02lu\n", hours, minutes, seconds);
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Disciplina IoT: acesso a nuvem via ESP32");
  delay(500);
  initWiFi();
  initMQTT();
  dht.begin();
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  startTime = millis(); // Salva o tempo inicial
}

void loop() {
  VerificaConexoesWiFIEMQTT();
  valorUmidadeETemperatura();
  ultrassonico();
  printElapsedTime();
  MQTT.loop();
  delay(2000);
}
