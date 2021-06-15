#include <RBDdimmer.h>//
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define USE_SERIAL  Serial
#define outputPin  13 // Пин выхода (PWM)
#define zerocross  12 // Пин Zero-Cross (ZC)

const char* ssid = "SGm"; // Название вайфай сетки (такой же как у брокера)
const char* password = "13371024"; // Пароль вайфай сетки

const char* mqtt_server = "46.101.212.61"; // IP MQTT брокера

// Инициализирует классы вайфая и штуки для MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Инициализирует класс диммера
dimmerLamp dimmer(outputPin, zerocross); 

int outVal = 0;

// Переменные для таймера, чтобы не постоянно слать ответы
long now = millis();
long lastMeasure = 0;

// НЕ МЕНЯТЬ!
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// Эта функция вызывается, когда кто-то публикует сообщение в топике, на который подписана есп
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic); // Название топика
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i]; // Чтение самого сообщения в строку messageTemp
  }
  Serial.println();

  // Если топик lamp/brighness
  if(topic=="lamp/brightness"){
      Serial.print("Changing Room lamp to ");
      dimmer.setPower(messageTemp.toInt()); // Сконвертируй строку в число и поменяй яркость
      Serial.print(messageTemp.toInt());
  }
  Serial.println();
}

// Эта функция подключает к MQTT брокеру
void reconnect() {
  // Бесконечный цикл пока не подключимся
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Пытаемся подключиться с названием девайся ESP8266Client (у всех девайсов должно быть разное название)
    if (client.connect("ESP8266Client")) {
      Serial.println("connected"); 
      // Подключаемся к топику lamp/brightness (таким же образом можем подключиться одновременно и к другому топику)
      client.subscribe("lamp/brightness");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Ждем 5 секунд перед переподключением
      delay(5000);
    }
  }
}

void setup() {
  USE_SERIAL.begin(9600); 
  dimmer.begin(NORMAL_MODE, ON); 
  USE_SERIAL.println("Dimmer Program is starting...");
  USE_SERIAL.println("Set value");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void printSpace(int val)
{
  if ((val / 100) == 0) USE_SERIAL.print(" ");
  if ((val / 10) == 0) USE_SERIAL.print(" ");
}

void loop() {
  int preVal = outVal;
  // Два ифа ниже нужны чтобы подключалось
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");
//  if (USE_SERIAL.available())
//  {
//    int buf = USE_SERIAL.parseInt();
//    if (buf != 0) outVal = buf;
//    delay(200);
//    dimmer.setPower(outVal);
//  }
//
//  if (preVal != outVal)
//  {
//    USE_SERIAL.print("lampValue -> ");
//    printSpace(dimmer.getPower());
//    USE_SERIAL.print(dimmer.getPower());
//    USE_SERIAL.println("%");
//
//  }
  now = millis();
  // Каждые 10 секунд отправляем новое значение яркости
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    int power = dimmer.getPower(); // Считали яркость
    if (isnan(power)) {
      Serial.println("Failed to read power from lamp!");
      return;
    }
    char str[8];
    itoa(power, str, 10 ); // Преобразовали в массив char

    // Отправили данные в топик lamp/brightness
    client.publish("lamp/brightness", str);
    
    Serial.print("Power: ");
    Serial.print(power);
  }

}
