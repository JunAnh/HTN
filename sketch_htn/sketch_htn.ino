//---------Thư viện---------
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>

//--------Khai báo chân---------
#define DHTPIN 12 // D6
#define DHTTYPE DHT11 
#define MoisturePIN A0 
#define pumpPin 2 //D4
//-----------Wifi-------
const char* ssid = "Tio";//"Phuong Anh";
const char* password = "24112001";// "phuonganh2001";

//----------MQTT--------
const char* mqtt_server = "192.168.43.254";   
const uint16_t mqtt_port = 1883;

//------------Khai báo biến------
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

long lastMsg,lastRead,lastProcess,lastWater = 0;
char msgMoisture[50];
char msgHumidity[50];
char msgTemperature[50];
int value = 0;
int mt = 0;
float t;
float h;

//--------Threshold------------
int moisThreshold = 70;
float humidThreshold = 70;
float minTempThreshold = 20;
float maxTempThreshold = 25;

//---------Setup------------
void setup() {
  dht.begin();  // Khởi động DHT11 Sensor
  pinMode(MoisturePIN, INPUT); // Khởi động cảm biến độ ẩm đất
  Serial.begin(115200); // Mở Serial
  setup_wifi(); //khởi động kết nối wifi
  client.setServer(mqtt_server, mqtt_port); // đặt server mqtt broker
  client.setCallback(callback); // đặt callback mqtt broker
}

void loop() {
  // put your main code here, to run repeatedly:
  autoProcess();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

 // Thực hiện 2s gửi dữ liệu lên broker 1 lần
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    publishData();
    client.subscribe("button");
    client.subscribe("event");
  }
}

//------------Gửi dữ liệu lên broker----------
void publishData(){
    snprintf (msgMoisture, 75, "Moisture:%ld", mt);
    Serial.print("Publish message: ");
    Serial.println(msgMoisture);
    client.publish("event", msgMoisture);
    
    snprintf (msgHumidity, 75, "Humidity:%ld", (int) h);
    Serial.print("Publish message: ");
    Serial.println(msgHumidity);
    client.publish("event", msgHumidity);

    snprintf (msgTemperature, 75, "Temperature:%ld", (int) t);
    Serial.print("Publish message: ");
    Serial.println(msgTemperature);
    client.publish("event", msgTemperature);
}

//------------Processing---------------
void autoProcess(){
  readSensor();
  long procestTime = millis();
  if (procestTime - lastProcess > 2000)
  {
    if (mt < moisThreshold || h < humidThreshold) {
      Serial.println("Tuoi cay"); 
      tuoi();   // tuoi cay 
    }
    else {
      Serial.println("Ngung tuoi");
      digitalWrite(pumpPin, LOW);   // ngung tuoi
    }
    lastProcess = procestTime;
  }
}

// -----------Read Sensors-------------
int readMoisture(){
  int moisture = analogRead(MoisturePIN);
  float m = (moisture * 100) / 1023;
  return m;
}

float readTemperatureDHT(){
  float temp = dht.readTemperature();
    if (isnan(temp)) {
    Serial.println(F("Đọc nhiệt độ thất bại!"));
    }
  return temp;
}

float readHumidityDHT(){
  float humid = dht.readHumidity();
    if (isnan(humid)) {
    Serial.println(F("Đọc độ ẩm thất bại!"));
    }
  return humid;
}

void readSensor(){
  long sNow = millis();
  if (sNow - lastRead > 1000) // read sensors per second
  {
    lastRead = sNow;
    mt = readMoisture();
    t = readTemperatureDHT();
    h = readHumidityDHT();
  }
}

//----------------CALLBACK-------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    Serial.println("Tưới");
    tuoi();
  } 
  if ((char)payload[0] == '0') {
    Serial.println("Ngừng");
    digitalWrite(pumpPin, LOW);
  }
  
  // Set Threshold khi chọn Cà chua
  if ((char)payload[0] == '2') {
    Serial.println("CA CHUA");
    moisThreshold = 70;
    humidThreshold = 60;
    minTempThreshold = 26;
    maxTempThreshold = 29;
  }

  // Set Threshold khi chọn Dưa leo
  if ((char)payload[0] == '3') {
    Serial.println("DUA LEO");
    moisThreshold = 85;
    humidThreshold = 90;
    minTempThreshold = 25;
    maxTempThreshold = 30;
  }

  // Set Threshold khi chọn Đậu cô-ve
  if ((char)payload[0] == '5') {
    Serial.println("DAU COVE");
    moisThreshold = 70;
    humidThreshold = 65;
    minTempThreshold = 20;
    maxTempThreshold = 25;
  }

  // Set Threshold khi chọn Dâu tây
  if ((char)payload[0] == '6') {
    Serial.println("DAU TAY");
    moisThreshold = 70;
    humidThreshold = 84;
    minTempThreshold = 15;
    maxTempThreshold = 24;
  }
  // Set Threshold khi chọn Ớt
  if ((char)payload[0] == '7') {
    Serial.println("OT");
    moisThreshold = 70;
    humidThreshold = 70;
    minTempThreshold = 25;
    maxTempThreshold = 28;
  }
}

//--------------Tưới cây-----------
void tuoi()
{
  long thisTime = millis();
  if (lastWater - thisTime < 2000)
      digitalWrite(pumpPin, HIGH);
  thisTime = lastWater = 0;    
}

//------------Kết nối lại khi bị mất kết nối-------------
void reconnect() {
  // Đợi tới khi kết nối
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Khi kết nối thành công sẽ gửi chuỗi helloworld lên topic event
      client.publish("event", "Hello World");
      // ... sau đó sub lại thông tin
      client.subscribe("event");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//----------------WIFI SETUP----------------
void setup_wifi() {
  delay(10);
  // Wifi connecting 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
