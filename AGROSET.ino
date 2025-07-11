#include <SoftwareSerial.h>

// === GSM on pins RX=6, TX=5 ===
SoftwareSerial gsm(6, 5);

// === Configuration ===
const char* deviceID = "DEVICE_001";
const char* apn      = "internet";
const char* user     = "internet";
const char* pass     = "";

// === Sensor Pins ===
#define SOIL_MOISTURE_PIN A0
#define PH_SENSOR_PIN     A1
#define N_SENSOR_PIN      A2
#define P_SENSOR_PIN      A3
#define K_SENSOR_PIN      A4
#define EC_SENSOR_PIN     A5

// === Relay Pins ===
#define RELAY_WATER_PIN      9
#define RELAY_FERTILISER_PIN 10

bool firstRun = true;

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);

  pinMode(RELAY_WATER_PIN, OUTPUT);
  pinMode(RELAY_FERTILISER_PIN, OUTPUT);
  digitalWrite(RELAY_WATER_PIN, HIGH);     // OFF
  digitalWrite(RELAY_FERTILISER_PIN, HIGH);// OFF

  Serial.println("\n--- SYSTEM START ---");
  initGPRS();
}

void loop() {
  // === Read Sensors ===
  int moistureRaw = analogRead(SOIL_MOISTURE_PIN);
  int rawPH       = analogRead(PH_SENSOR_PIN);
  int rawN        = analogRead(N_SENSOR_PIN);
  int rawP        = analogRead(P_SENSOR_PIN);
  int rawK        = analogRead(K_SENSOR_PIN);
  int rawEC       = analogRead(EC_SENSOR_PIN);

  float pH         = map(rawPH, 0, 1023, 30, 100) / 10.0;
  int   nitrogen   = map(rawN, 0, 1023, 0, 100);
  int   phosphorus = map(rawP, 0, 1023, 0, 100);
  int   potassium  = map(rawK, 0, 1023, 0, 100);
  float voltage    = rawEC * (5.0 / 1023.0);
  float ec         = (voltage / 3.5) * 3500.0;
  float tds        = ec * 0.5;

  // === Print to Serial Monitor ===
  Serial.println("\n=== Sensor Readings ===");
  Serial.print("Moisture (raw): "); Serial.println(moistureRaw);
  Serial.print("pH: "); Serial.println(pH, 1);
  Serial.print("N: "); Serial.print(nitrogen);
  Serial.print(" | P: "); Serial.print(phosphorus);
  Serial.print(" | K: "); Serial.println(potassium);
  Serial.print("EC: "); Serial.print(ec, 2);
  Serial.print(" µS/cm | TDS: "); Serial.println(tds, 2);

  // === Relay Logic ===
  bool needWater = (moistureRaw == 30);
  bool needFert  = (nitrogen < 30 || phosphorus < 30 || potassium < 30);

  if (firstRun) {
    Serial.println("First run: forcing water pump ON");
    digitalWrite(RELAY_WATER_PIN, LOW);  // ON
    firstRun = false;
  } else {
    digitalWrite(RELAY_WATER_PIN, needWater ? LOW : HIGH);
  }

  digitalWrite(RELAY_FERTILISER_PIN, needFert ? LOW : HIGH);

  Serial.println((!firstRun && needWater) ? "Water pump ON" : "Water pump OFF");
  Serial.println(needFert ? "Fertiliser ON" : "Fertiliser OFF");

  // === Construct URL ===
  String url = "http://api.agroset.us/update?"
               "id=" + String(deviceID) +
               "&moisture=" + String(moistureRaw) +
               "&ph=" + String(pH, 1) +
               "&n=" + String(nitrogen) +
               "&p=" + String(phosphorus) +
               "&k=" + String(potassium) +
               "&ec=" + String(ec, 2) +
               "&tds=" + String(tds, 2);

  Serial.println("Sending to server:");
  Serial.println(url);

  // === Send Data via GSM ===
  sendGSM("AT+HTTPTERM");
  sendGSM("AT+HTTPINIT");
  sendGSM("AT+HTTPPARA=\"CID\",1");
  sendGSM("AT+HTTPPARA=\"URL\",\"" + url + "\"");
  sendGSM("AT+HTTPACTION=0");
  delay(3000);
  sendGSM("AT+HTTPREAD");
  sendGSM("AT+HTTPTERM");

  Serial.println("Cycle complete.");
  delay(2000);
}

void initGPRS() {
  sendGSM("AT");
  sendGSM("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sendGSM("AT+SAPBR=3,1,\"APN\",\"" + String(apn) + "\"");
  if (user[0]) sendGSM("AT+SAPBR=3,1,\"USER\",\"" + String(user) + "\"");
  if (pass[0]) sendGSM("AT+SAPBR=3,1,\"PWD\",\"" + String(pass) + "\"");
  sendGSM("AT+SAPBR=1,1");
  sendGSM("AT+SAPBR=2,1");
}

void sendGSM(const String &cmd) {
  gsm.println(cmd);
  delay(800);
  while (gsm.available()) Serial.write(gsm.read());
}
