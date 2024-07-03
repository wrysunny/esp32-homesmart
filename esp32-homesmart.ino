#include <Wire.h>
#include <WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
// SSD1306 设置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C  // SSD1306 显示屏的 I2C 地址
#define OLED_SDA 21  // ESP32 上的 SDA 引脚
#define OLED_SCL 22  // ESP32 上的 SCL 引脚
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi 设置
const char* ssid = "xxxxxxxx";
const char* password = "xxxxxxxx";
// DHT 设置
#define DHTPIN 13 
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);
// MQTT 设置
const char* mqtt_server = "iot_mqtt.synology.pub";
const int mqtt_port = 1883;
const char* mqtt_user = "esp32";
const char* mqtt_password = "esp32@202407";
const char* mqtt_topic_temp = "sensor/temperature";
const char* mqtt_topic_hum = "sensor/humidity";
const char* mqtt_topic_heatindex = "sensor/heat_index";
WiFiClient espClient;
PubSubClient mqttclient(espClient);
String client_id = "ESP32-" + String(WiFi.macAddress());

void setup() {
  Serial.begin(9600);
  Wire.begin(OLED_SDA, OLED_SCL);  // 初始化 I2C 总线，并连接到指定的引脚
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  dht.begin();
  // 添加 WiFi 网络
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  // 等待 WiFi 连接
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  // MQTT
  mqttclient.setServer(mqtt_server,mqtt_port);
  while (!mqttclient.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqtt_reconnect()) {
      Serial.println("connected");
    } else {
      Serial.print("MQTT can't connect: ");
      Serial.println(mqttclient.state());
      delay(3000);
    }
  }

  // display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("STA IP:");
  display.setCursor(0, 20);
  display.println(WiFi.localIP().toString());
  display.setCursor(0, 40);
  display.println("By @javashell");
  display.setCursor(0, 50);
  display.println("build 20240703");
  display.display();
  delay(5000);
}

bool mqtt_reconnect(){
  if (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect(client_id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  return false;
}

void loop() {
  display.clearDisplay();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(F("Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.println();

  // 在 ssd1306 显示屏上显示信息
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(F("Hum :"));
  display.println(h);
  display.setCursor(0, 25);
  display.print(F("Temp:"));
  display.println(t);
  display.setCursor(0, 50);
  display.print(F("Heat:"));
  display.println(hic);
  display.display();

  // 发送到mqtt
  if (mqttclient.connected()){
    mqttclient.publish(mqtt_topic_hum,(String(h)+" %").c_str());
    mqttclient.publish(mqtt_topic_temp,(String(t)+" °C ").c_str());
    mqttclient.publish(mqtt_topic_heatindex,(String(hic)+" °C ").c_str());
  } else {
    mqtt_reconnect();
  }
  // 延时5秒
  delay(5000);
}
