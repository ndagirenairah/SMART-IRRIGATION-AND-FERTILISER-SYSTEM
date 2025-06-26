// ThingSpeak WiFi Uploader with Analog Soil Sensors (Arduino Mega + ESP8266 using Serial1)

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASS";
const char* apiKey   = "YOUR_API_KEY";

// Sensor pins
#define SOIL_MOISTURE_PIN A0
#define PH_SENSOR_PIN     A1
#define N_SENSOR_PIN      A2
#define P_SENSOR_PIN      A3
#define K_SENSOR_PIN      A4
#define EC_SENSOR_PIN     A5

void setup() {
  Serial.begin(9600);     // For Serial Monitor
  Serial1.begin(9600);    // Connect ESP8266 TX to Mega pin 19, RX to pin 18

  Serial.println("Connecting to WiFi...");
  sendAT("AT+RST", 2000);
  sendAT("AT+CWMODE=1", 1000);
  sendAT("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"", 5000);
}

void loop() {
  int rawMoisture = analogRead(SOIL_MOISTURE_PIN);
  int rawPH       = analogRead(PH_SENSOR_PIN);
  int rawN        = analogRead(N_SENSOR_PIN);
  int rawP        = analogRead(P_SENSOR_PIN);
  int rawK        = analogRead(K_SENSOR_PIN);
  int rawEC       = analogRead(EC_SENSOR_PIN);

  float moisture  = map(rawMoisture, 0, 1023, 0, 100);
  float pH        = map(rawPH, 0, 1023, 30, 100) / 10.0;
  int nitrogen    = map(rawN, 0, 1023, 0, 100);
  int phosphorus  = map(rawP, 0, 1023, 0, 100);
  int potassium   = map(rawK, 0, 1023, 0, 100);
  float voltage   = rawEC * (5.0 / 1023.0);
  float ec        = (voltage / 3.5) * 3500.0;
  float tds       = ec * 0.5;

  ec  = max(0.0, ec);
  tds = max(0.0, tds);

  Serial.println("------ Sensor Readings ------");
  Serial.print("Soil Moisture: "); Serial.print(moisture); Serial.println("%");
  Serial.print("pH Level: "); Serial.println(pH);
  Serial.print("Nitrogen: "); Serial.println(nitrogen);
  Serial.print("Phosphorus: "); Serial.println(phosphorus);
  Serial.print("Potassium: "); Serial.println(potassium);
  Serial.print("EC: "); Serial.print(ec, 2); Serial.println(" μS/cm");
  Serial.print("TDS: "); Serial.print(tds, 2); Serial.println(" ppm");
  Serial.println("-----------------------------\n");

  String getData = "GET /update?api_key=agroset.us" + String(apiKey) +
    "&field1=" + String(moisture) +
    "&field2=" + String(pH) +
    "&field3=" + String(nitrogen) +
    "&field4=" + String(phosphorus) +
    "&field5=" + String(potassium) +
    "&field6=" + String(ec, 2) +
    "&field7=" + String(tds, 2);

  Serial.println("Sending to agroset...");
  sendAT("AT+CIPMUX=1", 1000);
  sendAT("AT+CIPSTART=0,\"TCP\",\"api.agroset.us\",80", 2000);
  String sendCmd = "AT+CIPSEND=0," + String(getData.length() + 4);
  sendAT(sendCmd, 1000);
  Serial1.println(getData);
  delay(2000);
  sendAT("AT+CIPCLOSE=0", 1000);
  Serial.println("Data sent.\n");

  delay(20000);
}

void sendAT(String cmd, const int timeout) {
  Serial1.println(cmd);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }
  }
}
