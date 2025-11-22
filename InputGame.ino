#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ---------- TFT DISPLAY ----------
#define TFT_CS    9
#define TFT_DC    8
#define TFT_RST   10
#define TFT_SCLK  6
#define TFT_MOSI  7

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ---------- JOYSTICK & BUTTON ----------
#define JOY_X     3
#define JOY_Y     2
#define JOY_BTN   1
#define BTN_EXT   21

// ---------- POTENTIOMETER ----------
#define POT_PIN   4

// ---------- JOYSTICK CAL ----------
#define JOY_CENTER_X 2240
#define JOY_CENTER_Y 2740
#define DEADZONE 1500

// ---------- ESP-NOW ----------
uint8_t receiverMAC[] = {0x0C, 0x4E, 0xA0, 0x4D, 0x53, 0x7C};  // robot
uint8_t hopperMAC[]   = {0x0C, 0x4E, 0xA0, 0x4D, 0x50, 0xF8};  // hopper

// ---------- UI STATE ----------
int mode = 0;   
// 0 = Welcome
// 1 = Launch Robot
// 2 = Hopper Control

int currentIndex = 0;

// Hopper options include Exit
const char* hopperOptions[] = {"Lifesavers", "Hi-Chew", "Exit"};
int hopperCount = 3;

// ---------- Timing ----------
unsigned long lastMoveTime = 0;
const int moveDelay = 300;

// ---------- CALLBACK ----------
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ---------- MIRROR FIX ----------
void setMirrorFix(uint8_t val) {
  SPI.beginTransaction(SPISettings(24000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, LOW);
  SPI.transfer(0x36);
  digitalWrite(TFT_DC, HIGH);
  SPI.transfer(val);
  digitalWrite(TFT_CS, HIGH);
  SPI.endTransaction();
}

// ---------- DRAW WELCOME SCREEN ----------
void drawWelcomeScreen() {
  tft.fillScreen(ST77XX_BLACK);

  // --- Big Title ---
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_GREEN);

  int w1 = 8 * 18;  // "Welcome!"
  tft.setCursor((240 - w1) / 2, 40);
  tft.println("Welcome!");

  // --- Middle Line 1 ---
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  int w2 = 14 * 12; // "Place a cube in"
  tft.setCursor((240 - w2) / 2, 110);
  tft.println("Place a cup in");

  // --- Middle Line 2 ---
  int w3 = 20 * 12; // "the robot to begin!"
  tft.setCursor((240 - w3) / 2, 140);
  tft.println("the robot to begin!");

  // --- Bottom instructions (TWO LINES) ---
  // "Press button"
  int w4 = 12 * 12;
  tft.setCursor((240 - w4) / 2, 220);
  tft.println("Press button");

  // "to start"
  int w5 = 7 * 12;
  tft.setCursor((240 - w5) / 2, 240);
  tft.println("to start");
}


// ---------- DRAW LAUNCH SCREEN ----------
void drawLaunchScreen() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_GREEN);

  // Center "Launch"
  int w1 = 6 * 18;
  tft.setCursor((240 - w1) / 2, 60);
  tft.println("Launch");

  // Center "Robot"
  int w2 = 5 * 18;
  tft.setCursor((240 - w2) / 2, 110);
  tft.println("Robot");

  // Two-line instructions
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  // "Press button"
  int w3 = 12 * 12;
  tft.setCursor((240 - w3) / 2, 220);
  tft.println("Press button");

  // "to start"
  int w4 = 7 * 12;
  tft.setCursor((240 - w4) / 2, 240);
  tft.println("to start");
}




// ---------- DRAW HOPPER MENU ----------
void drawHopperMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(40, 10);
  tft.println("Hopper Control");

  for (int i = 0; i < hopperCount; i++) {
    if (i == currentIndex)
      tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

    tft.setCursor(60, 60 + (i * 40));
    tft.println(hopperOptions[i]);
  }

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(40, 200);
  tft.setTextSize(2);
  tft.println("Amount:");
}

// ---------- UPDATE AMOUNT DISPLAY ----------
void drawAmount(int amt) {
  tft.fillRect(160, 200, 40, 40, ST77XX_BLACK);
  tft.setCursor(160, 200);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.print(amt);
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);

  digitalWrite(TFT_RST, LOW);
  delay(200);
  digitalWrite(TFT_RST, HIGH);
  delay(300);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  delay(50);
  tft.init(240, 320, SPI_MODE3);

  tft.setRotation(0);
  setMirrorFix(0x48);

  drawWelcomeScreen();

  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(BTN_EXT, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Add robot peer
  esp_now_peer_info_t robotPeer = {};
  memcpy(robotPeer.peer_addr, receiverMAC, 6);
  robotPeer.channel = 6;
  robotPeer.encrypt = false;
  esp_now_add_peer(&robotPeer);

  // Add hopper peer
  esp_now_peer_info_t hopperPeer = {};
  memcpy(hopperPeer.peer_addr, hopperMAC, 6);
  hopperPeer.channel = 6;
  hopperPeer.encrypt = false;
  esp_now_add_peer(&hopperPeer);

  Serial.println("ESP-NOW ready!");
}

// ---------- LOOP ----------
void loop() {
  int potVal = analogRead(POT_PIN);
  int amount = map(potVal, 0, 4095, 0, 9);

  int xVal = analogRead(JOY_X);
  bool joyPressed = !digitalRead(JOY_BTN);
  bool extPressed = !digitalRead(BTN_EXT);

  unsigned long now = millis();

  int xOffset = xVal - JOY_CENTER_X;
  int moveDir = 0;

  if (xOffset > DEADZONE) moveDir = -1;
  else if (xOffset < -DEADZONE) moveDir = 1;

  // ----- FIRST SCREEN: WELCOME -----
  if (mode == 0) {
    if (joyPressed || extPressed) {
      mode = 1;
      drawLaunchScreen();
      delay(300);
    }
    return;
  }

  // ----- LAUNCH SCREEN -----
  if (mode == 1) {
    if (joyPressed || extPressed) {
      const char* msg = "GO";
      esp_now_send(receiverMAC, (uint8_t*)msg, strlen(msg));

      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(3);
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(40, 140);
      tft.println("Launched!");
      delay(800);

      mode = 2;
      currentIndex = 0;
      drawHopperMenu();
      drawAmount(amount);
      delay(300);
    }
    return;
  }

  // ----- HOPPER NAVIGATION -----
  if (mode == 2 && moveDir != 0 && now - lastMoveTime > moveDelay) {
    currentIndex += moveDir;
    if (currentIndex < 0) currentIndex = hopperCount - 1;
    if (currentIndex >= hopperCount) currentIndex = 0;
    drawHopperMenu();
    drawAmount(amount);
    lastMoveTime = now;
  }

  // ----- LIVE AMOUNT UPDATE -----
  if (mode == 2) {
    drawAmount(amount);
  }

  // ----- HOPPER BUTTON PRESSED -----
  if (mode == 2 && (joyPressed || extPressed)) {

    if (currentIndex == 2) {
      // EXIT BACK TO LAUNCH
      mode = 1;
      drawLaunchScreen();
    }
    else {
      // hopper number
      char hopperNum = (currentIndex == 0) ? '1' : '2';
      char amountChar = '0' + amount;

      char packet[3] = {hopperNum, amountChar, '\0'};
      esp_now_send(hopperMAC, (uint8_t*)packet, 2);

      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(3);
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(40, 140);
      tft.println("Sent!");
      delay(600);

      drawHopperMenu();
      drawAmount(amount);
    }

    delay(300);
  }
}
