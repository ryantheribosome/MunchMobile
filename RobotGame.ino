#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ---------- TB6612FNG MOTOR PINS ----------
#define PWMA 4   // Right motor
#define AIN1 5
#define AIN2 6
#define PWMB 7   // Left motor
#define BIN1 8
#define BIN2 9
#define STBY 10

// ---------- QTR SENSOR ----------
#define QTR_PIN 3
#define QTR_MIN 300
#define QTR_MAX 400

// ---------- MOTOR SETTINGS ----------
int baseSpeedRight = 180;
int baseSpeedLeft  = 200;

// ---------- ALLOWED SENDER MAC ----------
uint8_t allowedMAC[] = {0x0C, 0x4E, 0xA0, 0x4D, 0x4E, 0x1C};

// ---------- GLOBAL ----------
int tapeCount = 0;
bool cycleComplete = false;

// ---------- MOTOR CONTROL ----------
void setMotorSpeedsReverse(int leftSpeed, int rightSpeed) {
  digitalWrite(STBY, HIGH);
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMA, constrain(rightSpeed, 0, 255));
  analogWrite(PWMB, constrain(leftSpeed, 0, 255));
}

void brakeMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, HIGH);
}

void forceStop() {
  brakeMotors();
  delay(50);
  digitalWrite(STBY, LOW);
  delay(100);
  digitalWrite(STBY, HIGH);
}

// ---------- MAIN BACKWARD LOGIC ----------
void runBackwardFlow() {
  Serial.println("🔄 Running backward tape sequence...");

  tapeCount = 0;

  while (true) {
    int qtrValue = analogRead(QTR_PIN);
    Serial.print("QTR: ");
    Serial.println(qtrValue);

    // DRIVE BACKWARD CONSTANTLY (with bias)
    const int bias = -5;
    int leftSpeed  = baseSpeedLeft  - bias;
    int rightSpeed = baseSpeedRight + bias;

    setMotorSpeedsReverse(leftSpeed, rightSpeed);

    // ---------- BLACK TAPE (final stop) ----------
    if (qtrValue > 3000) {
      Serial.println("⬛ BLACK TAPE — FINAL STOP");
      forceStop();
      cycleComplete = true;
      return;
    }

    // ---------- GRAY TAPE #1 OR #2 ----------
    if (qtrValue >= QTR_MIN && qtrValue <= QTR_MAX) {

      tapeCount++;
      Serial.printf("⬇️ Gray Tape #%d detected\n", tapeCount);
      forceStop();

      // ----------- 10 second pause -----------
      Serial.println("⏸️ Pausing 10 seconds...");
      delay(10000);

      // ----------- STOP AFTER 2 TAPES -----------
      if (tapeCount >= 2) {
        Serial.println("👉 2 tapes detected — continuing to black tape...");
      }

      // ----------- 800ms debounce (drive backward slowly) -----------
      Serial.println("🔄 800ms tape debounce...");
      unsigned long debounceStart = millis();
      while (millis() - debounceStart < 800) {
        setMotorSpeedsReverse(baseSpeedLeft, baseSpeedRight);
        delay(10);
      }

      // resume loop
    }

    delay(20);
  }
}

// ---------- ESP-NOW CALLBACK ----------
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {

  // Check MAC
  for (int i = 0; i < 6; i++) {
    if (info->src_addr[i] != allowedMAC[i]) {
      Serial.println("🚫 Unauthorized sender");
      return;
    }
  }

  if (!cycleComplete) {
    Serial.println("🚀 Command received — starting backward sequence!");
    runBackwardFlow();
  } else {
    Serial.println("⚠️ Cycle already complete — ignoring.");
  }
}

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  forceStop();

  pinMode(QTR_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  Serial.println("🟢 Robot ready for launch!");
}

// ---------- LOOP ----------
void loop() {
  delay(50);
}
